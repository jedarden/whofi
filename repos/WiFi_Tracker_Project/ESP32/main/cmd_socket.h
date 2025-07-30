#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_aton()
#include <netdb.h>
//for the input-port cast
#ifndef SCNu8
  #define SCNu8 "hhu"
#endif

//Socket Group
#define BUFLEN	128  	/* BUFFER LENGTH */
char    buf[BUFLEN]; 	/* transmission buffer */
char    rbuf[BUFLEN];   /* transmission buffer */

//METHODS
int send_msg(int socket, char *msg);
int start_socket_connection(char * ip, char *port);
int client_register(int socket);
unsigned long int client_timesync(int socket);
int send_sniffed_packages(int skt, const char *path);
time_t server_time; //FEDE
