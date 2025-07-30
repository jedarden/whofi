/* ESP HANDLE LIB */
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
/*--------------------------ESP Main libs*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
/* --------------------------LED HANDLE INCLUSION */
#include "driver/gpio.h"
#include "sdkconfig.h"
/* --------------------------MY FILES INCLUSION */
#include "config.h"		//for all my variable
#include "cmd_file.h"
#include "cmd_socket.h"
#include "../components/md5/md5.h"
/* --------------------------VFS and SPIFFS INCLUSION */
#include "esp_vfs.h"
#include "../components/spiffs/spiffs_vfs.h"
#include "../components/spiffs/mutex.h"
#define INCLUDE_vTaskSuspend                    1

const char *TAGM = "Main";
struct tm timeinfo;
extern time_t server_time; //FEDE
time_t server_time_available;
time_t offset_time_available;
time_t offset_time;
int s = 0;	/* socket ID */

/* shared resources and mutex
 * to handle the parallel storage
 * sniffed_pkg.txt is the file
 * containing the sniffed data */

SemaphoreHandle_t xMutex = NULL;

FILE *fSniffs;
const char *PATH_first = "/spiffs/sniffed_pkg1.txt";
const char *PATH_second = "/spiffs/sniffed_pkg2.txt";
int id_sniFile = 1;
char a = 'a';

/*---------------------------AP Credential */
#define AP_WIFI_SSID "AP_ACCESS\0"
#define AP_WIFI_PASS "12345678"

/*---------------------------SOCKET Credential */
//credential [wifi&socket] definition (by menuconfig)
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD
/* it must be the IP of the pc(server) to find it out go to ifconfig->Wifi0->inetaddr:(ipv4) */
#define DEFAULT_SERVER_IP CONFIG_SERVER_IP
#define DEFAULT_SERVER_PORT CONFIG_SERVER_PORT

/*---------------------------Connection group */
static wifi_country_t wifi_country = {.cc="CN", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};	// wifi policies (by country)
static EventGroupHandle_t wifi_event_group;					// Event group handling wifi connection
const int CONNECTED_BIT = BIT0;								// CONNECTION pin for wifiGroup definition

/*---------------------------Sniffer group */
const int TIMEOUT = 20000 / portTICK_RATE_MS;			    // sniffing timeout time
#define	CHANNEL_MAX		(13)
#define WIFI_PROMIS_FILTER_MASK_ALL         (0xFFFFFFFF)    /**< filter all packets */
#define WIFI_PROMIS_FILTER_MASK_MGMT        (1)             /**< filter the packets with type of WIFI_PKT_MGMT */
#define WIFI_EVENT_MASK_AP_PROBEREQRECVED   (BIT(0))        /**< mask SYSTEM_EVENT_AP_PROBEREQRECVED event */
#define PROBE_MASK 0x4000										// my own defined PROBE_MASK
#define BEACON_MASK 0x8000										// my own defined BEACON_MASK
#define DEFAULT_CHANNEL CONFIG_WIFI_SELECTED_CHANNEL		//define handle for blink task
static EventGroupHandle_t sniff_event_group;				// Event group for sniffer handle
const int SNIFFEND_BIT = BIT1;

//-----------------------Blink led
#define BLINK_GPIO 2										// LED pin definition
int BLINK_TIME = 1100;										// LED Blink time init

//-----------------------GLOBAL VARIABLE & STRUCTURES
typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; 										/* receiver address */
	uint8_t addr2[6]; 										/* sender address */
	uint8_t addr3[6]; 										/* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; 										/* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; 									/* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

//METHODS
static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_init();
//static void wifi_deinit();
int esp_settings_init();
void sniffer_init(uint8_t chnl);
static void sniffer_set_channel(uint8_t channel);
static void sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
void sniffer_start(void);
void sniffer_stop(void);
void blink_task(void *pvParameter);
void sniff_task(void *pvParameter);
void filesend_task(void *pvParameter);

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {

    case SYSTEM_EVENT_AP_START:
    	ESP_LOGI(TAGM,"[*] AP service started.");
        break;

    case SYSTEM_EVENT_STA_START:
    	ESP_LOGI(TAGM,"[*] STA service started.");
        esp_wifi_connect();
        break;

    //if here then it successfull connected(return SYSTEM_EVENT_STA_GOT_IP), so print the Network info
	case SYSTEM_EVENT_STA_GOT_IP:
		BLINK_TIME = 100;
		ESP_LOGI(TAGM,"[*]Successfully Connected to %s!\n",DEFAULT_SSID);
		tcpip_adapter_ip_info_t ip_info;
		ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
		ESP_LOGI(TAGM,"========================== INFO =======================");
		ESP_LOGI(TAGM,"   IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
		ESP_LOGI(TAGM,"   Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
		ESP_LOGI(TAGM,"   Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
		ESP_LOGI(TAGM,"=======================================================");
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;

    //in case of disconnection(returned SYSTEM_EVENT_STA_DISCONNECTED) then  clean the Event group
	case SYSTEM_EVENT_STA_DISCONNECTED:
		BLINK_TIME = 1100;
		ESP_LOGW(TAGM,"Warning: ESP is disconnected from the network!\n");
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		// here we connect with the network and continue, if it doesn't work after portMAX_DELAY time then it abort!
		esp_restart();
        break;

	case SYSTEM_EVENT_AP_STOP:
			ESP_LOGW(TAGM,"[!] Warning: ESP disconnection as AP\n");
	        break;
    
	default:
        break;
    }
   
	return ESP_OK;
}


// Main application
void app_main()
{
	TaskHandle_t TaskHandle_blink;
	TaskHandle_t TaskHandle_sniff;
	TaskHandle_t TaskHandle_filesend;

	esp_log_level_set("wifi", ESP_LOG_NONE);      // enable WARN logs from WiFi stack
	esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	//++++++++++++++++++++++ SPIFFS ++++++++++++++++++++++++++
	ESP_LOGI(TAGM, "[+] Starting spiffs memory function +++\n");

	vfs_spiffs_register();

	// the partition was mounted?
	if (spiffs_is_mounted) {
		ESP_LOGI(TAGM,"[*] Partition correctly mounted!\r\n");
	} else {
		ESP_LOGE(TAGM,"[x] Error: Issues while mounting the SPIFFS partition.\n\rREBOOTING\n");
		esp_restart();
	}
	/* check if I/O function correctly work and creationj of files */
	fSniffs = fopen(PATH_first, "w");
	if (fSniffs == NULL) {
		ESP_LOGE(TAGM,"[x] Error: Impossible to create the file! SPIFFS partition not working right.");
		esp_restart();
	}
	fclose(fSniffs);

	fSniffs = fopen(PATH_second, "w");
	if (fSniffs == NULL) {
		ESP_LOGE(TAGM,"[x] Error: Impossible to create the file! SPIFFS partition not working right.");
		esp_restart();
	}
	fclose(fSniffs);

	if (file_check(PATH_first) == -1 || file_check(PATH_second) == -1) {
		ESP_LOGE(TAGM,"[x] Error: Unknown errors with the file and Partition!");
		esp_restart();
	}
	ESP_LOGI(TAGM,"[*] Partition OK!\r\n");

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();

	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	//start blinking task - MICRO WORKING FINE
	xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &TaskHandle_blink);
	ESP_LOGI(TAGM,"[*] Blinker -> STARTED \n");

	// here we connect with the network and continue, if it doesn't work after portMAX_DELAY time then it abort!
	wifi_init();
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

	//after connecting with wifi open the socket connection for starting the service
	ESP_LOGI(TAGM,"\r(+) Opening the socket connection with broker server at %s:%s \n",DEFAULT_SERVER_IP,DEFAULT_SERVER_PORT);
	if ((s = start_socket_connection(DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT)) != -1)
    	ESP_LOGI(TAGM, "[*] Connected to target address.");
	else{
		close(s);
		esp_restart();
	}

	/* init the device with the server (REGISTRATION and TIMESTAMP acquisition) */
	if (esp_settings_init() != -1)
		ESP_LOGI(TAGM, "[*] ESP32 setup completed!");
	else{
		close(s);
		esp_restart();
	}

	/* CHECK if mutex are well configured
	 * they will be crutial in the task synchronization */
	xMutex = xSemaphoreCreateBinary();
	if (xMutex == NULL) {
		ESP_LOGE(TAGM,"[x] Error: Mutex init failed. REBOOTING");
		close(s);
		esp_restart();
	}
	xSemaphoreGiveFromISR(xMutex, NULL);

	ESP_LOGW(TAGM,"MUTEX CREATO on core %d", xPortGetCoreID());

	//start remaining task and creation of the group event to synchronize their workflow
	sniff_event_group = xEventGroupCreate();
	ESP_LOGI(TAGM,"[*] Sniffer Task -> STARTED \n");
	xTaskCreatePinnedToCore(&sniff_task, "sniff_task", 10000, NULL, 2, &TaskHandle_sniff, 0);
	vTaskDelay(500);
	ESP_LOGI(TAGM,"[*] Sender Task -> STARTED \n");
	xTaskCreatePinnedToCore(&filesend_task, "filesend_task", 10000, NULL, 2, &TaskHandle_filesend, 0);

	/* program continue with the main task
	 * it will be used for handle the commands coming
	 * from the server like the BLINKING, REBOOTING or STANDBY etc... */
	int n;
	ESP_LOGI(TAGM,"[*] Commander Task -> STARTED \n");
	while (1)
	{
		ESP_LOGI(TAGM,"waiting for command from the server...");
		n = recv(s, rbuf, BUFLEN-1, 0);
		ESP_LOGW(TAGM, "received a COMMAND of N=%d BYTES", n);
		if (n==0) {
			ESP_LOGE(TAGM,"Error: Server shut-down during transfering, data will be broken.\n");
			break;
		}
		else if (n < 0) {
			ESP_LOGE(TAGM,"Write Error.\n");
			break;
		}
		else {
			rbuf[n]='\0';
			ESP_LOGI(TAGM,"command : [%s]\n", rbuf);
			/* EXECUTION of commands based on text received */

			if (strstr(rbuf, "BLINK") != NULL) {
				ESP_LOGW(TAGM,"[c] start blinking");
				BLINK_TIME = 200;
			}
			else if (strstr(rbuf, "OKLED") != NULL) {
				ESP_LOGW(TAGM,"[c] stop blinking");
				BLINK_TIME = 100;
			}
			else if (strstr(rbuf,"REBOOT") != NULL)
			{
				ESP_LOGW(TAGM,"[c] rebooting");
				ESP_LOGI(TAGM,"[!] reboot called by the Server. Reboot of the device Started");
				close(s);
				esp_restart();
			}
			/* Only in this SYNC there is a variation: here the PING-PONG is
			 * preceeded by another SYNC from The ESP32.
			 * Nothing else change from the initial one */
			else if (strstr(rbuf,"SYNC") != NULL)
			{
				ESP_LOGW(TAGM,"[c] timestamping");
				ESP_LOGI(TAGM,"[+] ESP timestamp SyncTask -> STARTED ");
				while (xSemaphoreTake( xMutex, ( TickType_t ) 500 ) != pdTRUE) {
					ESP_LOGW(TAGM,"tentativo di prendere il mutex - sync_task");
				}
				ESP_LOGW(TAGM,"MUTEX PRESO! - sync_task");
				/* send SYNC to start the procedure */
				if (send_msg(s, "SYNC") != -1) {
					/* client syncronization procedure */
					if ((server_time = (time_t) client_timesync(s)) > 0) {
						offset_time = time(NULL);
						ESP_LOGI(TAGM,"[*] Timestamp Correctly exchanged! Tempt of configuration..");
						localtime_r(&server_time, &timeinfo);
					} else {
						ESP_LOGE(TAGM,"[x] Error : Sync Fail... RETRYING!");
						esp_restart();
					}
					// change the timezone to Italy
					setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
					tzset();
					// print the actual time in Italy
					char buffer[100];
					localtime_r(&server_time, &timeinfo);
					strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
					ESP_LOGI(TAGM,"[*] Actual time in Italy: %s", buffer);
					ESP_LOGI(TAGM,"[*] Timestamp synchronized correctly.");
				} else {
					ESP_LOGE(TAGM,"[x] Error : Sync Fail.");
					esp_restart();
				}
				xSemaphoreGiveFromISR(xMutex, NULL);
				ESP_LOGW(TAGM,"MUTEX RILASCIATO - sync_task");
			}
			else if (strstr(rbuf,"STANDBY") != NULL)
			{
				ESP_LOGW(TAGM,"[c] sniff process stop");
				vTaskSuspend(TaskHandle_sniff);
			}
			else if (strstr(rbuf,"RESUME") != NULL)
			{
				ESP_LOGW(TAGM,"[c] sniff process resume");
				vTaskResume(TaskHandle_sniff);
			}
		}
	}

	/* if we are here would be for sure for some error in code, so reboot */
	close(s);
	ESP_LOGE(TAGM,"[x] Error: Ops, some errors occurred. REBOOTING");
	esp_restart();
}

void wifi_init()
{
	// initialize the wifi stack in STAtion mode with config in RAM
	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	/*
	 * setup ESP32 as having two distinct network interfaces:
	 *  one for connecting to an access point (the ESP32 being a station)
	 *  and the other for being an access point
	 */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	// configure the wifi connection and start the interface
	wifi_config_t wifi_sta_config = {
	        .sta = {
	            .ssid = DEFAULT_SSID,
	            .password = DEFAULT_PWD,
	        },
	};

	wifi_config_t wifi_ap_config = {
			.ap = {
				.ssid = AP_WIFI_SSID,
				.ssid_len = 0,
				.password = AP_WIFI_PASS,
				.channel = 1,
				.authmode = WIFI_AUTH_WPA2_PSK,
				.beacon_interval = 400,
				.max_connection = 16,
			}
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAGM,"[+] Starting AP at %s ... \n", AP_WIFI_SSID);
	ESP_LOGI(TAGM,"[+] Connecting to %s ... \n", DEFAULT_SSID);
}

void wifi_deinit()
{
	BLINK_TIME = 100 / portTICK_RATE_MS;
	ESP_ERROR_CHECK( esp_wifi_stop() );
	ESP_ERROR_CHECK( esp_wifi_deinit() );
}

int esp_settings_init()
{
	/* REGISTRATION to the server */
	if (s >= 0) //valid socket
	{
		ESP_LOGI(TAGM,"[*] Socket connection -> STARTED \n");
		ESP_LOGI(TAGM,"[+] ESP registration-> message sent \n");
		/* client registration procedure */
		int trials = 5; //number of attempts to connect to server with a REGISTER message
		while (trials != 0)
		{
			if (client_register(s) != -1){
				ESP_LOGI(TAGM,"[*] REGISTERED correctly.");
				break;
			}else{
				ESP_LOGE(TAGM,"[x] Error in the registration... RETRYING! [ %d/%d ] \n",(5-trials),5);
				trials --;
			}
		}
		//check if errors occurs during registration
		if (trials == 0) { //after n failed attempts, the ESP will reboot
			ESP_LOGE(TAGM,"[x] Error in the socket connection... REBOOTING! \n");
			close(s);
			esp_restart();
		}
	} else {
		ESP_LOGE(TAGM,"[x] Error: Impossible to reach the broker.");
		return -1;
	}

	/* SYNCHRONIZATION of the TIMESTAMP with the server */
	if (s >= 0)
	{
		ESP_LOGI(TAGM,"[*] Connected Successfully with Time-Server.");
		ESP_LOGI(TAGM,"[+] ESP timestamp SyncTask -> STARTED ");
		/* client syncronization procedure */
		while (timeinfo.tm_year < (2018 - 1900))
		{
			ESP_LOGI(TAGM,"[x] Time not set, waiting...\n");
			// getting timestamp from the server for sync
			if ((server_time = (time_t) client_timesync(s)) != -1) {
				offset_time = time(NULL);
				ESP_LOGI(TAGM,"[*] Timestamp Correctly exchanged! Tempt of configuration..");
				localtime_r(&server_time, &timeinfo);
			} else {
				ESP_LOGE(TAGM,"[x] Error : Sync Fail... RETRYING! \n");
				esp_restart();
			}
		}
		// change the timezone to Italy
		setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
		tzset();

		// print the actual time in Italy
		char buffer[100];
		localtime_r(&server_time, &timeinfo);
		strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
		ESP_LOGI(TAGM,"[*] Actual time in Italy: %s", buffer);
		ESP_LOGI(TAGM,"[*] Timestamp synchronized correctly.");
	} else {
		ESP_LOGE(TAGM,"[x] Error: Impossible to reach the time-server.");
		return -1;
	}
	return 0;
}

void sniffer_init(uint8_t chnl)
{
	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
	sniffer_set_channel(chnl);
	wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_EVENT_MASK_AP_PROBEREQRECVED};
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
	esp_wifi_set_promiscuous_rx_cb(&sniffer_packet_handler);
}

void sniffer_stop(void)
{
	esp_wifi_set_promiscuous(false);
}

void sniffer_start(void)
{
	esp_wifi_set_promiscuous(true);
}

void sniffer_set_channel(uint8_t channel)
{
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
	/* http://blog.podkalicki.com/esp32-wifi-sniffer/	-> structure here!
	 * il PACCHETTO completo si compone di [RX_CONTROL e PAYLOAD]
	 * il PAYLOAD a sua volta si compone di un header(wifi_ieee80211_packet_t)
	 * e di un altro livello di payload (wifi_ieee80211_packet_t)
	 * */
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;				//wifi_promiscuous_pkt_t-> PACCHETTO
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;     //wifi_ieee80211_packet_t-> PACCHETTO->PAYLOAD->payload
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;									//wifi_ieee80211_packet_t-> PACCHETTO->PAYLOAD->header

	uint8_t result[16];
	int mask;

	//RETURN if no MGMT
	if (type != WIFI_PKT_MGMT)
		return;

	mask = 0xFF00;
	if ((ntohs(hdr->frame_ctrl) & mask) == PROBE_MASK)
	{
		fprintf(fSniffs, "Type=MGMT,");
		fprintf(fSniffs, " SubType=PROBE,");

		md5((uint8_t*) ipkt, sizeof(wifi_ieee80211_packet_t), result); //FEDE

		fprintf(fSniffs, " RSSI=%02d,"
			   " SRC=%02x:%02x:%02x:%02x:%02x:%02x,"
			   " seq_num=%d,"
			   " TIME=%llu,"
			   " HASH=%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x,",
			   ppkt->rx_ctrl.rssi,
			   hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
			   hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
			   (int)hdr->sequence_ctrl,
			   (unsigned long long) server_time_available + (unsigned long long) time(NULL) - (unsigned long long) offset_time_available,
			   result[0],result[1],result[2],result[3],result[4],result[5],result[6],result[7],
			   result[8],result[9],result[10],result[11],result[12],result[13],result[14],result[15]
		);

		uint8_t SSID_len =(uint8_t) ppkt->payload[25];
		char ssid_str[SSID_len];
		/* Remember that:
		 * if SSID_len == 0
		 * + Wildcard SSID or Null Probe Request.
		 * else
		 * + DIRECT PROBE REQUEST
		 * */
		if (SSID_len!=0)
		{
			//get the ssid
			for(int j=0; j<SSID_len; j++)
				ssid_str[j] = (char) ppkt->payload[26+j];
			ssid_str[SSID_len] = '\0';
			fprintf(fSniffs," SSID_id=%d,"
				   " SSID_lenght=%u,"
				   " SSID=%s,",
				   ppkt->payload[24],
				   SSID_len,
				   ssid_str
			);
		} else {
			fprintf(fSniffs," SSID_id=%d,"
				   " SSID_lenght=%u,"
				   " SSID=%s,",
				   ppkt->payload[24],
				   SSID_len,
				   "UNKNOWN"
			);
		}

		uint8_t supp_rates_len = ppkt->payload[25 + SSID_len + 2];
		/*printf(" supp_rates_id= %x,"
			   " supp_rates_len= %d,",
			   ppkt->payload[25 + SSID_len + 1],
			   supp_rates_len);
		*/

		/* if DIRECT PROBE then we have info about:
		 * Supported rates (not usefull for us)
		 * and HT capabilities
		 * usefull for some statistics*/
		int ht_start = 25 + SSID_len + 2 + supp_rates_len;
		uint8_t HT_len = ppkt->payload[ht_start+2];
		if(HT_len != 0)
		{
			fprintf(fSniffs," HT_id=%x,"
					" HT_cap_len=%d,"
					" HT_cap_str=",
					ppkt->payload[ht_start+1],
					HT_len);
			//get the ssid
			for(int j=0; j<HT_len; j++)
				fprintf(fSniffs,"%x",ppkt->payload[ht_start+3+j]);
		}else{
			fprintf(fSniffs," HT_id = %x", HT_len);
		}
		fprintf(fSniffs,"\n");
	}
}

/* Configure the IOMUX register for pad BLINK_GPIO (some pads are
 * muxed to GPIO on reset already, but some default to other
 * functions and need to be switched to GPIO. Consult the
 * Technical Reference for a list of pads and their default
 * functions.)
*/
void blink_task(void *pvParameter)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
    	if (BLINK_TIME >= 1000)
    	{
    		//SWITCH OFF the led (disconnected from the network)
    		gpio_set_level(BLINK_GPIO, 0);
    		vTaskDelay(500 / portTICK_RATE_MS);
    	}
    	else if (BLINK_TIME == 100)
    	{
    		//SWITCH ON the led (connected to the network)
    		gpio_set_level(BLINK_GPIO, 1);
    		vTaskDelay(500 / portTICK_RATE_MS);
     	}
    	else
    	{
    		/* Blink off (output low) */
			gpio_set_level(BLINK_GPIO, 0);
			vTaskDelay(BLINK_TIME / portTICK_RATE_MS);
			/* Blink on (output high) */
			gpio_set_level(BLINK_GPIO, 1);
			vTaskDelay(BLINK_TIME / portTICK_RATE_MS);
    	}
    }
}

/*
 * Sniffing in AP mode the network (in a given channel)
 * looking for PROBE_REQ from other devices.
 * Those will be stored in a certain file ready to be sent
 * every one minute by the main Task
*/
void sniff_task(void *pvParameter)
{
	uint8_t channel = 0;
	sscanf(DEFAULT_CHANNEL, "%" SCNu8, &channel);
	sniffer_init(channel);
	ESP_LOGI(TAGM,"\r=================================================================\n\r	");
	ESP_LOGI(TAGM,"\r(+) Start sniffing in: %s\n"
					"\rCHANNEL: %d \n"
					"\rDURATION: %d \n"
					"vTYPE PKG: Probe/Beacons \n",
					DEFAULT_SSID, channel, (TIMEOUT/1000));
	while (1)
	{
		server_time_available = server_time; //safe update of server_time_available
		offset_time_available = offset_time;
		if (id_sniFile == 0)
		{
			/* 1 is the second file is selected
			 *  return to update the first one */
			//pthread_mutex_lock(&mutex);
			while (xSemaphoreTake( xMutex, ( TickType_t ) 500 ) != pdTRUE) {
				ESP_LOGW(TAGM,"tentativo di prendere il mutex - sniff_task");
			}
			ESP_LOGW(TAGM,"MUTEX PRESO! - sniff_task");
			
			id_sniFile++;
			ESP_LOGI(TAGM,"Sniffing file %d\n", id_sniFile);

			//pthread_mutex_unlock(&mutex);
			xSemaphoreGiveFromISR(xMutex, NULL);
			ESP_LOGW(TAGM,"MUTEX RILASCIATO - sniff_task");
			fSniffs = fopen(PATH_second, "w");
		}
		else
		{
			/* otherwise write on select the
			 * and start write it */
			//pthread_mutex_lock(&mutex);
			while (xSemaphoreTake( xMutex, ( TickType_t ) 500 ) != pdTRUE) {
				ESP_LOGW(TAGM,"tentativo di prendere il mutex - sniff_task");
			}
			ESP_LOGW(TAGM,"MUTEX PRESO! - sniff_task");
			
			id_sniFile--;
			ESP_LOGI(TAGM,"Sniffing file %d\n", id_sniFile);
			
			//pthread_mutex_unlock(&mutex);
			xSemaphoreGiveFromISR(xMutex, NULL);
			ESP_LOGW(TAGM,"MUTEX RILASCIATO - sniff_task");
			fSniffs = fopen(PATH_first, "w");
		}
		esp_wifi_set_promiscuous(true);
		vTaskDelay(TIMEOUT);
		esp_wifi_set_promiscuous(false);
		fclose(fSniffs);
		ESP_LOGW(TAGM,"[*] END OF SNIFF -> START TRASFERRING");
		xEventGroupSetBits(sniff_event_group, SNIFFEND_BIT);
	}

	sniffer_stop();
}

/*
 * File send task
 * this task is always up since the sync process started
 * the functioning is actived by the event of SNIFFED_BIT every minute.
 * It takes the file saved by the sniffing process and sends
 * it throught the existing socket(s) to the server
*/
void filesend_task(void *pvParameter)
{
	while(1)
	{
		//sync of timestamp (after the first time the socket is just open for sending the packages!)
		if (s >= 0)
		{

			ESP_LOGI(TAGM,"[+] Sniffing the network for sending data to server. \n");
			xEventGroupWaitBits(sniff_event_group, SNIFFEND_BIT, true, true, portMAX_DELAY);
			//sending sniffed packages
			int result = 0;
			//pthread_mutex_lock(&mutex);
			while (xSemaphoreTake( xMutex, ( TickType_t ) 500 ) != pdTRUE) {
				ESP_LOGW(TAGM,"tentativo di prendere il mutex - filesend_task");
			}
			ESP_LOGW(TAGM,"MUTEX PRESO! - filesend_task");
			if (id_sniFile == 0) {
				ESP_LOGI(TAGM,"Sending file %d\n", id_sniFile);
				result = send_sniffed_packages(s, PATH_first);
			}
			else {
				ESP_LOGI(TAGM,"Sending file %d\n", id_sniFile);
				result = send_sniffed_packages(s, PATH_second);
			}
			//pthread_mutex_unlock(&mutex);
			xSemaphoreGiveFromISR(xMutex, NULL);
			ESP_LOGW(TAGM,"MUTEX RILASCIATO - filesend_task");
			ESP_LOGI(TAGM,"(+) send_sniffed_packages returned: %d", result);
			if (result == -1)
			{
				ESP_LOGE(TAGM,"[x] Error: Problem opening file! I can't do anything. REBOOTING");
				close(s);
				esp_restart();
			}
		} else
		{
			ESP_LOGE(TAGM,"[x] Error: Connection fail with server. REBOOTING");
			close(s);
			esp_restart();
		}
	}
}
