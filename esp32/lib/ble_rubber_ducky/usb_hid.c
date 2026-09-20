#include <stdint.h>
#include "esp_log.h"

#include <usb_hid.h>
#include "tinyusb_default_config.h"

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

// callback variables
static hid_keyboard_report_t last_keyboard_report = {0};
static uint8_t host_keyboard_leds = 0;

/* HID report descriptor for a keyboard
*/
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

/*
 * String descriptors
 * 0: supported language (English - 0x0409)
 * 1: Manufacturer
 * 2: Product
 * 3: Serial
 * 4: HID Interface
 */
static const char *hid_string_descriptor[5] = {
    (char[]) {0x09, 0x04},
    "ESP32", 
    "BLE Rubber Ducky",
    "213769",
    "Keyboard"
};

/* 
 * USB configuration descriptor
 * One interface HID, endpoint IN = 0x81
 */
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, true, sizeof(hid_report_descriptor), 0x81, 16, 10)
};

/*
 * TinyUSB HID callback
 * Host asks for HID report descriptor
 */
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return hid_report_descriptor;
}

/*
 * TinyUSB HID GET_REPORT callback
 */
uint16_t tud_hid_get_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t *buffer,
    uint16_t reqlen)
{
    (void)instance;
    (void)report_id;

    if(report_type == HID_REPORT_TYPE_INPUT) {
        uint16_t report_size = sizeof(last_keyboard_report);
        if(reqlen < report_size) return 0;
        
        memcpy(buffer, &last_keyboard_report, report_size);
        return report_size;
    }

    else if(report_type == HID_REPORT_TYPE_OUTPUT) {
        if(reqlen < 1) return 0;
        buffer[0] = host_keyboard_leds;
        return 1;
    }
    return 0;
}

/*
 * TinyUSB HID SET_REPORT callback.
 * Reading LED state, CapsLock, NumLock, etc.
 */
void tud_hid_set_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const *buffer,
    uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    if(report_type == HID_REPORT_TYPE_OUTPUT) {
        if(bufsize < 1) return;
        host_keyboard_leds = buffer[0];

        bool num_lock    = (host_keyboard_leds & KEYBOARD_LED_NUMLOCK)    != 0;
        bool caps_lock   = (host_keyboard_leds & KEYBOARD_LED_CAPSLOCK)   != 0;
        bool scroll_lock = (host_keyboard_leds & KEYBOARD_LED_SCROLLLOCK) != 0;

        ESP_LOGI("USB_LED", "Led State - Num: %d, Caps: %d, Scroll: %d", 
                 num_lock, caps_lock, scroll_lock);

    }
}

/*
 * USB HID initialization
 */
void init_usb(void) {
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    // default device descriptor
    tusb_cfg.descriptor.device = NULL;

    // HID configuration descriptor
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;

    // string descriptors
    tusb_cfg.descriptor.string = hid_string_descriptor;

    tusb_cfg.descriptor.string_count =
        sizeof(hid_string_descriptor) /
        sizeof(hid_string_descriptor[0]);

#if (TUD_OPT_HIGH_SPEED)

    tusb_cfg.descriptor.high_speed_config =
        hid_configuration_descriptor;

#endif
    ESP_ERROR_CHECK(
        tinyusb_driver_install(&tusb_cfg)
    );
}

/*
 * LUT for ASCII to HID keycode mapping
 */
typedef struct {
    uint8_t keycode;
    uint8_t modifier;
} hid_key_mapping_t;

static const hid_key_mapping_t ascii_to_hid_table[128] = {
    // 0x00 - 0x1F (Control characters)
    ['\n'] = { HID_KEY_ENTER, 0 },
    ['\r'] = { HID_KEY_ENTER, 0 },

    // 0x20 - 0x2F
    [' ']  = { HID_KEY_SPACE, 0 },
    ['!']  = { HID_KEY_1, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['"']  = { HID_KEY_APOSTROPHE, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['#']  = { HID_KEY_3, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['$']  = { HID_KEY_4, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['%']  = { HID_KEY_5, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['&']  = { HID_KEY_7, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['\''] = { HID_KEY_APOSTROPHE, 0 },
    ['(']  = { HID_KEY_9, KEYBOARD_MODIFIER_LEFTSHIFT },
    [')']  = { HID_KEY_0, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['*']  = { HID_KEY_8, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['+']  = { HID_KEY_EQUAL, KEYBOARD_MODIFIER_LEFTSHIFT },
    [',']  = { HID_KEY_COMMA, 0 },
    ['-']  = { HID_KEY_MINUS, 0 },
    ['.']  = { HID_KEY_PERIOD, 0 },
    ['/']  = { HID_KEY_SLASH, 0 },

    // 0x30 - 0x39 (Cyfry)
    ['0']  = { HID_KEY_0, 0 },
    ['1']  = { HID_KEY_1, 0 },
    ['2']  = { HID_KEY_2, 0 },
    ['3']  = { HID_KEY_3, 0 },
    ['4']  = { HID_KEY_4, 0 },
    ['5']  = { HID_KEY_5, 0 },
    ['6']  = { HID_KEY_6, 0 },
    ['7']  = { HID_KEY_7, 0 },
    ['8']  = { HID_KEY_8, 0 },
    ['9']  = { HID_KEY_9, 0 },

    // 0x3A - 0x40
    [':']  = { HID_KEY_SEMICOLON, KEYBOARD_MODIFIER_LEFTSHIFT },
    [';']  = { HID_KEY_SEMICOLON, 0 },
    ['<']  = { HID_KEY_COMMA, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['=']  = { HID_KEY_EQUAL, 0 },
    ['>']  = { HID_KEY_PERIOD, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['?']  = { HID_KEY_SLASH, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['@']  = { HID_KEY_2, KEYBOARD_MODIFIER_LEFTSHIFT },

    // 0x41 - 0x5A (Wielkie litery A-Z)
    ['A'] = { HID_KEY_A, KEYBOARD_MODIFIER_LEFTSHIFT }, ['B'] = { HID_KEY_B, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['C'] = { HID_KEY_C, KEYBOARD_MODIFIER_LEFTSHIFT }, ['D'] = { HID_KEY_D, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['E'] = { HID_KEY_E, KEYBOARD_MODIFIER_LEFTSHIFT }, ['F'] = { HID_KEY_F, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['G'] = { HID_KEY_G, KEYBOARD_MODIFIER_LEFTSHIFT }, ['H'] = { HID_KEY_H, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['I'] = { HID_KEY_I, KEYBOARD_MODIFIER_LEFTSHIFT }, ['J'] = { HID_KEY_J, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['K'] = { HID_KEY_K, KEYBOARD_MODIFIER_LEFTSHIFT }, ['L'] = { HID_KEY_L, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['M'] = { HID_KEY_M, KEYBOARD_MODIFIER_LEFTSHIFT }, ['N'] = { HID_KEY_N, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['O'] = { HID_KEY_O, KEYBOARD_MODIFIER_LEFTSHIFT }, ['P'] = { HID_KEY_P, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['Q'] = { HID_KEY_Q, KEYBOARD_MODIFIER_LEFTSHIFT }, ['R'] = { HID_KEY_R, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['S'] = { HID_KEY_S, KEYBOARD_MODIFIER_LEFTSHIFT }, ['T'] = { HID_KEY_T, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['U'] = { HID_KEY_U, KEYBOARD_MODIFIER_LEFTSHIFT }, ['V'] = { HID_KEY_V, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['W'] = { HID_KEY_W, KEYBOARD_MODIFIER_LEFTSHIFT }, ['X'] = { HID_KEY_X, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['Y'] = { HID_KEY_Y, KEYBOARD_MODIFIER_LEFTSHIFT }, ['Z'] = { HID_KEY_Z, KEYBOARD_MODIFIER_LEFTSHIFT },

    // 0x5B - 0x60
    ['[']  = { HID_KEY_BRACKET_LEFT, 0 },
    ['\\'] = { HID_KEY_BACKSLASH, 0 },
    [']']  = { HID_KEY_BRACKET_RIGHT, 0 },
    ['^']  = { HID_KEY_6, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['_']  = { HID_KEY_MINUS, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['`']  = { HID_KEY_GRAVE, 0 },

    // 0x61 - 0x7A (Małe litery a-z)
    ['a'] = { HID_KEY_A, 0 }, ['b'] = { HID_KEY_B, 0 },
    ['c'] = { HID_KEY_C, 0 }, ['d'] = { HID_KEY_D, 0 },
    ['e'] = { HID_KEY_E, 0 }, ['f'] = { HID_KEY_F, 0 },
    ['g'] = { HID_KEY_G, 0 }, ['h'] = { HID_KEY_H, 0 },
    ['i'] = { HID_KEY_I, 0 }, ['j'] = { HID_KEY_J, 0 },
    ['k'] = { HID_KEY_K, 0 }, ['l'] = { HID_KEY_L, 0 },
    ['m'] = { HID_KEY_M, 0 }, ['n'] = { HID_KEY_N, 0 },
    ['o'] = { HID_KEY_O, 0 }, ['p'] = { HID_KEY_P, 0 },
    ['q'] = { HID_KEY_Q, 0 }, ['r'] = { HID_KEY_R, 0 },
    ['s'] = { HID_KEY_S, 0 }, ['t'] = { HID_KEY_T, 0 },
    ['u'] = { HID_KEY_U, 0 }, ['v'] = { HID_KEY_V, 0 },
    ['w'] = { HID_KEY_W, 0 }, ['x'] = { HID_KEY_X, 0 },
    ['y'] = { HID_KEY_Y, 0 }, ['z'] = { HID_KEY_Z, 0 },

    // 0x7B - 0x7E
    ['{']  = { HID_KEY_BRACKET_LEFT, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['|']  = { HID_KEY_BACKSLASH, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['}']  = { HID_KEY_BRACKET_RIGHT, KEYBOARD_MODIFIER_LEFTSHIFT },
    ['~']  = { HID_KEY_GRAVE, KEYBOARD_MODIFIER_LEFTSHIFT },
};

/*
 * Conversion from ASCII to HID keycode
 */
uint8_t ascii_to_keycode(const char c, uint8_t *modifier) {
    *modifier = 0;
    // normal characters
    if((uint8_t) c > 127) {
        if(modifier) *modifier = 0;
        return 0;
    }
    if(modifier) *modifier = ascii_to_hid_table[(uint8_t) c].modifier;
    return ascii_to_hid_table[(uint8_t) c].keycode;
}

void send_keystroke(const char c) {
    uint8_t modifier = 0;
    uint8_t keycode = ascii_to_keycode(c, &modifier);
    if(keycode == 0) return;
    
    while(!tud_hid_ready()) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t key_report[6] = {keycode, 0, 0, 0, 0, 0};
    // key press
    tud_hid_keyboard_report(0,  modifier, key_report);

    // internal keyboard state for callback functions
    last_keyboard_report.modifier = modifier;
    last_keyboard_report.keycode[0] = keycode;

    vTaskDelay(pdMS_TO_TICKS(2 + rand() % 5));
    
    while(!tud_hid_ready()) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // key release
    uint8_t empty_report[6] = {0, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, 0, empty_report);

    last_keyboard_report.modifier = 0;
    memset(last_keyboard_report.keycode, 0, sizeof(last_keyboard_report.keycode));

    vTaskDelay(pdMS_TO_TICKS(2 + rand() % 5));
}

void send_string(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        send_keystroke(str[i]);
    }
}

void send_key_combination(uint8_t modifier, uint8_t keycode) {
    while(!tud_hid_ready()) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t key_report[6] = {keycode, 0, 0, 0, 0, 0};
    // key press
    tud_hid_keyboard_report(0, modifier, key_report);

    vTaskDelay(pdMS_TO_TICKS(2 + rand() % 5));
    
    while(!tud_hid_ready()) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // key release
    uint8_t empty_report[6] = {0, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, 0, empty_report);

    vTaskDelay(pdMS_TO_TICKS(2 + rand() % 5));
}