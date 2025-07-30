#ifndef PDSPROJECT_CONSUMERTASK_H
#define PDSPROJECT_CONSUMERTASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include <iostream>
#include <map>
#include <iomanip>
#include <sstream>
#include <memory>
#include "esp_wifi.h"
#include "esp_log.h"
#include "80211Packet.h"
#include "string.h"
#include "mbedtls/md5.h"
#include "CppJSON.h"
#include "cJSON.h"
#include "ProbeReq.h"
#include "socketUtils.h"
#include "esp_heap_trace.h"
#include "esp32_pds.h"
#include "synchronizeboard.h"

// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG                   /* level of logging */

#define NUM_RECORDS 100

// static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM


using namespace std;

class ConsumerTask {
public:
	enum Keys {
		SOCKET = 1,
		RINGBUFFER = 2
	};

	ConsumerTask(int socket, RingbufHandle_t packetRingBuffer);

	//  Not useful for now because the consumer is not designed to terminate
	~ConsumerTask();
	/**
	  * @brief extract packet from Ringbuffer     
	  *
	  * @param     args 		map conteining socket where sending data and ringBuffer pointer
	  */
	static void consume(void *args);

private:
	TaskHandle_t consumerHandle;

	/**
	  * @brief Utility function that given raw sniffed produce a probeReq class
	  *
	  * @param     probePacket	raw sniffed packet
	  * @param     retProbeReq	probeReq return pointer to instance
	  * @return    
	  *    - false probePacket in not a probe request so value in retProbeReq is invalid
	  *    - true  probePacket is a probe request
	  */  
	static bool consumeSniffedPacket(void *probePacket, unique_ptr<ProbeReq>& retProbeReq);
};

#endif //PDSPROJECT_CONSUMERTASK_H
