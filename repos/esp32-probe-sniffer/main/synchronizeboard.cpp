#include "synchronizeboard.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "apps/sntp/sntp.h"
#include "esp32_pds.h"

void SynchronizeBoard::initialize_sntp(void)
{
	ip_addr_t sntp_server_ip;
	memset(&sntp_server_ip, 0, sizeof(sntp_server_ip));
	inet_pton(AF_INET, SNTP_SERVER_IP, &sntp_server_ip);
	//ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	printf("%s", ip4addr_ntoa(&sntp_server_ip.u_addr.ip4));
	sntp_setserver(0, (const ip_addr_t*)&sntp_server_ip.u_addr.ip4);
	sntp_init();
}

void SynchronizeBoard::obtain_time(void)
{
	initialize_sntp();
	// wait for time to be set
	time_t now = 0;
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
    ESP_LOGD(TAG, "Getting Time from NTP server");
	while(timeinfo.tm_year < (2018 - 1900)) {
		//ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	}
	setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
	tzset();
}

void SynchronizeBoard::print_time(void)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    while(1) {
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("The current date/time is: %s\n", strftime_buf);
        //cout << "The current date/time is: " << strftime_buf << endl;
        vTaskDelay(1*100/portTICK_PERIOD_MS);
    }
}

struct tm SynchronizeBoard::getTime(void) {
	time_t now;
	struct tm timeinfo;
	
	time(&now);
    localtime_r(&now, &timeinfo);

    return timeinfo;
}

void SynchronizeBoard::tmToCStr(struct tm time, char (&timestampCStr)[64]) {
	strftime(timestampCStr, sizeof(timestampCStr), "%c", &time);
}