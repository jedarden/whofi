#include "ProbeReq.h"

using namespace std;

ProbeReq::Builder& ProbeReq::Builder::withType(wifi_promiscuous_pkt_type_t type) {
	this->type = type;
	return *this;	
}

ProbeReq::Builder& ProbeReq::Builder::withSubtype(uint8_t subtype) {
	this->subtype = subtype;
	// cout << this << endl;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withChannel(uint8_t channel) {
	this->channel = channel;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withRssi(int8_t rssi) {
	this->rssi = rssi;
    // cout << this << endl;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withSsid(const char *ssid, uint8_t ssidLen) {
	string ssidStr(ssid, (size_t) ssidLen);
	this->ssid = move (ssidStr);
	this->ssidLen = ssidLen;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withSsid2(string ssid) {
	this->ssid = ssid;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withDestAddress(const uint8_t destAddress[6]) {
	// this->destAddress = destAddress;
    copy(destAddress, destAddress+6, begin(this->destAddress));
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withSourceAddress(const uint8_t sourceAddress[6]) {
	copy(sourceAddress, sourceAddress+6, begin(this->sourceAddress));
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withBssid(const uint8_t bssid[6]) {
    copy(bssid, bssid+6, begin(this->bssid));
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withMd5digest(const unsigned char md5digest[16]) {
    copy(md5digest, md5digest+16, begin(this->md5digest));
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withTimestamp(struct tm timestamp) {
	this->timestamp = timestamp;
	return *this;
}

ProbeReq::Builder& ProbeReq::Builder::withSequenceNumber(uint16_t sequence_number) {
	this->sequence_number = sequence_number;
	return *this;
}

ProbeReq ProbeReq::Builder::build(void) {
	return ProbeReq(type, subtype, channel, rssi, ssid, ssidLen,
			move(destAddress), move(sourceAddress), move(bssid),
			sequence_number, move(md5digest), timestamp);
}

unique_ptr<CppJSON> createTmJSON(struct tm timestamp) {
	unique_ptr<CppJSON> tmJSONPtr(new CppJSON);
	if (tmJSONPtr->jsonObj == NULL)
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_sec", timestamp.tm_sec) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_min", timestamp.tm_min) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_hour", timestamp.tm_hour) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_mday", timestamp.tm_mday) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_mon", timestamp.tm_mon) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_year", timestamp.tm_year) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_wday", timestamp.tm_wday) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_yday", timestamp.tm_yday) == NULL) 
		return nullptr;
	if (cJSON_AddNumberToObject(tmJSONPtr->jsonObj, "tm_isdst", timestamp.tm_isdst) == NULL)
		return nullptr;

	return tmJSONPtr;
}

unique_ptr<CppJSON> ProbeReq::toJson(void) {
	unique_ptr<CppJSON> rootPtr(new CppJSON);
	char sourceAddressCStr[17+1];
	char md5digestCStr[16+1];
	stringstream ss;

	for(size_t i = 0; i < sourceAddress.size(); i++) {
		ss << uppercase << setfill('0') << setw(2) << hex 
				<< static_cast<unsigned int>(sourceAddress[i]);
		if (i != 5) {
			ss << ':';
		}
	}
	strcpy (sourceAddressCStr, ss.str().c_str());
	if (cJSON_AddStringToObject(rootPtr->jsonObj, ProbeReq::Keys::SADDR, sourceAddressCStr) == NULL)
		return nullptr;
	if (cJSON_AddStringToObject(rootPtr->jsonObj, ProbeReq::Keys::SSID, ssid.c_str()) == NULL)
		return nullptr;
	unique_ptr<CppJSON> timestampPtr = createTmJSON(timestamp);
	if (timestampPtr == nullptr)
		return nullptr;
	cJSON_AddItemToObject(rootPtr->jsonObj, ProbeReq::Keys::TIMESTAMP, timestampPtr->jsonObj);
	// set NULL so when timestampPtr goes out of scope in CppJSON we don't delete the timestamp cJSON struct
	// from now cJSON struct is ownership of rootPtr's cJSON
	timestampPtr->jsonObj = NULL;

	ss.str("");
	ss.clear();
	for (int i = 0; i < 16; i++) {
		ss << nouppercase << setfill('0') << setw(2) << hex << static_cast<unsigned int>(md5digest[i]);
    }
	strcpy (md5digestCStr, ss.str().c_str());
	if (cJSON_AddStringToObject(rootPtr->jsonObj, ProbeReq::Keys::MD5HASH, md5digestCStr) == NULL)
		return nullptr;
	if (cJSON_AddNumberToObject(rootPtr->jsonObj, ProbeReq::Keys::RSSI, rssi) == NULL)
		return nullptr;
	if (cJSON_AddNumberToObject(rootPtr->jsonObj, ProbeReq::Keys::SEQUENCE_NUM, sequence_number) == NULL)
		return nullptr;

	return rootPtr;
}

std::ostream& operator<<(ostream& os, const ProbeReq& probeReq){
    os << "SUBTYPE=" << packetSubtype2Str(probeReq.subtype) << ", ";
	os << "SSID=" << probeReq.ssid << ", ";
	os << "CHAN=" << setfill('0') << setw(2) << dec
		<< static_cast<unsigned int>(probeReq.channel) << ", ";
	os << "RSSI=" << setfill('0') << setw(2) << dec << static_cast<int>(probeReq.rssi);
	os << ", SOURCE=";
	for (auto iter = probeReq.sourceAddress.begin(); iter != probeReq.sourceAddress.end(); iter++) {
		const unsigned char& byte = *iter;
		os << setfill('0') << setw(2) << hex <<  static_cast<unsigned int>(byte);
		if (iter != probeReq.sourceAddress.end() - 1)
			os << ":";
	}
	os << ", DEST=";
	for (auto iter = probeReq.destAddress.begin(); iter != probeReq.destAddress.end(); iter++) {
		const unsigned char& byte = *iter;
		os << setfill('0') << setw(2) << hex << static_cast<unsigned int>(byte);
		if (iter != probeReq.destAddress.end() - 1)
			os << ":";
	}
	os << ", BSSID=";
	for (auto iter = probeReq.bssid.begin(); iter != probeReq.bssid.end(); iter++) {
		const unsigned char& byte = *iter;
		os << setfill('0') << setw(2) << hex << static_cast<unsigned int>(byte);
		if (iter != probeReq.bssid.end() - 1)
			os << ":";
	}
	char timestamp[64];
	SynchronizeBoard::tmToCStr(probeReq.timestamp, timestamp);
	os << ", timestamp=" << dec << timestamp;
	os << ", seq num: " << dec << probeReq.sequence_number;
	os << ", md5hash: ";
	for (int i = 0; i < 16; i++) {
		os << setfill('0') << setw(2) << hex << static_cast<unsigned int>(probeReq.md5digest[i]);
    }
	os << endl;

    return os;
}
	