/*
 * LibEventCurler.h
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

#ifndef LIBEVENTCURLER_H_
#define LIBEVENTCURLER_H_

#include <curl/curl.h>
#include "ConnectionData.h"
#include "StringUtil.h"


class LibEventCurler {
private:
	CURLM* curlm;
	int activeHandles;
	int doneWithAllConnections;
	StringUtil* stUtil;
	long timeoutMs;
	struct event* timerEv;
	struct event* clockEv;
	CURL** easys;

private:
	CURL* createConnection(ConnectionData* connection);

public:
	LibEventCurler();
	virtual ~LibEventCurler();

	void addConnections(ConnectionData** connections);

	void loop();

	inline CURLM* getCURLM() {
		return curlm;
	}

	inline CURL** getEasyTransfers() {
		return easys;
	}

	inline struct event* getTimerEv() {
		return timerEv;
	}

	inline struct event* getClockEv() {
		return clockEv;
	}

	inline int getActiveHandles() {
		return activeHandles;
	}

	inline void decActiveHandles() {
		activeHandles--;
	}

};

#endif /* LIBEVENTCURLER_H_ */
