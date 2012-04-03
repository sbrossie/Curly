/*
 * SelectCurler.h
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

#ifndef SELECTCURLER_H_
#define SELECTCURLER_H_


#include <curl/curl.h>

#include "StringUtil.h"


class SelectCurler {
private:
	CURLM* curlm;
	int activeHandles;
	int doneWithAllConnections;
	StringUtil* stUtil;

private:
	int turnTheCrank();
	CURL* createConnection(const char *url);
	void cleanupIfNeeded(int running_handles);

public:
	SelectCurler();
	virtual ~SelectCurler();

	void addConnections(char* connections[]);
	int doSelect(fd_set* readFdSet, fd_set* writeFdSet, fd_set* execFdSet);
};

#endif /* SELECTCURLER_H_ */
