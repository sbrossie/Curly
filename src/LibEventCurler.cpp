/*
 * LibEventCurler.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

/*
 * 1. Create a multi handle

2. Set the socket callback with CURLMOPT_SOCKETFUNCTION

3. Set the timeout callback with CURLMOPT_TIMERFUNCTION, to get to know what timeout value to use when waiting for socket activities.

4. Add easy handles with curl_multi_add_handle()

5. Provide some means to manage the sockets libcurl is using, so you can check them for activity. This can be done through your application code, or by way of an external library such as libevent or glib.

6. Call curl_multi_socket_action() to kickstart everything. To get one or more callbacks called.

7. Wait for activity on any of libcurl's sockets, use the timeout value your callback has been told

8, When activity is detected, call curl_multi_socket_action() for the socket(s) that got action. If no activity is detected and the timeout expires, call curl_multi_socket_action(3) with CURL_SOCKET_TIMEOUT

 *
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#include <curl/curl.h>
#include <event.h>

#include "Trace.h"
#include "StringUtil.h"
#include "AbortIf.h"
#include "LibEventCurler.h"
#include "ConnectionData.h"
#include "LibEventCurlerCallbacks.h"

static const long EASY_TIMEOUT = 5;


LibEventCurler::LibEventCurler() {
	activeHandles = 0;
	doneWithAllConnections = 0;
	timeoutMs = 0;
	stUtil = new StringUtil();

	event_init();
	timerEv = (struct event *) calloc(1, sizeof(struct event));
	ABORT_IF_NULL("calloc (ev)", timerEv);

	evtimer_set(timerEv, timer_cb, this);

	clockEv = (struct event *) calloc(1, sizeof(struct event));
	ABORT_IF_NULL("calloc (ev)", clockEv);

	evtimer_set(clockEv, clock_cb, this);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	evtimer_add(clockEv, &timeout);

	curlm = curl_multi_init();
	ABORT_IF_NULL("curl_multi_init", curlm);

	CURLMcode errm = curl_multi_setopt(curlm, CURLMOPT_SOCKETFUNCTION, socket_callback_from_curl);
	ABORT_IF_CURLM_FAIL("curl_multi_setopt (CURLMOPT_SOCKETFUNCTION)", errm);

	errm = curl_multi_setopt(curlm, CURLMOPT_SOCKETDATA, this);
	ABORT_IF_CURLM_FAIL("curl_multi_setopt (CURLMOPT_SOCKETDATA)", errm);

	errm = curl_multi_setopt(curlm, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
	ABORT_IF_CURLM_FAIL("curl_multi_setopt (CURLMOPT_TIMERFUNCTION)", errm);

	errm = curl_multi_setopt(curlm, CURLMOPT_TIMERDATA, this);
	ABORT_IF_CURLM_FAIL("curl_multi_setopt (CURLMOPT_TIMERDATA)", errm);

}

LibEventCurler::~LibEventCurler() {
	if (stUtil != NULL) {
		delete(stUtil);
	}
	free(timerEv);
	free(clockEv);
	free(easys);
	(void) curl_multi_cleanup(curlm);
	curlm = NULL;
	activeHandles = 0;
}


CURL *
LibEventCurler::createConnection(ConnectionData* connection) {
	CURL *curl = curl_easy_init();
	ABORT_IF_NULL("curl_easy_init", curl);

	if (curl) {
		CURLcode err = curl_easy_setopt(curl, CURLOPT_URL, connection->getUrl());
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_URL)", err);

		err = curl_easy_setopt(curl, CURLOPT_PRIVATE, connection);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_PRIVATE)", err);

		err = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION , write_handler);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_WRITEFUNCTION)", err);

		err = curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_WRITEDATA)", err);
/*
        -- does not seem to work with curl_multi --
		err = curl_easy_setopt(curl, CURLOPT_TIMEOUT, EASY_TIMEOUT);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_TIMEOUT)", err);
*/

		trace_debug("Setting easy options for  %s <END>\n", connection->getUrl());
	}
	return curl;
}

void
LibEventCurler::addConnections(ConnectionData** connections) {

	int cur = 0;
	ConnectionData* curConnection = NULL;

	easys = (CURL**) calloc(1, sizeof(CURL*));

	do {
		curConnection = connections[cur];
		if (curConnection != NULL) {
			CURL * easy = createConnection(curConnection);
			if (easy != NULL) {
				CURLMcode errm = curl_multi_add_handle(curlm, easy);
				ABORT_IF_CURLM_FAIL("curl_multi_add_handle", errm);
				activeHandles++;
				easys = (CURL**) realloc(easys, sizeof(CURL*) * activeHandles);
				easys[cur] = easy;
				cur++;
			}
		}
	} while (curConnection != NULL);
	easys = (CURL**) realloc(easys, sizeof(CURL*) * (activeHandles + 1));
	easys[activeHandles] = NULL;
}

void
LibEventCurler::loop() {
	event_dispatch();
	trace_debug("event_dispatch returned\n");

}

