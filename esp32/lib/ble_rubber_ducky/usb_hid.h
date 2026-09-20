#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "tusb.h"
#include "class/hid/hid_device.h"
#include "tinyusb.h"

void init_usb(void);

uint8_t ascii_to_keycode(const char c, uint8_t *modifier);

void send_keystroke(const char c);

void send_string(const char *str);

void send_key_combination(uint8_t modifier, uint8_t keycode);
