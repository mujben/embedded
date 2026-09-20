/*
 * Examples used to build BTLE communications
 * https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/ble_get_started/nimble/NimBLE_Security
 */
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <usb_hid.h>
#include <ble_rubber_ducky.h>

const char *TAG = "ble_ducky";

// Queue to hold events received from BLE characteristic write
static QueueHandle_t hid_event_queue = NULL;

// GATT service characteristic
static const ble_uuid128_t svc_uuid =
    BLE_UUID128_INIT(0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x42,0x69,0x37,0x21);
// GATT characteristic for receiving data
static const ble_uuid128_t chr_uuid =
    BLE_UUID128_INIT(0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xee,0xff,0xc0);

const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def []) {
            {
                .uuid = &chr_uuid.u,
                .access_cb = gatt_char_write_cb,
                // only paired and authorized device can write
                // when monitor is connected
                // .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_AUTHEN | BLE_GATT_CHR_F_WRITE_ENC
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC
            }, {0}
        }
    }, {0}
};

// Event for data interpreting inside the HID task, to differentiate between normal character and prank payload
typedef enum {
    HID_EVT_CHAR,
    HID_EVT_PAYLOAD_PRANK
} hid_event_type_t;

// Event type and datat to be sent
typedef struct {
    hid_event_type_t type;
    char data;
} hid_event_t;

// Mainly used for PIN pairing (if monitor is connected)
// int gap_event_handler(struct ble_gap_event *event, void *arg) {
//     int rc = 0;

//     switch(event->type) {
//         // for now, only passkey event to display random 6 digit code on the screen
//         case BLE_GAP_EVENT_PASSKEY_ACTION: {
//             if(event->passkey.params.action == BLE_SM_IOACT_DISP) {
//                 struct ble_sm_io passkey = {0};
//                 passkey.action = event->passkey.params.action;
//                 passkey.passkey = 100000 + esp_random() % 999999;
//                 ESP_LOGI(TAG, "Enter the passkey on the other device: %d", passkey.passkey);
//                 rc = ble_sm_inject_io(event->passkey.conn_handle, &passkey);

//                 if(rc != 0) {
//                     ESP_LOGE(TAG, "Failed to inject security manager io, error code: %d", rc);
//                     return rc;
//                 }
//             }
//             return rc;
//         }
//         case BLE_GAP_EVENT_ENC_CHANGE: {
//             ESP_LOGI(TAG, "Connection encryption changed, ble_err code: %d", event->enc_change.status);
//             return rc;
//         }
//         default:
//             break;
//     }
//     return rc;
// }

void ble_advertise() {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields adv_fields = {0};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name = (uint8_t *)"BLE Rubber Ducky";
    adv_fields.name_len = strlen("BLE Rubber Ducky");
    adv_fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&adv_fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, 0 /*when monitor is connected gap_event_handler*/, NULL);
}

// Sync callback, calls ble_advertise()
void on_sync() {
    ble_advertise();
}

/* 
 * Handling BLE characteristics to send data receieved from other device to a hid_char_queue
 */
int gatt_char_write_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        char buf[256] = {0};
        if(len >= sizeof(buf)) len = sizeof(buf) - 1;
        os_mbuf_copydata(ctxt->om, 0, len, buf);
        
        // Prank sequence
        uint8_t prank_trigger[] = {0xc0, 0xff, 0xee};
        bool is_prank_trigger = (len == sizeof(prank_trigger)) && (memcmp(buf, prank_trigger, sizeof(prank_trigger)) == 0);
        if(is_prank_trigger) {
            hid_event_t prank_event = {
                .type = HID_EVT_PAYLOAD_PRANK,
                .data = 0
            };
            xQueueSend(hid_event_queue, &prank_event, 0);
        }

        // Other data
        for(int i = 0; i < len; i++) {
            hid_event_t char_to_send = {
                .type = HID_EVT_CHAR,
                .data = (char)buf[i]
            };
            xQueueSend(hid_event_queue, &char_to_send, 0);
        }
    }
    return 0;
}

void run_prank_payload();

/*
 * Separate task for handling HID character queue and sending keystrokes
 */
void usb_hid_task(void *pvParameters) {
    hid_event_t event;
    while(true) {
        if(xQueueReceive(hid_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            if(event.type == HID_EVT_PAYLOAD_PRANK) {
                run_prank_payload();
            } else if(event.type == HID_EVT_CHAR) {
                send_keystroke(event.data);
            }
        }
    }
}

void nimble_host_task(void *param) {
    hid_event_queue = xQueueCreate(512, sizeof(hid_event_t));
    xTaskCreatePinnedToCore(usb_hid_task, "usb_hid_task", 4096, NULL, 1, NULL, 1);
    
    // Main loop for NimBLE host task
    nimble_port_run();
    nimble_port_freertos_deinit();
}


void nimble_init() {
    ESP_ERROR_CHECK(nimble_port_init());
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_hs_cfg.sync_cb = on_sync;
    
    // LESC (LE Secure Connections) setup
    ble_hs_cfg.sm_sc = 1;

    // Safety and LESC
    // When monitor is connected
    // ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
    // ble_hs_cfg.sm_mitm = 1;
    
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_bonding = 1;

    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
}

void run_prank_payload() {
    send_key_combination(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);

    vTaskDelay(pdMS_TO_TICKS(500));

    send_string("cmd /c start https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    send_key_combination(0, HID_KEY_ENTER);

    vTaskDelay(pdMS_TO_TICKS(2000));

    send_key_combination(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);

    vTaskDelay(pdMS_TO_TICKS(500));

    send_string("powershell -w h -c \"Start-Process notepad; Start-Sleep 1; $w = New-Object -Com WScript.Shell; 'Hello... I am trapped inside the motherboard.'.ToCharArray() | % { $w.SendKeys($_); Start-Sleep -m 120 }\"");
    send_key_combination(0, HID_KEY_ENTER);
}