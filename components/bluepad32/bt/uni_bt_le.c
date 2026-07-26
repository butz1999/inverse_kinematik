/*
 * Copyright (C) 2017 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

/*
 * Copyright (C) 2023 Ricardo Quesada
 * Unijoysticle additions based on BlueKitchen's test/example code
 */

/*
 * Execution order:
 *  uni_bt_le_on_gap_event_advertising_report()
 *      -> hog_connect()
 *  uni_sm_packet_handler()
 *  wait for SM_EVENT_REENCRYPTION_COMPLETE or SM_EVENT_PAIRING_COMPLETE
 *  uni_device_information_packet_handler()
 *  wait for GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE
 *  uni_hids_client_packet_handler()
 *  wait for GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED
 *  uni_hid_device_set_ready()
 */

#include "bt/uni_bt_le.h"

#include <bluetooth_data_types.h>
#include <btstack.h>
#include <btstack_config.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "bt/uni_bt_conn.h"
#include "bt/uni_bt_defines.h"
#include "parser/uni_hid_parser.h"
#include "uni_common.h"
#include "uni_config.h"
#include "uni_hid_device.h"
#include "uni_log.h"
#include "uni_property.h"

static bool is_scanning;
static bool ble_enabled;
static uni_bt_le_advertisement_debug_t advertisement_debug[12];
static uint8_t advertisement_debug_next;
static uint8_t advertisement_debug_count;
static btstack_timer_source_t service_dump_timer;
static hci_con_handle_t service_dump_con_handle = UNI_BT_CONN_HANDLE_INVALID;
#define GATT_POC_MAX_SERVICES 8
#define GATT_POC_MAX_NOTIFY_CHARACTERISTICS 8
static gatt_client_service_t service_dump_services[GATT_POC_MAX_SERVICES];
static gatt_client_characteristic_t notify_dump_characteristics[GATT_POC_MAX_NOTIFY_CHARACTERISTICS];
static gatt_client_notification_t notify_dump_listeners[GATT_POC_MAX_NOTIFY_CHARACTERISTICS];
static uint8_t service_dump_count;
static uint8_t characteristic_dump_index;
static uint8_t notify_dump_count;
static uint8_t notify_config_index;
static uint8_t notify_cccd_value[] = {0x01, 0x00};
static uint32_t notification_dump_count;
static uint32_t notification_dump_suppressed_count;
static uint32_t notification_dump_last_log_ms;

// Temporal space for SDP in BLE
static uint8_t hid_descriptor_storage[HID_MAX_DESCRIPTOR_LEN * CONFIG_BLUEPAD32_MAX_DEVICES];
static btstack_packet_callback_registration_t sm_event_callback_registration;

static void uni_hids_client_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size);

static void print_uuid128(const uint8_t* uuid128) {
    for (int i = 0; i < 16; i++)
        printf("%02x", uuid128[i]);
}

static void characteristic_dump_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size);
static void start_next_notification_config(void);
extern void ik_switch2_pro_ble_input_report(const uint8_t* report, uint16_t report_size);

static void notification_dump_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    const uint8_t* value;
    uint32_t now_ms;
    uint16_t value_handle;
    uint16_t value_len;
    uint16_t preview_len;
    bool should_log;

    ARG_UNUSED(packet_type);
    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    if (hci_event_packet_get_type(packet) != GATT_EVENT_NOTIFICATION)
        return;

    value_handle = gatt_event_notification_get_value_handle(packet);
    value_len = gatt_event_notification_get_value_length(packet);
    value = gatt_event_notification_get_value(packet);

    if (value_handle == 0x002e && value_len >= 63 && value[1] == 0x20)
        ik_switch2_pro_ble_input_report(value, value_len);

    notification_dump_count++;
    now_ms = btstack_run_loop_get_time_ms();
    should_log = notification_dump_count <= 8 ||
                 btstack_time_delta(now_ms, notification_dump_last_log_ms) >= 1000;
    if (!should_log) {
        notification_dump_suppressed_count++;
        return;
    }

    preview_len = value_len < 16 ? value_len : 16;
    printf("GATT-POC notification sample count=%" PRIu32 " suppressed=%" PRIu32
           " handle=0x%04x len=%u first%u=",
           notification_dump_count, notification_dump_suppressed_count, value_handle, value_len, preview_len);
    for (int i = 0; i < preview_len; i++)
        printf("%02x", value[i]);
    printf("\n");
    notification_dump_suppressed_count = 0;
    notification_dump_last_log_ms = now_ms;
}

static void notification_config_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    ARG_UNUSED(packet_type);
    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    if (hci_event_packet_get_type(packet) != GATT_EVENT_QUERY_COMPLETE)
        return;

    printf("GATT-POC notify config complete index=%u att=0x%02x\n", notify_config_index,
           gatt_event_query_complete_get_att_status(packet));
    notify_config_index++;
    start_next_notification_config();
}

static void start_next_notification_config(void) {
    uint8_t status;
    uint16_t cccd_handle;
    gatt_client_characteristic_t* characteristic;

    if (notify_config_index >= notify_dump_count) {
        printf("GATT-POC notify config complete; move controller inputs now\n");
        return;
    }

    characteristic = &notify_dump_characteristics[notify_config_index];
    printf("GATT-POC listen+enable notify index=%u value=0x%04x props=0x%02x uuid=",
           notify_config_index, characteristic->value_handle, characteristic->properties);
    if (characteristic->uuid16 != 0) {
        printf("0x%04x\n", characteristic->uuid16);
    } else {
        print_uuid128(characteristic->uuid128);
        printf("\n");
    }

    gatt_client_listen_for_characteristic_value_updates(&notify_dump_listeners[notify_config_index],
                                                       notification_dump_packet_handler, service_dump_con_handle,
                                                       characteristic);
    cccd_handle = characteristic->value_handle + 1;
    printf("GATT-POC direct CCCD write handle=0x%04x value=0100\n", cccd_handle);
    status = gatt_client_write_characteristic_descriptor_using_descriptor_handle(notification_config_packet_handler,
                                                                                service_dump_con_handle, cccd_handle,
                                                                                sizeof(notify_cccd_value),
                                                                                notify_cccd_value);
    printf("GATT-POC direct CCCD write request status=0x%02x\n", status);
    if (status != ERROR_CODE_SUCCESS) {
        notify_config_index++;
        start_next_notification_config();
    }
}

static void start_next_characteristic_dump(void) {
    uint8_t status;
    gatt_client_service_t* service;

    if (characteristic_dump_index >= service_dump_count) {
        printf("GATT-POC characteristic dump complete\n");
        notify_config_index = 0;
        start_next_notification_config();
        return;
    }

    service = &service_dump_services[characteristic_dump_index];
    printf("GATT-POC discover characteristics service_index=%u range=0x%04x-0x%04x ",
           characteristic_dump_index, service->start_group_handle, service->end_group_handle);
    if (service->uuid16 != 0) {
        printf("uuid16=0x%04x\n", service->uuid16);
    } else {
        printf("uuid128=");
        print_uuid128(service->uuid128);
        printf("\n");
    }

    status = gatt_client_discover_characteristics_for_service(characteristic_dump_packet_handler,
                                                             service_dump_con_handle, service);
    printf("GATT-POC discover characteristics request status=0x%02x\n", status);
    if (status != ERROR_CODE_SUCCESS) {
        characteristic_dump_index++;
        start_next_characteristic_dump();
    }
}

static void characteristic_dump_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    gatt_client_characteristic_t characteristic;

    ARG_UNUSED(packet_type);
    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            printf("GATT-POC characteristic service_index=%u start=0x%04x value=0x%04x end=0x%04x props=0x%02x ",
                   characteristic_dump_index, characteristic.start_handle, characteristic.value_handle,
                   characteristic.end_handle, characteristic.properties);
            if (characteristic.uuid16 != 0) {
                printf("uuid16=0x%04x\n", characteristic.uuid16);
            } else {
                printf("uuid128=");
                print_uuid128(characteristic.uuid128);
                printf("\n");
            }
            if (characteristic.value_handle == 0x002e && (characteristic.properties & 0x10) != 0 &&
                notify_dump_count < GATT_POC_MAX_NOTIFY_CHARACTERISTICS) {
                notify_dump_characteristics[notify_dump_count++] = characteristic;
                printf("GATT-POC input notify candidate index=%u value=0x%04x props=0x%02x\n",
                       notify_dump_count - 1, characteristic.value_handle, characteristic.properties);
            }
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            printf("GATT-POC characteristic query complete service_index=%u att=0x%02x\n", characteristic_dump_index,
                   gatt_event_query_complete_get_att_status(packet));
            characteristic_dump_index++;
            start_next_characteristic_dump();
            break;

        default:
            break;
    }
}

static void service_dump_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    gatt_client_service_t service;

    ARG_UNUSED(packet_type);
    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            gatt_event_service_query_result_get_service(packet, &service);
            if (service_dump_count < GATT_POC_MAX_SERVICES)
                service_dump_services[service_dump_count++] = service;
            if (service.uuid16 != 0) {
                printf("GATT-POC service range=0x%04x-0x%04x uuid16=0x%04x\n", service.start_group_handle,
                       service.end_group_handle, service.uuid16);
            } else {
                printf("GATT-POC service range=0x%04x-0x%04x uuid128=", service.start_group_handle,
                       service.end_group_handle);
                print_uuid128(service.uuid128);
                printf("\n");
            }
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            printf("GATT-POC service dump complete att=0x%02x\n", gatt_event_query_complete_get_att_status(packet));
            characteristic_dump_index = 0;
            start_next_characteristic_dump();
            break;

        default:
            break;
    }
}

static void service_dump_timer_callback(btstack_timer_source_t* ts) {
    uint8_t status;

    ARG_UNUSED(ts);

    if (service_dump_con_handle == UNI_BT_CONN_HANDLE_INVALID)
        return;

    service_dump_count = 0;
    characteristic_dump_index = 0;
    notify_dump_count = 0;
    notify_config_index = 0;
    notification_dump_count = 0;
    notification_dump_suppressed_count = 0;
    notification_dump_last_log_ms = 0;
    printf("GATT-POC discover all primary services con_handle=0x%04x\n", service_dump_con_handle);
    status = gatt_client_discover_primary_services(service_dump_packet_handler, service_dump_con_handle);
    printf("GATT-POC discover all primary services request status=0x%02x\n", status);
}

static void schedule_service_dump(hci_con_handle_t con_handle) {
    service_dump_con_handle = con_handle;
    btstack_run_loop_set_timer_context(&service_dump_timer, NULL);
    btstack_run_loop_set_timer_handler(&service_dump_timer, &service_dump_timer_callback);
    btstack_run_loop_set_timer(&service_dump_timer, 250);
    btstack_run_loop_add_timer(&service_dump_timer);
}

void uni_bt_le_switch2_pro_poc_disconnect(void) {
    hci_con_handle_t con_handle = service_dump_con_handle;

    if (con_handle == UNI_BT_CONN_HANDLE_INVALID)
        return;

    btstack_run_loop_remove_timer(&service_dump_timer);
    service_dump_con_handle = UNI_BT_CONN_HANDLE_INVALID;
    notify_dump_count = 0;
    notify_config_index = 0;
    printf("GATT-POC disconnect con_handle=0x%04x\n", con_handle);
    if (gap_get_connection_type(con_handle) != GAP_CONNECTION_INVALID)
        gap_disconnect(con_handle);
}

/**
 * Connect to remote device but set timer for timeout
 */
static void hog_connect(bd_addr_t addr, bd_addr_type_t addr_type) {
    // Stop scan, otherwise it will be able to connect.
    // Happens in ESP32, but not in libusb
    gap_stop_scan();
    logi("BLE scan -> 0\n");

    gap_connect(addr, addr_type);
}

static void resume_scanning_hint(void) {
    // Resume scanning, only if it was scanning before connecting
    if (is_scanning) {
        gap_start_scan();
        logi("BLE scan -> 1\n");
    }
}

static void hog_disconnect(hci_con_handle_t con_handle) {
    // MUST not call uni_hid_device_disconnect(), called from it.
    uint8_t status;
    uni_hid_device_t* device;

    device = uni_hid_device_get_instance_for_connection_handle(con_handle);
    if (device) {
        status = hids_client_disconnect(device->hids_cid);
        if (status != ERROR_CODE_SUCCESS) {
            loge("Failed to disconnect HIDS client for hids_cid=%d, status=%d\n", device->hids_cid, status);
        }
        // gap_delete_bonding(0, device->conn.btaddr);
    }

    if (gap_get_connection_type(con_handle) != GAP_CONNECTION_INVALID)
        gap_disconnect(con_handle);

    resume_scanning_hint();
}

static bool start_hid_service_connect(hci_con_handle_t con_handle, const char* reason) {
    uint8_t status;
    uint16_t hids_cid;
    uni_hid_device_t* device;

    device = uni_hid_device_get_instance_for_connection_handle(con_handle);
    if (!device) {
        loge("Cannot start HID service connect (%s): no device for con_handle %#x\n", reason, con_handle);
        return false;
    }

    logi("Search for HID service, con_handle: %#x (%s)\n", con_handle, reason);
    status = hids_client_connect(con_handle, uni_hids_client_packet_handler, HID_PROTOCOL_MODE_REPORT, &hids_cid);
    if (status == ERROR_CODE_COMMAND_DISALLOWED) {
        logi("HID client connection failed with COMMAND_DISALLOWED, ignoring\n");
    }
    if (status != ERROR_CODE_SUCCESS) {
        logi("HID client connection failed, status=%#x (%s)\n", status, reason);
        hog_disconnect(con_handle);
        return false;
    }

    logi("Using hids_cid=%d (%s)\n", hids_cid, reason);
    device->hids_cid = hids_cid;
    return true;
}

static bool name_contains_token(const char* name, const char* token) {
    while (*name) {
        const char* candidate = name;
        const char* expected = token;
        while (*candidate && *expected && tolower((unsigned char)*candidate) == tolower((unsigned char)*expected)) {
            candidate++;
            expected++;
        }
        if (*expected == '\0')
            return true;
        name++;
    }
    return false;
}

static bool is_likely_gamepad_name(const char* name) {
    return name_contains_token(name, "switch") || name_contains_token(name, "joy-con") ||
           name_contains_token(name, "pro controller") || name_contains_token(name, "nintendo");
}

static bool is_switch2_pro_poc_address(bd_addr_t addr) {
    return strcmp(bd_addr_to_str(addr), "A4:C1:E8:50:BC:2B") == 0;
}

static void store_advertisement_debug(bd_addr_t addr,
                                      bd_addr_type_t addr_type,
                                      uint8_t event_type,
                                      uint8_t rssi,
                                      uint8_t data_len,
                                      uint16_t appearance,
                                      const char* name,
                                      bool has_hid_service,
                                      bool has_generic_access_service,
                                      bool matched_relaxed_filter) {
    const char* address = bd_addr_to_str(addr);
    for (int i = 0; i < advertisement_debug_count; i++) {
        if (strcmp(advertisement_debug[i].address, address) == 0) {
            uni_bt_le_advertisement_debug_t* existing = &advertisement_debug[i];
            snprintf(existing->name, sizeof(existing->name), "%s", name);
            existing->appearance = appearance;
            existing->address_type = addr_type;
            existing->event_type = event_type;
            existing->rssi = rssi;
            existing->data_len = data_len;
            existing->has_hid_service = has_hid_service;
            existing->has_generic_access_service = has_generic_access_service;
            existing->matched_relaxed_filter = matched_relaxed_filter;
            return;
        }
    }

    uni_bt_le_advertisement_debug_t* entry = &advertisement_debug[advertisement_debug_next];
    snprintf(entry->address, sizeof(entry->address), "%s", address);
    snprintf(entry->name, sizeof(entry->name), "%s", name);
    entry->appearance = appearance;
    entry->address_type = addr_type;
    entry->event_type = event_type;
    entry->rssi = rssi;
    entry->data_len = data_len;
    entry->has_hid_service = has_hid_service;
    entry->has_generic_access_service = has_generic_access_service;
    entry->matched_relaxed_filter = matched_relaxed_filter;

    advertisement_debug_next = (advertisement_debug_next + 1) % ARRAY_SIZE(advertisement_debug);
    if (advertisement_debug_count < ARRAY_SIZE(advertisement_debug))
        advertisement_debug_count++;
}

static void get_advertisement_data(const uint8_t* adv_data,
                                   uint8_t adv_size,
                                   uint16_t* appearance,
                                   char* name,
                                   bool* has_hid_service,
                                   bool* has_generic_access_service) {
    ad_context_t context;

    for (ad_iterator_init(&context, adv_size, (uint8_t*)adv_data); ad_iterator_has_more(&context);
         ad_iterator_next(&context)) {
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t size = ad_iterator_get_data_len(&context);
        const uint8_t* data = ad_iterator_get_data(&context);

        int i;
        // Assigned Numbers GAP

        switch (data_type) {
            case BLUETOOTH_DATA_TYPE_FLAGS:
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_16_BIT_SERVICE_SOLICITATION_UUIDS:
                if (ad_data_contains_uuid16(adv_size, adv_data, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE))
                    *has_hid_service = true;
                if (ad_data_contains_uuid16(adv_size, adv_data, 0x1800))
                    *has_generic_access_service = true;
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_32_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_32_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_32_BIT_SERVICE_SOLICITATION_UUIDS:
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_128_BIT_SERVICE_SOLICITATION_UUIDS:
                break;
            case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
                for (i = 0; i < size; i++) {
                    name[i] = data[i];
                }
                name[size] = 0;
                break;
            case BLUETOOTH_DATA_TYPE_TX_POWER_LEVEL:
                break;
            case BLUETOOTH_DATA_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE:
                break;
            case BLUETOOTH_DATA_TYPE_SERVICE_DATA:
                break;
            case BLUETOOTH_DATA_TYPE_PUBLIC_TARGET_ADDRESS:
            case BLUETOOTH_DATA_TYPE_RANDOM_TARGET_ADDRESS:
                break;
            case BLUETOOTH_DATA_TYPE_APPEARANCE:
                // https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
                *appearance = little_endian_read_16(data, 0);
                break;
            case BLUETOOTH_DATA_TYPE_ADVERTISING_INTERVAL:
                break;
            case BLUETOOTH_DATA_TYPE_3D_INFORMATION_DATA:
                break;
            case BLUETOOTH_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA:  // Manufacturer Specific Data
                break;
            case BLUETOOTH_DATA_TYPE_CLASS_OF_DEVICE:
                logi("class of device: %#x\n", little_endian_read_16(data, 0));
                break;
            case BLUETOOTH_DATA_TYPE_SIMPLE_PAIRING_HASH_C:
            case BLUETOOTH_DATA_TYPE_SIMPLE_PAIRING_RANDOMIZER_R:
            case BLUETOOTH_DATA_TYPE_DEVICE_ID:
                logi("device id: %#x\n", little_endian_read_16(data, 0));
                break;
            case BLUETOOTH_DATA_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS:
            case BLUETOOTH_DATA_TYPE_MESH_BEACON:
            case BLUETOOTH_DATA_TYPE_MESH_MESSAGE:
                // Safely ignore these messages
                break;
            case BLUETOOTH_DATA_TYPE_SECURITY_MANAGER_OUT_OF_BAND_FLAGS:
                // fall-through
            default:
                logi("Advertising Data Type 0x%2x not handled yet\n", data_type);
                break;
        }
    }
}

static void adv_event_get_data(const uint8_t* packet,
                               uint16_t* appearance,
                               char* name,
                               bool* has_hid_service,
                               bool* has_generic_access_service) {
    const uint8_t* ad_data;
    uint16_t ad_len;

    ad_data = gap_event_advertising_report_get_data(packet);
    ad_len = gap_event_advertising_report_get_data_length(packet);

    get_advertisement_data(ad_data, ad_len, appearance, name, has_hid_service, has_generic_access_service);
}

static void parse_report(const uint8_t* packet, uint16_t size) {
    uint16_t service_index;
    uint16_t hids_cid;
    uni_hid_device_t* device;
    const uint8_t* descriptor_data;
    uint16_t descriptor_len;
    const uint8_t* report_data;
    uint16_t report_len;

    ARG_UNUSED(size);

    service_index = gattservice_subevent_hid_report_get_service_index(packet);
    hids_cid = gattservice_subevent_hid_report_get_hids_cid(packet);
    device = uni_hid_device_get_instance_for_hids_cid(hids_cid);

    if (!device) {
        loge("BLE parser report: Invalid device for hids_cid=%d\n", hids_cid);
        return;
    }

    // FIXME: Copying the HID descriptor should be done at setup time since some device, like Xbox requires it
    // to set the correct parser.
    // But not clear how to get the "service_index" from setup
    if (device->hid_descriptor_len == 0) {
        descriptor_data = hids_client_descriptor_storage_get_descriptor_data(hids_cid, service_index);
        descriptor_len = hids_client_descriptor_storage_get_descriptor_len(hids_cid, service_index);

        uni_hid_device_set_hid_descriptor(device, descriptor_data, descriptor_len);
    }
    report_data = gattservice_subevent_hid_report_get_report(packet);
    report_len = gattservice_subevent_hid_report_get_report_len(packet);

    uni_hid_parse_input_report(device, report_data, report_len);
    uni_hid_device_process_controller(device);
}

static void uni_hids_client_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    uint8_t status;
    uint16_t hids_cid;
    uni_hid_device_t* device;
    uint8_t event_type;
    hid_protocol_mode_t protocol_mode;

    ARG_UNUSED(packet_type);
    ARG_UNUSED(channel);
    ARG_UNUSED(size);

#if 0
    // FIXME: Bug in BTStack??? This comparison fails because packet_type is HCI_EVENT_GATTSERVICE_META
    if (packet_type != HCI_EVENT_PACKET) {
        loge("uni_hids_client_packet_handler: unsupported packet type: %#x\n", packet_type);
        return;
    }
#endif

    event_type = hci_event_packet_get_type(packet);
    if (event_type != HCI_EVENT_GATTSERVICE_META) {
        loge("uni_hids_client_packet_handler: unsupported event type: %#x\n", event_type);
        return;
    }

    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
        case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED:
            status = gattservice_subevent_hid_service_connected_get_status(packet);
            logi("GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED, status=0x%02x\n", status);
            switch (status) {
                case ERROR_CODE_SUCCESS:
                    protocol_mode = gattservice_subevent_hid_service_connected_get_protocol_mode(packet);
                    logi("HID service client connected, found %d services, protocol_mode=%d\n",
                         gattservice_subevent_hid_service_connected_get_num_instances(packet), protocol_mode);

                    // XXX TODO: store device as bonded
                    hids_cid = gattservice_subevent_hid_service_connected_get_hids_cid(packet);
                    device = uni_hid_device_get_instance_for_hids_cid(hids_cid);
                    if (!device) {
                        loge("Hids Cid: Could not find valid device for hids_cid=%d\n", hids_cid);
                        break;
                    }
#if 0
                    status = hids_client_enable_notifications(hids_cid);
                    if (status != ERROR_CODE_SUCCESS)
                        logi("Failed to enable client notifications for hids_cid=%d, status=%#x\n", hids_cid, status);
                    else
                        logi("Client notifications enabled for for hids_cid=%d\n", hids_cid);
#endif

                    uni_hid_device_guess_controller_type_from_pid_vid(device);
                    uni_hid_device_connect(device);
                    uni_hid_device_set_ready(device);

                    resume_scanning_hint();
                    break;
                default:
                    loge("HID service client connection failed, err 0x%02x.\n", status);
                    hids_cid = gattservice_subevent_hid_service_connected_get_hids_cid(packet);
                    device = uni_hid_device_get_instance_for_hids_cid(hids_cid);
                    if (device != NULL && is_switch2_pro_poc_address(device->conn.btaddr)) {
                        printf("GATT-POC scheduling service dump after HIDS failure status=0x%02x\n", status);
                        schedule_service_dump(device->conn.handle);
                    }
                    break;
            }
            break;

        case GATTSERVICE_SUBEVENT_HID_REPORT:
            logd("GATTSERVICE_SUBEVENT_HID_REPORT\n");
            parse_report(packet, size);
            break;
        case GATTSERVICE_SUBEVENT_HID_INFORMATION:
            logi(
                "Hid Information: service index %d, USB HID 0x%02X, country code %d, remote wake %d, normally "
                "connectable %d\n",
                gattservice_subevent_hid_information_get_service_index(packet),
                gattservice_subevent_hid_information_get_base_usb_hid_version(packet),
                gattservice_subevent_hid_information_get_country_code(packet),
                gattservice_subevent_hid_information_get_remote_wake(packet),
                gattservice_subevent_hid_information_get_normally_connectable(packet));
            break;

        case GATTSERVICE_SUBEVENT_HID_PROTOCOL_MODE:
            logi("Protocol Mode: service index %d, mode 0x%02X (Boot mode: 0x%02X, Report mode 0x%02X)\n",
                 gattservice_subevent_hid_protocol_mode_get_service_index(packet),
                 gattservice_subevent_hid_protocol_mode_get_protocol_mode(packet), HID_PROTOCOL_MODE_BOOT,
                 HID_PROTOCOL_MODE_REPORT);
            break;

        case GATTSERVICE_SUBEVENT_HID_SERVICE_REPORTS_NOTIFICATION:
            if (gattservice_subevent_hid_service_reports_notification_get_configuration(packet) == 0) {
                logi("Reports disabled\n");
            } else {
                logi("Reports enabled\n");
            }
            break;
#if 0  // Does not compile on Pico SDK 1.5.1. Enable it only if needed.
        case GATTSERVICE_SUBEVENT_HID_REPORT_WRITTEN:
            // Called when a client a hid report was written.
            // E.g.: "set rumble" was sent to the gamepad.
            // TODO: Inform the device that it is ready to write another hid report?
            break;
#endif
        default:
            logi("Unsupported gatt client event: 0x%02x\n", hci_event_gattservice_meta_get_subevent_code(packet));
            break;
    }
}

static void uni_device_information_packet_handler(uint8_t packet_type,
                                                  uint16_t channel,
                                                  uint8_t* packet,
                                                  uint16_t size) {
    uint8_t code;
    uint8_t status;
    uint8_t att_status;
    hci_con_handle_t con_handle;
    uni_hid_device_t* device;
    uint8_t event_type;

    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        loge("uni_device_information_packet_handler: unsupported packet type: %#x\n", packet_type);
        return;
    }

    event_type = hci_event_packet_get_type(packet);
    if (event_type != HCI_EVENT_GATTSERVICE_META) {
        loge("uni_device_information_packet_handler: unsupported event type: %#x\n", event_type);
        return;
    }

    code = hci_event_gattservice_meta_get_subevent_code(packet);
    switch (code) {
        case GATTSERVICE_SUBEVENT_SCAN_PARAMETERS_SERVICE_CONNECTED:
            logi("PnP ID: vendor source ID 0x%02X, vendor ID 0x%02X, product ID 0x%02X, product version 0x%02X\n",
                 gattservice_subevent_device_information_pnp_id_get_vendor_source_id(packet),
                 gattservice_subevent_device_information_pnp_id_get_vendor_id(packet),
                 gattservice_subevent_device_information_pnp_id_get_product_id(packet),
                 gattservice_subevent_device_information_pnp_id_get_product_version(packet));
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE:
            status = gattservice_subevent_device_information_done_get_att_status(packet);
            con_handle = gattservice_subevent_device_information_done_get_con_handle(packet);
            switch (status) {
                case ERROR_CODE_SUCCESS:
                    logi("Device Information service found\n");
                    device = uni_hid_device_get_instance_for_connection_handle(con_handle);
                    if (!device) {
                        loge("Invalid device for in GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE");
                        break;
                    }

                    // Continue - query primary services.
                    start_hid_service_connect(con_handle, "device information done");
                    break;
                default:
                    logi("Device Information service client connection failed, error=%#x.\n", status);
                    hog_disconnect(con_handle);
                    break;
            }
            break;
        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_MANUFACTURER_NAME:
            att_status = gattservice_subevent_device_information_manufacturer_name_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Manufacturer Name read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Manufacturer Name: %s\n",
                     gattservice_subevent_device_information_manufacturer_name_get_value(packet));
            }
            break;
        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_MODEL_NUMBER:
            att_status = gattservice_subevent_device_information_model_number_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Model Number read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Model Number:     %s\n", gattservice_subevent_device_information_model_number_get_value(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_SERIAL_NUMBER:
            att_status = gattservice_subevent_device_information_serial_number_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Serial Number read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Serial Number:    %s\n", gattservice_subevent_device_information_serial_number_get_value(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_HARDWARE_REVISION:
            att_status = gattservice_subevent_device_information_hardware_revision_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Hardware Revision read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Hardware Revision: %s\n",
                     gattservice_subevent_device_information_hardware_revision_get_value(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_FIRMWARE_REVISION:
            att_status = gattservice_subevent_device_information_firmware_revision_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Firmware Revision read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Firmware Revision: %s\n",
                     gattservice_subevent_device_information_firmware_revision_get_value(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_SOFTWARE_REVISION:
            att_status = gattservice_subevent_device_information_software_revision_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("Software Revision read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Software Revision: %s\n",
                     gattservice_subevent_device_information_software_revision_get_value(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_SYSTEM_ID:
            att_status = gattservice_subevent_device_information_system_id_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("System ID read failed, ATT Error 0x%02x\n", att_status);
            } else {
                uint32_t manufacturer_identifier_low =
                    gattservice_subevent_device_information_system_id_get_manufacturer_id_low(packet);
                uint8_t manufacturer_identifier_high =
                    gattservice_subevent_device_information_system_id_get_manufacturer_id_high(packet);

                logi("Manufacturer ID:  0x%02x%08x\n", manufacturer_identifier_high, manufacturer_identifier_low);
                logi("Organizationally Unique ID:  0x%06x\n",
                     gattservice_subevent_device_information_system_id_get_organizationally_unique_id(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_IEEE_REGULATORY_CERTIFICATION:
            att_status = gattservice_subevent_device_information_ieee_regulatory_certification_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("IEEE Regulatory Certification read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("value_a:          0x%04x\n",
                     gattservice_subevent_device_information_ieee_regulatory_certification_get_value_a(packet));
                logi("value_b:          0x%04x\n",
                     gattservice_subevent_device_information_ieee_regulatory_certification_get_value_b(packet));
            }
            break;

        case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID:
            con_handle = gattservice_subevent_device_information_pnp_id_get_con_handle(packet);
            device = uni_hid_device_get_instance_for_connection_handle(con_handle);
            if (!device) {
                loge("Invalid device for in GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID");
                break;
            }

            att_status = gattservice_subevent_device_information_pnp_id_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS) {
                logi("PNP ID read failed, ATT Error 0x%02x\n", att_status);
            } else {
                logi("Vendor Source ID: 0x%02x\n",
                     gattservice_subevent_device_information_pnp_id_get_vendor_source_id(packet));
                logi("Vendor  ID:       0x%04x\n",
                     gattservice_subevent_device_information_pnp_id_get_vendor_id(packet));
                logi("Product ID:       0x%04x\n",
                     gattservice_subevent_device_information_pnp_id_get_product_id(packet));
                logi("Product Version:  0x%04x\n",
                     gattservice_subevent_device_information_pnp_id_get_product_version(packet));
            }
            uni_hid_device_set_vendor_id(device, gattservice_subevent_device_information_pnp_id_get_vendor_id(packet));
            uni_hid_device_set_product_id(device,
                                          gattservice_subevent_device_information_pnp_id_get_product_id(packet));

            break;

        default:
            logi("Unknown gattservice meta subevent code: %#x\n", code);
            break;
    }
}

/* HCI packet handler
 *
 * text The SM packet handler receives Security Manager Events required for
 * pairing. It also receives events generated during Identity Resolving see
 * Listing SMPacketHandler.
 */
static void uni_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    bd_addr_t addr;
    uni_hid_device_t* device;
    uint8_t status;
    uint8_t type;
    hci_con_handle_t con_handle = UNI_BT_CONN_HANDLE_INVALID;
    bool request_device_information_query = false;

    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        loge("uni_sm_packet_handler: unsupported packet type: %#x\n", packet_type);
        return;
    }

    type = hci_event_packet_get_type(packet);
    switch (type) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            logi("Just works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            logi("Confirming numeric comparison: %" PRIu32 "\n",
                 sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            logi("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_STARTED:
            logi("SM_EVENT_IDENTITY_RESOLVING_STARTED\n");
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED:
            sm_event_identity_created_get_address(packet, addr);
            logi("Identity resolving failed for %s\n\n", bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
            sm_event_identity_resolving_succeeded_get_identity_address(packet, addr);
            logi("Identity resolved: type %u address %s\n",
                 sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_PAIRING_STARTED:
            logi("SM_EVENT_PAIRING_STARTED\n");
            break;
        case SM_EVENT_IDENTITY_CREATED:
            sm_event_identity_created_get_identity_address(packet, addr);
            logi("Identity created: type %u address %s\n", sm_event_identity_created_get_identity_addr_type(packet),
                 bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            logi("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                 sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            con_handle = sm_event_reencryption_complete_get_handle(packet);
            switch (sm_event_reencryption_complete_get_status(packet)) {
                case ERROR_CODE_SUCCESS:
                    logi("Re-encryption complete, success\n");
                    request_device_information_query = true;
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    logi("Re-encryption failed, timeout\n");
                    hog_disconnect(con_handle);
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    logi("Re-encryption failed, disconnected\n");
                    hog_disconnect(con_handle);
                    break;
                case ERROR_CODE_PIN_OR_KEY_MISSING:
                    logi("Re-encryption failed, bonding information missing\n\n");
                    logi("Assuming remote lost bonding information\n");
                    logi("Deleting local bonding information and start new pairing...\n");
                    sm_event_reencryption_complete_get_address(packet, addr);
                    type = sm_event_reencryption_started_get_addr_type(packet);
                    gap_delete_bonding(type, addr);
                    sm_request_pairing(sm_event_reencryption_complete_get_handle(packet));
                    break;
                default:
                    break;
            }
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            sm_event_pairing_complete_get_address(packet, addr);
            device = uni_hid_device_get_instance_for_address(addr);
            con_handle = sm_event_pairing_complete_get_handle(packet);
            if (!device) {
                loge("SM_EVENT_PAIRING_COMPLETE: Invalid device for addr %s\n", bd_addr_to_str(addr));
                hog_disconnect(con_handle);
                break;
            }

            status = sm_event_pairing_complete_get_status(packet);
            switch (status) {
                case ERROR_CODE_SUCCESS:
                    logi("Pairing complete, success\n");
                    request_device_information_query = true;
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    logi("Pairing failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    logi("Pairing failed, disconnected\n");
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    logi("Pairing failed, reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                    break;
                default:
                    loge("Unknown paring status: %#x\n", status);
                    break;
            }

            // TODO: Double check
            // Do not disconnect. Sometimes it appears as "failure" although
            // the connection as Ok (???)
            // hog_disconnect(device->conn.handle);
            break;

        default:
            loge("Unknown SM packet type: %#x\n", type);
            break;
    }

    if (request_device_information_query) {
        if (con_handle == UNI_BT_CONN_HANDLE_INVALID) {
            // Should not happen.
            loge("Error: Invalid conn_handle: %d\n", con_handle);
            return;
        }
        logi("Requesting device information\n");
        status = device_information_service_client_query(con_handle, uni_device_information_packet_handler);
        if (status != ERROR_CODE_SUCCESS) {
            loge("Failed to set device information client: %#x\n", status);
        }
    }
}

void uni_bt_le_on_hci_event_le_meta(const uint8_t* packet, uint16_t size) {
    uni_hid_device_t* device;
    hci_con_handle_t con_handle;
    bd_addr_t event_addr;
    uint8_t subevent;

    ARG_UNUSED(size);

    subevent = hci_event_le_meta_get_subevent_code(packet);

    switch (subevent) {
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            hci_subevent_le_connection_complete_get_peer_address(packet, event_addr);
            device = uni_hid_device_get_instance_for_address(event_addr);
            if (!device) {
                loge("uni_bt_le_on_connection_complete: Device not found for addr: %s\n", bd_addr_to_str(event_addr));
                break;
            }
            con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
            logi("Using con_handle: %#x\n", con_handle);

            uni_hid_device_set_connection_handle(device, con_handle);
            if (is_switch2_pro_poc_address(event_addr)) {
                logi("Switch 2 Pro PoC target connected; skipping Bluepad32 HIDS path and dumping proprietary GATT\n");
                btstack_run_loop_remove_timer(&device->connection_timer);
                schedule_service_dump(con_handle);
                break;
            }

            sm_request_pairing(con_handle);

            // Resume scanning
            // gap_start_scan();
            break;

        case HCI_SUBEVENT_LE_ADVERTISING_REPORT:
            // Safely ignore it, we handle the GAP advertising report instead
            break;

        default:
            logd("Unsupported LE_META sub-event: %#x\n", subevent);
            break;
    }
}

void uni_bt_le_on_hci_event_encryption_change(const uint8_t* packet, uint16_t size) {
    uni_hid_device_t* device;
    hci_con_handle_t con_handle;

    ARG_UNUSED(size);

    // Might be called from BR/EDR connections.
    // Only handle BLE in this function.
    con_handle = hci_event_encryption_change_get_connection_handle(packet);
    if (gap_get_connection_type(con_handle) != GAP_CONNECTION_LE)
        return;

    device = uni_hid_device_get_instance_for_connection_handle(con_handle);
    if (!device) {
        loge("uni_bt_le_on_encryption_change: Device not found for connection handle: 0x%04x\n", con_handle);
        return;
    }
    // This event is also triggered by Classic, and might crash the stack.
    // Real case: Connect a Wii, disconnect it, and try re-connection
    if (device->conn.protocol != UNI_BT_CONN_PROTOCOL_BLE)
        // Abort on non BLE connections
        return;

    logi("Connection encrypted: %u\n", hci_event_encryption_change_get_encryption_enabled(packet));
    if (hci_event_encryption_change_get_encryption_enabled(packet) == 0) {
        logi("Encryption failed -> abort\n");
        hog_disconnect(con_handle);
    }
}

void uni_bt_le_on_gap_event_advertising_report(const uint8_t* packet, uint16_t size) {
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint16_t appearance;
    uint16_t cod;
    uint8_t rssi;
    char name[64];
    bool has_hid_service;
    bool has_generic_access_service;
    bool matched_relaxed_filter;

    appearance = 0;
    name[0] = 0;
    has_hid_service = false;
    has_generic_access_service = false;
    matched_relaxed_filter = false;

    ARG_UNUSED(size);

    gap_event_advertising_report_get_address(packet, addr);
    if (uni_hid_device_get_instance_for_address(addr)) {
        // Ignore, address already found
        return;
    }

    adv_event_get_data(packet, &appearance, name, &has_hid_service, &has_generic_access_service);

    addr_type = gap_event_advertising_report_get_address_type(packet);
    rssi = gap_event_advertising_report_get_rssi(packet);
    uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
    uint8_t data_len = gap_event_advertising_report_get_data_length(packet);

    if (appearance != UNI_BT_HID_APPEARANCE_GAMEPAD && appearance != UNI_BT_HID_APPEARANCE_JOYSTICK &&
        appearance != UNI_BT_HID_APPEARANCE_MOUSE && appearance != UNI_BT_HID_APPEARANCE_KEYBOARD) {
        if (has_hid_service || is_likely_gamepad_name(name) || is_switch2_pro_poc_address(addr)) {
            logi("BLE HID-like device found without gamepad appearance: address %s, name '%s', appearance %#x, "
                 "hid_service=%d\n",
                 bd_addr_to_str(addr), name, appearance, has_hid_service);
            appearance = UNI_BT_HID_APPEARANCE_GAMEPAD;
            matched_relaxed_filter = true;
        } else {
            // Don't log it. There too many devices advertising themselves.
            if (appearance != 0 || strlen(name) != 0)
                logd("Not a HID controller, appearance: %#x, name =%s\n", appearance, name);
            store_advertisement_debug(addr, addr_type, event_type, rssi, data_len, appearance, name, has_hid_service,
                                      has_generic_access_service, false);
            return;
        }
    }

    switch (appearance) {
        case UNI_BT_HID_APPEARANCE_MOUSE:
            cod = UNI_BT_COD_MAJOR_PERIPHERAL | UNI_BT_COD_MINOR_MICE;
            break;
        case UNI_BT_HID_APPEARANCE_JOYSTICK:
            cod = UNI_BT_COD_MAJOR_PERIPHERAL | UNI_BT_COD_MINOR_JOYSTICK;
            break;
        case UNI_BT_HID_APPEARANCE_GAMEPAD:
            cod = UNI_BT_COD_MAJOR_PERIPHERAL | UNI_BT_COD_MINOR_GAMEPAD;
            break;
        case UNI_BT_HID_APPEARANCE_KEYBOARD:
            cod = UNI_BT_COD_MAJOR_PERIPHERAL | UNI_BT_COD_MINOR_KEYBOARD;
            break;
        default:
            cod = 0;
            break;
    }

    store_advertisement_debug(addr, addr_type, event_type, rssi, data_len, appearance, name, has_hid_service,
                              has_generic_access_service, matched_relaxed_filter);

    logi("Device found: %s (%s)", bd_addr_to_str(addr), addr_type == 0 ? "public" : "random");
    logi(", appearance %#x / COD %#x", appearance, cod);
    logi(", rssi %u dBm", rssi);
    logi(", name '%s'\n", name);

    if (uni_hid_device_on_device_discovered(addr, name, cod, rssi) != UNI_ERROR_SUCCESS)
        return;

    uni_hid_device_t* d = uni_hid_device_create(addr);
    if (!d) {
        loge("Error: no more available device slots\n");
        return;
    }

    // FIXME: Using CODs to make it compatible with legacy BR/EDR code.
    uni_hid_device_set_cod(d, cod);
    uni_hid_device_set_name(d, name);
    uni_bt_conn_set_protocol(&d->conn, UNI_BT_CONN_PROTOCOL_BLE);
    uni_bt_conn_set_state(&d->conn, UNI_BT_CONN_STATE_DEVICE_DISCOVERED);
    d->conn.rssi = rssi;

    hog_connect(addr, addr_type);
}

void uni_bt_le_on_hci_disconnection_complete(uint16_t channel, const uint8_t* packet, uint16_t size) {
    hci_con_handle_t con_handle;

    ARG_UNUSED(channel);
    ARG_UNUSED(size);

    con_handle = hci_event_disconnection_complete_get_connection_handle(packet);
    if (con_handle == service_dump_con_handle) {
        service_dump_con_handle = UNI_BT_CONN_HANDLE_INVALID;
        notify_dump_count = 0;
        notify_config_index = 0;
    }

    resume_scanning_hint();
}

void uni_bt_le_list_bonded_keys(void) {
    bd_addr_t entry_address;
    int i;

    if (!ble_enabled)
        return;

    logi("Bluetooth LE keys:\n");

    for (i = 0; i < le_device_db_max_count(); i++) {
        int entry_address_type = (int)BD_ADDR_TYPE_UNKNOWN;
        le_device_db_info(i, &entry_address_type, entry_address, NULL);

        // skip unused entries
        if (entry_address_type == (int)BD_ADDR_TYPE_UNKNOWN)
            continue;

        logi("%s - type %u\n", bd_addr_to_str(entry_address), (int)entry_address_type);
    }
    logi(".\n");
}

void uni_bt_le_delete_bonded_keys(void) {
    bd_addr_t entry_address;
    int i;

    if (!ble_enabled)
        return;

    logi("Deleting stored BLE link keys:\n");

    for (i = 0; i < le_device_db_max_count(); i++) {
        int entry_address_type = (int)BD_ADDR_TYPE_UNKNOWN;
        le_device_db_info(i, &entry_address_type, entry_address, NULL);

        // skip unused entries
        if (entry_address_type == (int)BD_ADDR_TYPE_UNKNOWN)
            continue;

        logi("%s - type %u\n", bd_addr_to_str(entry_address), (int)entry_address_type);
        gap_delete_bonding((bd_addr_type_t)entry_address_type, entry_address);
    }
    logi(".\n");
}

void uni_bt_le_setup(void) {
    // register for events from Security Manager
    sm_event_callback_registration.callback = &uni_sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // Setup LE device db
    le_device_db_init();

    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    // TL;DR:
    // Enable Secure connection, disable bonding

    // Legacy paring, Just Works in ESP32
    // - Stadia: Ok
    // - MS mouse: Ok
    // - Xbox 3 buttons: flaky, fails to connect or connects
    // - Xbox 2 buttons: flaky, fails to connect or connects
    // sm_set_authentication_requirements(0);

    // sm_set_authentication_requirements(SM_AUTHREQ_BONDING);

    // Secure connection + NO bonding in ESP32:
    // - Stadia: Ok
    // - MS mouse: Ok
    // - Xbox 3 buttons: Ok
    // - Xbox 2 buttons: fails to connect
    // Switch 2 Pro PoC: the controller is discovered, but pairing currently
    // fails before the HID service is opened. Try LE Secure Connections
    // without storing a bond on either side.
    sm_set_secure_connections_only_mode(true);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION);

    // Secure connection + bonding in ESP32:
    // - Stadia: Ok
    // - MS mouse: Ok... but disconnects after 10 seconds
    // - Xbox 3 buttons: fails to connect
    // - Xbox 2 buttons: fails to connect
    // sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // libusb works with mostly any configuration

    gatt_client_init();
    hids_client_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    // FIXME: this is an empty function and PicoW toolchain is removing empty function (?)
    // scan_parameters_service_client_init();
    device_information_service_client_init();

    // Switch 2 Pro appears to advertise only Generic Access (0x1800) in the
    // initial packet. Active scan asks for scan-response data, where BLE HID
    // controllers often publish name and service details.
    gap_set_scan_parameters(1 /* type: active */, 48 /* interval */, 48 /* window */);
}

void uni_bt_le_scan_start(void) {
    if (!ble_enabled)
        return;

    gap_start_scan();
    logi("BLE scan -> 1\n");
    is_scanning = true;
}

void uni_bt_le_scan_stop(void) {
    if (!ble_enabled)
        return;

    gap_stop_scan();
    logi("BLE scan -> 0\n");
    is_scanning = false;
}

void uni_bt_le_disconnect(uni_hid_device_t* d) {
    // if (gap_get_connection_type(conn->handle) == GAP_CONNECTION_INVALID)
    //     return;
    hog_disconnect(d->conn.handle);
}

void uni_bt_le_set_enabled(bool enabled) {
    // Called from different Task. Don't call BTstack functions.
    uni_property_value_t val;

    val.u8 = enabled;
    uni_property_set(UNI_PROPERTY_IDX_BLE_ENABLED, val);

    ble_enabled = enabled;
}

bool uni_bt_le_is_enabled() {
    // Expensive call. Avoid calling it from this same file.
    // Called from "uni_bt_setup"
    uni_property_value_t val;

    val = uni_property_get(UNI_PROPERTY_IDX_BLE_ENABLED);

    ble_enabled = val.u8;

    return ble_enabled;
}

int uni_bt_le_get_advertisement_debug(uni_bt_le_advertisement_debug_t* out_entries, int max_entries) {
    if (!out_entries || max_entries <= 0)
        return 0;

    int total = btstack_min((int)advertisement_debug_count, max_entries);
    uint8_t start = (advertisement_debug_next + ARRAY_SIZE(advertisement_debug) - advertisement_debug_count) %
                    ARRAY_SIZE(advertisement_debug);
    for (int i = 0; i < total; i++) {
        out_entries[i] = advertisement_debug[(start + i) % ARRAY_SIZE(advertisement_debug)];
    }
    return total;
}
