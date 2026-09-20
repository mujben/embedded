#pragma once

#include <stdint.h>
#include "host/ble_hs.h"

void ble_advertise();

void on_sync();

int gatt_char_write_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

void nimble_host_task(void *param);

void nimble_init();