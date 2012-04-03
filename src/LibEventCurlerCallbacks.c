/*
 * LibEventCurlerCallbacks.c
 *
 *  Created on: Mar 24, 2012
 *      Author: stephane
 */


#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#include <curl/curl.h>
#include <event.h>

#include "Trace.h"
#include "AbortIf.h"
#include "LibEventCurler.h"
#include "ConnectionData.h"
#include "LibEventCurlerCallbacks.h"



static void cleanup_one_connection(LibEventCurler * instance, ConnectionData* priv, CURL *easy,
		double total_time, CURLcode res, int httpStatus, int stoppedByTimeout) {
	priv->onConnectionComplete(total_time, res, httpStatus, stoppedByTimeout);
	curl_multi_remove_handle(instance->getCURLM(), easy);
	curl_easy_cleanup(easy);
	instance->decActiveHandles();
}

void cleanup(int still_running, LibEventCurler * instance) {

	int msgs_in_queue;
	struct CURLMsg * curlm_msg;
	CURL *easy;
	CURLcode res;
	int httpStatus;
	double total_time;
	ConnectionData* priv;

	while ((curlm_msg = curl_multi_info_read(instance->getCURLM(), &msgs_in_queue)) != NULL)  {

		easy = curlm_msg->easy_handle;
		res = curlm_msg->data.result;
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv);
		curl_easy_getinfo(easy, CURLINFO_TOTAL_TIME, &total_time);
		curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpStatus);
		if (curlm_msg->msg == CURLMSG_DONE) {
			cleanup_one_connection(instance, priv, easy, total_time, res, httpStatus, 0);
		}
	}

	if (still_running != -1) {
		trace_debug("CLEANUP running = %d , left = %d\n", still_running, instance->getActiveHandles());
	}

	if (instance->getActiveHandles() == 0) {
		trace_debug("Nothing is left, done!\n");
		evtimer_del(instance->getTimerEv());
		evtimer_del(instance->getClockEv());
	}
}


/**
 * CURLOPT_WRITEFUNCTION handler -- called when there is something to read from the socket attached
 *  to that easy connection
 */
size_t write_handler(char *ptr, size_t size, size_t nmemb, void *userdata) {

	int received = size * nmemb;
	CURL *curl = (CURL *) userdata;
	char* priv_in  = NULL;

	CURLcode err = curl_easy_getinfo(curl, CURLINFO_PRIVATE, &priv_in);
	ABORT_IF_CURL_FAIL("curl_easy_getinfo (CURLINFO_PRIVATE)", err);

	ConnectionData * priv = (ConnectionData *) priv_in;
	priv->onDataRead(ptr, received);

	return received;
}


/**
 * LibEvent event handler
 *
 * Called when libevent detects there is something to read/write on particular socket
 * This will call curl_multi_socket_action to trigger the corresponding easy callback (e.g write_handler)
 */
void event_handler(int fd, short event, void *arg) {

	LibEventCurler *instance = (LibEventCurler *) arg;

	trace_debug("event_handler socket = %d event = %d\n", fd, event);

	int running_handles;

	CURLMcode errm;
	int max_retry = 5;
	do {
		trace_debug("calling curl_multi_socket_action for fd %d <BEFORE>\n", fd);
		errm = curl_multi_socket_action(instance->getCURLM(), fd, 0, &running_handles);
		ABORT_IF_CURLM_FAIL("curl_multi_socket_action", errm);
		trace_debug("calling curl_multi_socket_action for fd %d <AFTER>\n", fd);
	} while (errm == CURLM_CALL_MULTI_PERFORM && max_retry-- > 0);
	cleanup(running_handles, instance);
}


/**
 * CURLMOPT_TIMERFUNCTION curl callback to set new timeout value
 */
int multi_timer_cb(CURLM *multi, long timeout_ms, void * data)
{

	LibEventCurler * instance = (LibEventCurler *) data;
	struct timeval timeout;
	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;
	trace_debug("multi_timer_cb: Setting timeout to %ld ms\n", timeout_ms);
	evtimer_add(instance->getTimerEv(), &timeout);
	return 0;
}

/**
 * LibEvent timer:
 *
 * Set to refresh curl timeout value
 */
void timer_cb(int fd, short kind, void *userp)
{
	trace_debug("timer_cb ENTER\n");
	LibEventCurler *instance = (LibEventCurler *)userp;
	int still_running;
	CURLMcode errm = curl_multi_socket_action(instance->getCURLM(),
			CURL_SOCKET_TIMEOUT, 0, &still_running);
	ABORT_IF_CURLM_FAIL("curl_multi_socket_action (CURL_SOCKET_TIMEOUT)", errm);
	cleanup(still_running, instance);
	trace_debug("timer_cb EXIT\n");
}


static void check_for_timeout(LibEventCurler *instance) {

	CURL** easys = instance->getEasyTransfers();
	CURL** tmp = &easys[0];
	CURL* easy = *tmp;
	while (easy != NULL) {
		ConnectionData* priv;
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv);
		time_t timeout = 5;

		if (priv->hasElapsed(timeout) && ! priv->isStoppedByTimeout()) {
			cleanup_one_connection(instance, priv, easy, 0.0, CURLE_OK, 0, 1);
		}
		tmp++;
		easy = *tmp;
	}
	cleanup(-1, instance);
}


void clock_cb(int fd, short kind, void *userp) {
	trace_debug("clock_cb ENTER\n");
	LibEventCurler *instance = (LibEventCurler *) userp;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	evtimer_add(instance->getClockEv(), &timeout);
	check_for_timeout(instance);
	trace_debug("clock_cb EXIT\n");
}


/**
 * Mapping between easy curl connection and socket-- setsock and addsock
 *
 */
void setsock(SockInfo*f, curl_socket_t s, CURL*e, int act, LibEventCurler * instance)
{
	trace_debug("setsock s = %d act = %d\n", s, act);

	int kind =
			(act & CURL_POLL_IN ? EV_READ : 0) |
			(act & CURL_POLL_OUT ? EV_WRITE : 0) | EV_PERSIST;
	f->sockfd = s;
	f->action = act;
	f->easy = e;
	if (f->evset) {
		event_del(&f->ev);
	}
	event_set(&f->ev, f->sockfd, kind, event_handler, instance);
	f->evset = 1;
	event_add(&f->ev, NULL);
}

void addsock(curl_socket_t s, CURL *easy, int action, LibEventCurler * instance) {

	SockInfo *fdp = (SockInfo *) calloc(1, sizeof(SockInfo));
	ABORT_IF_NULL("calloc (SockInfo)", fdp);
	setsock(fdp, s, easy, action, instance);
	curl_multi_assign(instance->getCURLM(), s, fdp);
}

void remove_sock(SockInfo *fdp) {
	if (fdp->evset) {
		event_del(&fdp->ev);
	}
	free(fdp);
}

/**
 * CURLMOPT_SOCKETFUNCTION : curl callback when something is ready for read/write/remove
 *
 * This is used
 * - to set/update mapping between socket and {easy, action}
 * - register interest in that socket through libevent
 *
 */
int socket_callback_from_curl(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {

	const char* whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

	trace_debug("socket_callback_from_curl s  = %d action = %s <START>\n", s, whatstr[action]);

	LibEventCurler * instance = (LibEventCurler *) userp;

	SockInfo *fdp = (SockInfo *) socketp;
	if (action == CURL_POLL_REMOVE) {
		remove_sock(fdp);
	} else {
		if (!fdp) {
			addsock(s, easy, action, instance);
		} else {
			setsock(fdp, s, easy, action, instance);
		}
	}
	return 0;
}

