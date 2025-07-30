#ifndef PDSPROJECT_80211PACKET_H
#define PDSPROJECT_80211PACKET_H

#include "esp_wifi_types.h"
#include <string>


#define WIFI_MGMT_PROBE_REQ 0x40

#include <iostream>
#include <iomanip>

using namespace std;

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned fragment_number:4;
	unsigned sequence_number:12;
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	//unsigned tag_number:8;
	//unsigned tag_len:8;
	uint8_t tag_number;
	uint8_t tag_len;
} ssid_parameter_set;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;

	// ssid_parameter_set ssid_prmtr;
	// uint8_t tag_number;
	// uint8_t tag_len;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

typedef struct {
	uint8_t timestamp;
	unsigned beacin_interval:16;
	unsigned capability_info:16;

}wifi_iee80211_mac_body_t;

typedef struct {
	struct tm timestamp;
	wifi_promiscuous_pkt_t packet;
} attached_timestamp_packet_t;

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
string packetSubtype2Str(uint8_t subtype);
void dumpPacket(const wifi_promiscuous_pkt_t *ppkt, int payloadSize);

#endif //PDSPROJECT_80211PACKET_H
