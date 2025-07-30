#include "freertos/FreeRTOS.h"

#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <string>
#include <sstream>
#include <string.h>
#include <map>
#include <memory>
#include <stdio.h>
#include <iostream>
#include "memory"
#include "freertos/ringbuf.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "synchronizeboard.h"
#include "socketUtils.h"
#include "ConsumerTask.h"
#include "esp32_pds.h"
#include "80211Packet.h"

// static wifi_country_t wifi_country = {.cc="CN", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};
/* FreeRTOS event group to signal when we are connected*/
// static EventGroupHandle_t send_event_group;
// static SemaphoreHandle_t chanMutex;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_set_channel(uint8_t channel);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
static uint8_t wifi_connection_chan;
static wifi_second_chan_t wifi_connection_second_chan;

const char *TAG = "pds 2018";

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

const int SEND_BIT = BIT0;

EventGroupHandle_t wifi_event_group;

/* shared ringbuffer between producer and consumer */
RingbufHandle_t packetRingBuffer;

extern "C" {
    void app_main(void);
}

using namespace std;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d", 
                MAC2STR(event->event_info.sta_connected.mac),
                event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
                MAC2STR(event->event_info.sta_disconnected.mac),
                event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void
wifi_sniffer_set_channel(uint8_t channel)
{

	ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config;
    memset(&wifi_config,0, sizeof(wifi_config));
    memcpy(wifi_config.ap.ssid ,ESP_WIFI_SSID, sizeof(ESP_WIFI_SSID));
    memcpy(wifi_config.ap.password, ESP_WIFI_PASS, sizeof(ESP_WIFI_PASS));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    //sniffer configuration
    // ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    //connecting the station to wifi
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", 
            ESP_WIFI_SSID, ESP_WIFI_PASS);
    
    ESP_ERROR_CHECK(esp_wifi_get_channel(&wifi_connection_chan, &wifi_connection_second_chan));
    ESP_LOGD(TAG, "chan: %d second chan: %d", wifi_connection_chan, wifi_connection_second_chan);
    
    //setting sniffer
    ESP_LOGI(TAG, "setting promiscuous mode");
    wifi_promiscuous_filter_t promFilter;
    promFilter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&promFilter);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);    
    ESP_LOGI(TAG, "wifi_init_sta finished.");

}
	
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{   
    // xSemaphoreTake(chanMutex, portMAX_DELAY);
    // xEventGroupWaitBits(send_event_group, SEND_BIT,
            // false, true, portMAX_DELAY);
    // Todo try to remove it beacause of mask of promiscuous mode
    uint8_t subtype;
    struct tm timestamp = SynchronizeBoard::getTime();

    if (type != WIFI_PKT_MGMT) {
        return;
    }
    wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *) buff;
    if (ppkt->rx_ctrl.sig_len > RINGBUF_SIZE) 
        return;

    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    /* filter only PROBE REQ packets */
    subtype = hdr->frame_ctrl;
    subtype = subtype & 0xF0;
    if (subtype != 0x40) {
        // ESP_LOGD(TAG, "remaining size %d", xRingbufferGetCurFreeSize(packetRingBuffer));
        return;
    }

    attached_timestamp_packet_t *ppkt_with_tm = (attached_timestamp_packet_t*) ::operator new(
            sizeof(attached_timestamp_packet_t) + ppkt->rx_ctrl.sig_len);
    printf("%2X:%2X:%2X:%2X:%2X:%2X", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
    ppkt_with_tm->timestamp = timestamp;
    memcpy(&ppkt_with_tm->packet, ppkt, ppkt->rx_ctrl.sig_len);
    int res = xRingbufferSend(packetRingBuffer, ppkt_with_tm, ppkt_with_tm->packet.rx_ctrl.sig_len + sizeof(struct tm), 0);
    if (res != pdTRUE) {
        ESP_LOGW(TAG, "Error during ringBuffer insertion buffer full\n");
    } else {
        // ESP_LOGD(TAG, "(Core %d) Paket inserted, size: %d", xPortGetCoreID(), ppkt->rx_ctrl.sig_len);
        ESP_LOGD(TAG, "inserted remaining size %d", xRingbufferGetCurFreeSize(packetRingBuffer));
    }
    delete ppkt_with_tm;
    // esp_wifi_set_promiscuous(false);
    // xSemaphoreGive(chanMutex);
}

void esp_initialization() {
    // #ifndef FIXED_CHANNEL
    //    uint8_t channel = 1;
    // #endif

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    // Initialize RingBuffer
    ESP_LOGD(TAG, "dim ringbuf: %d", RINGBUF_SIZE);
    packetRingBuffer = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT);
    assert(packetRingBuffer && "Critical error during ring buffer initialization");
    // vSemaphoreCreateBinary(chanMutex);

    // Wifi mode STATION
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    // Wait until the device is connected to the wifi
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
            false, true, portMAX_DELAY);
    SynchronizeBoard::obtain_time();
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    // #ifdef FIXED_CHANNEL
    //     wifi_sniffer_set_channel(FIXED_CHANNEL);
    //     ESP_LOGI(TAG, "Fixed channel %d", FIXED_CHANNEL);
    // #endif
}

void app_main(void)
{
    int level = 0;

    esp_initialization();
    int socket = -1;
    ESP_ERROR_CHECK(connect_to_server(&socket, true));

    // ToDo taskRAM size e ringbuf size optimization
    ConsumerTask consumerTask(socket, packetRingBuffer);
    // enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    while (true) {
        #ifndef FIXED_CHANNEL
        //ToDo mutex on channel operation so when i'm sending data I lock the channel
            vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
            ESP_LOGI(TAG, "Changing channel to %d", channel);
            wifi_sniffer_set_channel(channel);
            channel = (channel % WIFI_CHANNEL_MAX) + 1;
        #else
            // wifi_sniffer_set_channel(FIXED_CHANNEL);
            // esp_wifi_set_promiscuous(true);
        #endif
            gpio_set_level(GPIO_NUM_2, level);
            level = !level;
            vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
