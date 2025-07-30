#ifndef PDSPROJECT_CPPJSON_H
#define PDSPROJECT_CPPJSON_H

#include "cJSON.h"

/**
  * Wrapper RAI class for cJSON pointer
  */  
class CppJSON {
public:
	cJSON *jsonObj;
	CppJSON(void) {	jsonObj = cJSON_CreateObject(); }
	~CppJSON() {
		if (jsonObj != NULL)
			cJSON_Delete(jsonObj);
	}
};

#endif //PDSPROJECT_CPPJSON_H
