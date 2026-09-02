#include <math.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <ap_radar.h>

static ap_entry_t ap_list[MAX_DEVICES] = {};
static int discovered_ap_count = 0;
static bool data_changed = false;
static SemaphoreHandle_t list_mutex = NULL;

float rssi_to_m(const int rssi) {
    return powf(10.0f, (REF_RSSI_1M - (float) rssi) / (10.0f * PATH_LOSS));
}

void setup_ap_radar() {
    // List operation semaphore
    list_mutex = xSemaphoreCreateMutex();
    // NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Init wifi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Promiscuous mode
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void print_summary() {
    printf("\033[2J\033[H\033[3J");
    printf("|    MAC ADDRESS    |               SSID               | CHANNEL |  RSSI  | DISTANCE [m] |\n");
    for(int i = 0; i < discovered_ap_count; i++) {
        printf("| %02x:%02x:%02x:%02x:%02x:%02x | %-32.32s |   %3d   |  %4d  |   %7.2f    |\n",
        ap_list[i].mac[0], ap_list[i].mac[1], ap_list[i].mac[2],
        ap_list[i].mac[3], ap_list[i].mac[4], ap_list[i].mac[5],
        ap_list[i].ssid, ap_list[i].channel, ap_list[i].rssi, ap_list[i].distance);
    }
    fflush(stdout);
}

int compare_by_dist(const void *a, const void *b) {
    const ap_entry_t *ent_a = (const ap_entry_t*) a;
    const ap_entry_t *ent_b = (const ap_entry_t*) b;
    if(ent_a->distance > ent_b->distance) return 1;
    if(ent_a->distance < ent_b->distance) return -1;
    return 0;
}

void wifi_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    if(type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_mgmt_header_t *header = (wifi_mgmt_header_t *) pkt->payload;
    const uint8_t *raw_payload = (const uint8_t *) pkt->payload;
    uint16_t length = pkt->rx_ctrl.sig_len;
    int channel = pkt->rx_ctrl.channel; // channel of the receiver

    const uint8_t *mac = header->sa;
    bool broadcast = true;
    for(int i = 0; i < 6; i++) {
        if(mac[i] != 0xFF) broadcast = false;
    }
    if(broadcast) return;

    int frame_type = raw_payload[0] & 0xFC;
    char ssid[MAX_SSID_LEN + 1] = "N/A";
    if(frame_type == 0x80 || frame_type == 0x50) {
        int offset = 36;
        while (offset + 2 < length) {
            uint8_t tag_id = raw_payload[offset];
            uint8_t tag_len = raw_payload[offset + 1];
            // Extract SSID from IE
            if(tag_id == 0 && tag_len > 0 && tag_len < 32 && (offset + tag_len + 2) <= length) {
                memcpy(ssid, &raw_payload[offset + 2], MIN(tag_len, MAX_SSID_LEN));
                ssid[MIN(tag_len, MAX_SSID_LEN)] = '\0';
                data_changed = true;
            }
            // Determine the channel of the AP
            else if(tag_id == 3 && tag_len == 1) {
                channel = raw_payload[offset + 2];
            }
            offset += tag_len + 2;
        }
    }
    
    int rssi = pkt->rx_ctrl.rssi;
    float distance = rssi_to_m(rssi);
    int64_t now = esp_timer_get_time();

    if(list_mutex != NULL && xSemaphoreTake(list_mutex, 0) == pdTRUE) {
        int found_idx = -1;
        for(int i = 0; i < MAX_DEVICES; i++) {
            if(memcmp(ap_list[i].mac, mac, 6) == 0) {
                found_idx = i;
                break;
            }
        }
        if(found_idx != -1) {
            memcpy(ap_list[found_idx].ssid, ssid, MAX_SSID_LEN);
            ap_list[found_idx].channel = channel;
            ap_list[found_idx].rssi = rssi;
            ap_list[found_idx].distance = distance;
            ap_list[found_idx].last_seen_us = now;
            data_changed = true;
        } else if(discovered_ap_count < MAX_DEVICES) {
            memcpy(ap_list[discovered_ap_count].mac, mac, 6);
            memcpy(ap_list[discovered_ap_count].ssid, ssid, MAX_SSID_LEN);
            ap_list[discovered_ap_count].channel = channel;
            ap_list[discovered_ap_count].rssi = rssi;
            ap_list[discovered_ap_count].distance = distance;
            ap_list[discovered_ap_count].last_seen_us = now;
            discovered_ap_count++;
            data_changed = true;
        }
        xSemaphoreGive(list_mutex);
    }
}

void run_radar() {
    setup_ap_radar();
    int channel = 1;
    while(1) {
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        if(xSemaphoreTake(list_mutex, portMAX_DELAY) == pdTRUE) {
            int64_t now = esp_timer_get_time();
            for(int i = 0; i < discovered_ap_count; i++) {
                if((now - ap_list[i].last_seen_us) > (DEVICE_TIMEOUT_SEC * 1000000LL)) {
                    ap_list[i] = ap_list[discovered_ap_count - 1];
                    discovered_ap_count--;
                    i--;
                    data_changed = true;
                }
            }
            if(data_changed) {
                qsort(ap_list, discovered_ap_count, sizeof(ap_entry_t), compare_by_dist);
                print_summary();
                data_changed = false;
            }
            xSemaphoreGive(list_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        channel++;
        if(channel > 13) channel = 1;
    }
}