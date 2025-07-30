#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include <esp_http_server.h>
#include "rom/ets_sys.h"

#define CONFIG_PACKET_RATE 350
#define PORT 2223

char data[256] =
    "01230123012301230123012301230123012301230123012301230123012301230123012301"
    "230123012301230123012301230123012301230123012301230123";

void socket_transmitter_sta_loop(bool (*is_wifi_connected)()) {
    int socket_fd = -1;
    while (1) {
        close(socket_fd);
        char *ip = "192.168.4.1";
        struct sockaddr_in caddr;
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(PORT);
        while (!is_wifi_connected()) {
            // wait until connected to AP
            printf("waiting\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        if (inet_aton(ip, &caddr.sin_addr) == 0) {
            printf("ERROR: inet_aton\n");
            continue;
        }

        socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            printf("ERROR: Socket creation error [%s]\n", strerror(errno));
            continue;
        }
        if (connect(socket_fd, (const struct sockaddr *) &caddr, sizeof(struct sockaddr)) == -1) {
            printf("ERROR: socket connection error [%s]\n", strerror(errno));
            continue;
        }

        int index = 1;
        while (1) {
            if (!is_wifi_connected()) {
                printf("ERROR: wifi is not connected\n");
                break;
            }

            if (sendto(socket_fd, &data, index, /*flags=*/0, (const struct sockaddr *) &caddr, sizeof(caddr)) !=
                index) {
                vTaskDelay(1);
                continue;
            }
            vTaskDelay(2);
            vTaskDelay(20); // This limits TX to approximately 100 per second.
            if (index >= 255)
              index = 1;
            else
              index += 5;
        }
    }
}

void udp_server_task(void *pv) {
  char rx_buffer[256];
  char addr_str[128];
  int addr_family = AF_INET;
  int ip_protocol = 0;

  while (1) {
    struct sockaddr_in dest_addr_ip4;
    dest_addr_ip4.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4.sin_family = AF_INET;
    dest_addr_ip4.sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
      break;
    }

    int err = bind(sock, (struct sockaddr *)&dest_addr_ip4, sizeof(dest_addr_ip4));
    if (err < 0) {
      break;
    }

    while (1) {
      struct sockaddr_in source_addr;
      socklen_t socklen = sizeof(source_addr);
      int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
      if (len < 0)
        break;

      else {
        rx_buffer[len] = 0;

        int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        if (err < 0)
          break;
      }
    }
  }
}
