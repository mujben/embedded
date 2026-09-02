#include "esp_wifi.h"
#include "freertos/task.h"

#define MAX_DEVICES 32
#define DEVICE_TIMEOUT_SEC 5
#define REF_RSSI_1M -45
#define PATH_LOSS 2.5

typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t da[6];
    uint8_t sa[6];
    uint8_t bssid[6];
    uint16_t seq_ctrl;
} __attribute__ ((packed)) wifi_mgmt_header_t;

typedef struct {
    uint8_t mac[6];
    char ssid[MAX_SSID_LEN + 1];
    int channel;
    int rssi;
    float distance;
    int64_t last_seen_us;
} ap_entry_t;

void setup_ap_radar();
void print_summary();
void wifi_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type);
void run_radar();