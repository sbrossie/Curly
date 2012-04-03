/*
 * ConnectionData.h
 *
 *  Created on: Mar 21, 2012
 *      Author: stephane
 */

#ifndef CONNECTION_DATA_H_
#define CONNECTION_DATA_H_

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "AbortIf.h"

class ConnectionData {

private:
	char* url;
	char* data;
	double totalTime;
	int nbBytes;
	CURLcode res;
	int httpStatus;
	time_t startTime;
	int stopppedByTimeout;

public:
	ConnectionData(char* url);
	virtual ~ConnectionData();

	inline char* getUrl() {
		return url;
	}

	inline int getHttpStatus() {
		return httpStatus;
	}

	inline int getNbBytes() {
		return nbBytes;
	}

	inline int hasElapsed(time_t timeout) {
		time_t currentTime;
		(void) ctime(&currentTime);
		return (startTime + timeout < currentTime) ? 1 : 0;
	}

	inline int isStoppedByTimeout() {
		return stopppedByTimeout;
	}

	void onDataRead(char* input, size_t size);
	void onConnectionComplete(double time, CURLcode r, int status, int stoppedByTimeout);
};

inline ConnectionData::ConnectionData(char* input) {
	url = strdup(input);
	nbBytes = 0;
	totalTime = 0;
	httpStatus = 0;
	res = CURLE_OK;
	data = NULL;
	stopppedByTimeout = 0;
	(void) ctime(&startTime);
}


inline ConnectionData::~ConnectionData() {
	free(url);
	free(data);
}

#endif /* CONNECTION_DATA_H_ */
