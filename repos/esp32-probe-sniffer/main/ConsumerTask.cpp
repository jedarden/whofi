#include "ConsumerTask.h"

ConsumerTask::ConsumerTask(int socket, RingbufHandle_t packetRingBuffer) {
    // boxing task args
    map<int, void*> *args = new map<int, void*>;
    args->insert(pair<int, void*>(ConsumerTask::Keys::SOCKET, (void *) socket));
    args->insert(pair<int, void*>(ConsumerTask::Keys::RINGBUFFER, (void *) packetRingBuffer));
    // args[1] = (void*) socket;
    // Create consumer task
    xTaskCreate(ConsumerTask::consume, "consumer_task", STACK_SIZE,
            static_cast<void*>(args), tskIDLE_PRIORITY, &(this->consumerHandle));                
}

//  Not useful for now because the consumer is not designed to terminate
// ConsumerTask::~ConsumerTask() {

    // ToDo wait termination with Event_Group

    // if( this->consumerHandle != NULL )
    //  {
    //      vTaskDelete( xHandle );
    //  }
// }


void ConsumerTask::consume(void *args) {
    ESP_LOGI(TAG, "Consumer task created");
    map<int, void*> *argsMap = static_cast<map<int, void*>*> (args);
    auto it = argsMap->find(ConsumerTask::Keys::SOCKET);
    assert(it != argsMap->end() && "error missing socket argument");
    int socket = (int) it->second;
    ESP_LOGD(TAG, "socket: %d", socket);
    it = argsMap->find(ConsumerTask::Keys::RINGBUFFER);
    assert(it != argsMap->end() && "error missing ringbuffer argument");
    RingbufHandle_t packetRingBuffer = (RingbufHandle_t) it->second;
    delete argsMap;

    while (1) {
        size_t packetSize;  

        // task yield in order to context switch to other task if neccessary and hence resetting
        // the watchdog because if many packets are ready i will not put the thread in sleeping queue
        // vTaskDelay(portTICK_PERIOD_MS);
        taskYIELD();
        void *probePacket = xRingbufferReceive(packetRingBuffer, &packetSize, portMAX_DELAY);
        if (probePacket == NULL) {
            ESP_LOGE(TAG, "Error retrieving element from queue\n");
            continue;
        }
        // ESP_LOGD(TAG, "(Core %d) Extracted element from queue, size: %d",xPortGetCoreID(), packetSize);        
        // if not a probe req i will discard packet
        unique_ptr<ProbeReq> probePtr;
        if (consumeSniffedPacket(probePacket, probePtr) == false) {
            // remove packet from ringbuffer
            vRingbufferReturnItem(packetRingBuffer, probePacket);
            ESP_LOGD(TAG, "discarded remaining size %d", xRingbufferGetCurFreeSize(packetRingBuffer));
            continue;
        }
        cout << probePtr.get();
        // stringstream ss;
        // ss << *probePtr;
        // string str = ss.str();
        // char *sendStr = new char[str.length() + 1];
        // strcpy(sendStr, str.c_str());
        unique_ptr<CppJSON> toSendCppJSONPtr = probePtr->toJson();
        char *sendStr = cJSON_Print(toSendCppJSONPtr->jsonObj);
        // char *sendStr = NULL;
        if (sendStr == NULL)
        {
            ESP_LOGW(TAG, "Failed to send JSON");
        } else {
            // ESP_LOGD(TAG, "%s", sendStr);
            while (send(socket, sendStr, strlen(sendStr), 0) == -1) {
                ESP_LOGE(TAG, "Error sending sniffed packet info to server");
                close(socket);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                ESP_ERROR_CHECK(connect_to_server(&socket, true));
                // connect_to_server(&socket);
            }
            free(sendStr);
        }
        vRingbufferReturnItem(packetRingBuffer, probePacket);
        ESP_LOGD(TAG, "sended remaining size %d", xRingbufferGetCurFreeSize(packetRingBuffer));
    }
}

bool ConsumerTask::consumeSniffedPacket(void *probePacket, unique_ptr<ProbeReq>& retProbeReq) {
    char buf[32 + 1];    
    uint8_t subtype;
    int8_t rssi;
    uint8_t channel;
    uint8_t ssidLen;
    unsigned char md5digest[16];
    string ssid;

    const attached_timestamp_packet_t *ppkt_with_tm = (attached_timestamp_packet_t*) probePacket;
    const wifi_promiscuous_pkt_t *ppkt = &(ppkt_with_tm->packet);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    /* filter only PROBE REQ packets */
    subtype = hdr->frame_ctrl;
    subtype = subtype & 0xF0;
    // if (subtype != 0x40) {
    //     // ESP_LOGD(TAG, "remaining size %d", xRingbufferGetCurFreeSize(packetRingBuffer));
    //     return false;
    // }
    // i don't know why but last 4 bytes (of FCS i think) are different for any ESP receiver
    // also other 24 byte are different
    int payloadSize = ppkt->rx_ctrl.sig_len - 28;
    // debug
    // dumpPacket(ppkt, payloadSize);
    // uint8_t *payloadHash = new uint8_t[payloadSize];
    // memset(payloadHash, 0, payloadSize);
    // memcpy(payloadHash, ppkt->payload, payloadSize);
    // extracting packet info from the payload
    rssi = ppkt->rx_ctrl.rssi;
    // take channel byte and filter useful bits
    channel = ppkt->rx_ctrl.channel;
    channel &= 0xF0;
    // i don't know why but last 4 bytes (of CRC i think) are different for any ESP receiver
    // calculating md5 packet digest
    mbedtls_md5((const unsigned char *) ppkt->payload, payloadSize, md5digest);
    // copy from ssid begin a length of ssid length specified in the payload 
    ssidLen = ppkt->payload[25];
    ESP_LOGD(TAG, "ssid len: %d", ssidLen);
    if (ssidLen <= 32)  {
        memcpy(buf, &(ppkt->payload[26]), (size_t) ppkt->payload[25]);
        buf[ppkt->payload[25]] = '\0';
        ESP_LOGD(TAG, "%s", buf);
        ssid = buf;
    } else {
        ssid = "";
    }
    retProbeReq.reset(new ProbeReq (
                ProbeReq::Builder()
                .withType(WIFI_PKT_MGMT)
                .withSubtype(subtype)
                .withChannel(ppkt->rx_ctrl.channel)
                .withRssi(rssi)
                // .withSsid((char *) &(ppkt->payload[26]), ppkt->payload[25])
                .withSsid2(ssid)
                .withSourceAddress(hdr->addr2)
                .withDestAddress(hdr->addr1)
                .withBssid(hdr->addr3)
                .withMd5digest(md5digest)
                .withSequenceNumber(hdr->sequence_number)
                .withTimestamp(ppkt_with_tm->timestamp)
                .build()
            ));

    return true;
}
