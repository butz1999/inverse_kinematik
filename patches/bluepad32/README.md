# Bluepad32-Patches

Die Patch-Serie basiert auf Bluepad32 `4.2.0` am Commit `6efa7123fe8badf5a40ad1205743a80b31c00ea4`.

`scripts/bootstrap.sh` initialisiert das Submodule und wendet die Patches idempotent an. Die gepatchte Komponente wird über den relativen Symlink `components/bluepad32` in den bestehenden ESP-IDF-Build eingebunden.

Der Patch `0001-switch2-pro-ble-support.patch` enthält nur die produktiv benötigte Switch-2-Pro-BLE-Anbindung. Bring-up-Dumps und Advertisement-Debugging gehören nicht zur Patch-Serie.

Neue Patches werden gegen den gepinnten Submodule-Commit erzeugt und mit einer fortlaufenden Nummer abgelegt. Vor einem Upstream-Update wird die Patch-Serie auf den neuen Commit übertragen und erneut gegen Hardware sowie native Tests verifiziert.
