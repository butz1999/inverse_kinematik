// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Ricardo Quesada
// http://retro.moe/unijoysticle2

#ifndef UNI_BT_LE_H
#define UNI_BT_LE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

#include <btstack.h>
#include <btstack_config.h>

#include "bt/uni_bt_conn.h"
#include "uni_hid_device.h"

typedef struct {
    char address[18];
    char name[64];
    uint16_t appearance;
    uint8_t address_type;
    uint8_t event_type;
    uint8_t rssi;
    uint8_t data_len;
    bool has_hid_service;
    bool has_generic_access_service;
    bool matched_relaxed_filter;
} uni_bt_le_advertisement_debug_t;

void uni_bt_le_on_hci_event_le_meta(const uint8_t* packet, uint16_t size);
void uni_bt_le_on_hci_event_encryption_change(const uint8_t* packet, uint16_t size);
void uni_bt_le_on_gap_event_advertising_report(const uint8_t* packet, uint16_t size);
void uni_bt_le_on_hci_disconnection_complete(uint16_t channel, const uint8_t* packet, uint16_t size);

void uni_bt_le_scan_start(void);
void uni_bt_le_scan_stop(void);

// Called from uni_hid_device_disconnect()
void uni_bt_le_disconnect(uni_hid_device_t* d);

void uni_bt_le_list_bonded_keys(void);
void uni_bt_le_delete_bonded_keys(void);
void uni_bt_le_setup(void);

void uni_bt_le_set_enabled(bool enabled);
bool uni_bt_le_is_enabled(void);
int uni_bt_le_get_advertisement_debug(uni_bt_le_advertisement_debug_t* out_entries, int max_entries);
void uni_bt_le_switch2_pro_poc_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif  // UNI_BT_LE_H
