/*
 * ConnectionData.cpp
 *
 *  Created on: Mar 21, 2012
 *      Author: stephane
 */

#include "ConnectionData.h"
#include "Trace.h"

void ConnectionData::onDataRead(char* input, size_t size) {
	data = (data == NULL) ? (char* ) malloc(size) : (char* ) realloc(data, (nbBytes + size));
	ABORT_IF_NULL("malloc/realloc data", data);
	(void) memcpy(data + nbBytes, input, size);
	nbBytes += size;
	trace_debug("Got %d bytes for connection %s\n", nbBytes, url);
}

void ConnectionData::onConnectionComplete(double time, CURLcode r, int status, int stoppedByTimeout) {
	totalTime = time;
	res  = r;
	httpStatus = status;
	stopppedByTimeout = 1;
}
