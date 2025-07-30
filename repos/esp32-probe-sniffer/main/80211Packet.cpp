#include "80211Packet.h"

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
	switch(type) {
	case WIFI_PKT_MGMT: return "MGMT";
	case WIFI_PKT_DATA: return "DATA";
	default:
	case WIFI_PKT_MISC: return "MISC";
	}
}

string packetSubtype2Str(uint8_t subtype) {
	switch (subtype) {
		case WIFI_MGMT_PROBE_REQ: return string("PROBE REQ");
		default: 
			return string("");
	}
}

//debug
void dumpPacket(const wifi_promiscuous_pkt_t *ppkt, int payloadSize) {
	for (int i = 0; i < payloadSize; i++) {
	        cout << setfill('0') << setw(2) << hex << static_cast<unsigned int>(ppkt->payload[i]);
	        if (i %15 == 0) {
	            cout << endl;
	        }
	}
	    cout << "-----------------------------------------" <<endl;
	    for (int i = 0; i < payloadSize; i++) {
	            cout << static_cast<char>(ppkt->payload[i]);
	            if (i %15 == 0) {
	                cout << endl;
	            }
    }
}