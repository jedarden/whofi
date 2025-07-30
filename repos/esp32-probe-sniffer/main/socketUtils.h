#ifndef PDSPROJECT_SOCKETUTILS_H
#define PDSPROJECT_SOCKETUTILS_H

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include <iostream>
#include <string.h>
#include "esp_log.h"
#include "esp32_pds.h"

#define NO_FLAGS 0

using namespace std;

/**
  * @brief     wrapper to send data of size n in more than one send if required
  *
  * @param     fd 		socket where sending data
  * @param     vptr		data buffer
  * @param 	   n 		size of vptr
  * @param 	   flags 	send flags
  *
  * @return
  *    - -1 if erro
  *	   - size of sent data
  */
ssize_t sendn (int fd, const void *vptr, size_t n, int flags);

/**
  * @brief     
  *
  * @param     sockPtr socket pointer of connection to the server SERVER_ADDR on
  *			   SERVER_PORT
  *
  * @return
  *    - ESP_OK: succeed
  *	   - ESP_FAIL: fail
  */
esp_err_t connect_to_server(int *sockPtr, bool handshake);

#endif //PDSPROJECT_SOCKETUTILS_H
