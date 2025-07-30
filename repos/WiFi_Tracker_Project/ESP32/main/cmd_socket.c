#include "cmd_socket.h"

const char *TAGS = "Socket";
extern time_t server_time; //FEDE

int start_socket_connection(char *ip, char *port)
{
	uint16_t	   		tport_n, tport_h;	/* server port number (net/host order) */
	int 		   		skt;
	int		       		result;				/* variable to check the build status */
	struct sockaddr_in	saddr;				/* server address structure */
	struct in_addr		sIPaddr; 			/* server IP address. structure */

	result = inet_aton(ip, &sIPaddr);
	if (!result)
	{
		ESP_LOGE(TAGS, "[x] Error: Invalid address.");
		return -1;
	}

	if (sscanf(port, "%"SCNu16, &tport_h)!= 1)
	{
		ESP_LOGE(TAGS, "[x] Error: Invalid port number.");
		return -1;
	}
	tport_n = htons(tport_h);

    skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(skt < 0)
    {
    	ESP_LOGE(TAGS, "[x] Error: Impossible to open socket.");
        return -1;
    }

	struct timeval timeout;      
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt (skt, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		ESP_LOGE(TAGS, "setsockopt failed 1");
		esp_restart();
	}
	int val = 1;
	if(setsockopt(skt, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0) {
		ESP_LOGE(TAGS, "setsockopt failed 2");
		esp_restart();
	}

    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = tport_n;
    saddr.sin_addr = sIPaddr;

    if(connect(skt, (struct sockaddr *) &saddr, sizeof(saddr))!=0)
    {
    	ESP_LOGE(TAGS, "[x] Error: Impossible to connect with the server.");
        return -1;
    }

    return skt;
}

//-1 errore
int send_msg(int socket, char *msg)
{
	size_t	len, n;
	ESP_LOGW(TAGS,"[i] Send message!");
	strcpy(buf, msg);
	strcat(buf, "\r\n");
	len = strlen(buf);
	if ((n = write(socket, buf, len)) != len)
	{
		ESP_LOGE(TAGS,"Write error");
		esp_restart();
		return -1;
	}
	return n;
}

int client_register(int socket)
{
	int n=0;
	//int port=0;
	char register_string[20];
	uint8_t efuse_mac[6];

	/*
	 * esp_efuse_mac_get_default return base MAC address which
	 * is factory-programmed by Espressif in BLK0 of EFUSE.
	 * The original MAC address in efuse is unique
	 */
	if (esp_base_mac_addr_get(efuse_mac) != ESP_OK) {
	        esp_efuse_mac_get_default(efuse_mac);
	}

	size_t	len;
	strcpy(buf, "REGISTER(");
	sprintf(register_string, "%02x:%02x:%02x:%02x:%02x:%02x",
			efuse_mac[0],efuse_mac[1],efuse_mac[2],
			efuse_mac[3],efuse_mac[4],efuse_mac[5]);
	strcat(buf, strcat(register_string,")\r\n"));
	len = strlen(buf);
	if(write(socket, buf, len) != len)
	{
		ESP_LOGE(TAGS,"Write error");
		return -1;
	}

	ESP_LOGI(TAGS,"waiting for the response...");
	n=recv(socket, rbuf, BUFLEN-1, 0);
	if (n==0)
	{
		ESP_LOGE(TAGS, "Error: Server shut-down during transfering, data will be broken.\n");
		return -1;
	}
	else if(n<0)
	{
		ESP_LOGE(TAGS,"Write Error.");
		return -1;
	}
	else {
		rbuf[n]='\0';
		//ESP_LOGI(TAGS,"Received : [%s]\n", rbuf);
		if(strstr(rbuf,"ACCEPT") != NULL)
		{
			ESP_LOGI(TAGS,"[*] ACCEPT ok!");
			//sscanf(rbuf, "OKPORT(%d)\r\n", &port);
			//port = ntohs(port);
		}
		return 0;
	}
	return -1;
}

//ritorna 0 se fallisce, il timestamp se ha successo
unsigned long int client_timesync(int socket)
{
	int n=0;
	unsigned long int timestamp = 0;

	while(!(strcmp(rbuf,"CLOSE")==0))
	{
		size_t	len;
		ESP_LOGI(TAGS,"waiting for the message from the server...");
		n=recv(socket, rbuf, BUFLEN-1, 0);
		if (n==0)
		{
			ESP_LOGE(TAGS,"Error: Server shut-down during transfering, data will be broken.");
			break;
		}
		else if(n<0)
		{
			ESP_LOGE(TAGS,"Write Error.");
			break;
		}
		else
		{
			rbuf[n]='\0';
			//ESP_LOGI(TAGS,"Received : [%s]\n", rbuf);

			if (strstr(rbuf, "PING") != NULL) {
				ESP_LOGI(TAGS,"[*] PING received!");
				strcpy(buf, "PONG\r\n");
				len = strlen(buf);
				if(write(socket, buf, len) != len)
				{
					ESP_LOGE(TAGS,"Write error");
					return 0;
				}
				ESP_LOGI(TAGS, "[*] PONG sent!");
			}

			if (strstr(rbuf,"CLOCK") != NULL)
			{
				ESP_LOGI(TAGS,"[*] CLOCK ok!");
				sscanf(rbuf, "CLOCK(%lu)\r\n", &timestamp);
				ESP_LOGI(TAGS, "Received timepstamp: %lu", timestamp);
				return timestamp;
			}
		}
	}
	return 0;
}


int send_sniffed_packages(int skt, const char *path){
	uint32_t size=0,modification_ms=0;
	size_t	msg_len;
	struct stat st;
	char success='t';

	FILE *f = fopen(path, "r");
	if (f == NULL) {
		ESP_LOGE(TAGS,"(!) Error: File does not exists.");
		return -1;
	}

	/* get the required information then (size & date) */
	if (stat(path, &st) == -1) {
		/* if impossible to get the info then set to 0 */
		ESP_LOGE(TAGS,"(!) Error: Impossible to get Info about the file.");
		success='f';
	} else {
		/* insert information in the designed variables */
        modification_ms = st.st_mtime;
		modification_ms = modification_ms + server_time;
        size= st.st_size;
    }

	ESP_LOGI(TAGS,"(+) Sending the file to Server through TCP Socket.");

	/* Send a message with this format - |FILE|CL|LF|B1|B2|B3|B4|T1|T2|T3|T4|
	 * where B is the size and T the last time it has been modified */
	//sending |FILE|
	strcpy(buf, "FILE\r\n");
	msg_len = strlen(buf);
	int res=0;
	//send thge starting info about the file to the server
	res = write(skt, buf, msg_len);
	ESP_LOGI(TAGS,"(+) DOPO n= %d", res);
	if (res != msg_len || res == -1) {
		ESP_LOGE(TAGS,"(!) Error: Fail in sending header.");
		success='f';
		esp_restart();
	}
	
	/* send the size to the client */
	//sending |B1|B2|B3|B4|
	size = htonl(size);
	if (write(skt, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
		ESP_LOGE(TAGS,"(!) Error: Fail in sending size.");
		success='f';
		esp_restart();
	}
	
	/* send last modification date */
	//sending |T1|T2|T3|T4|
	modification_ms = htonl(modification_ms);
	if(write(skt,&modification_ms, sizeof(uint32_t)) != sizeof(uint32_t)){
		ESP_LOGE(TAGS,"(!) Error: Fail in sending last_modification_time.");
		success='f';
		esp_restart();
	}

	/* file send loop */
	/* send the file BUFLEN byte at time to the client until the sended
	 * size is equal to the one obtained before from the file*/
	uint32_t sendn_size=0;
	ESP_LOGI(TAGS,"(+) ntohl(size)_u = %u", ntohl(size));
	while (sendn_size < ntohl(size)) {
		msg_len = fread(buf, 1, BUFLEN, f);
		if (write(skt, buf, msg_len) != msg_len) {
			ESP_LOGE(TAGS,"(!) Error: Fail in sending file content.");
			success='f';
			esp_restart();
			break;
		 }
		 sendn_size += msg_len;
		 ESP_LOGI(TAGS,"(+) sendn_size = %u", sendn_size);
	}
	ESP_LOGI(TAGS,"(+) DOPO del FILE vero e proprio");
	if (f != NULL)
		fclose(f);
	ESP_LOGI(TAGS,"(+) PRIMA del RETURN");
	if (success != 't')
		return -1;

	ESP_LOGI(TAGS,"(*) File sent Succesfully.");
	return 0;

}
