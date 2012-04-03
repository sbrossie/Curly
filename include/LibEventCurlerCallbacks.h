/*
 * LibEventCurlerCallbacks.h
 *
 *  Created on: Mar 24, 2012
 *      Author: stephane
 */

#ifndef LIBEVENTCURLERCALLBACKS_H_
#define LIBEVENTCURLERCALLBACKS_H_


typedef struct _SockInfo {
	curl_socket_t sockfd;
	CURL *easy;
	int action;
	struct event ev;
	int evset;
} SockInfo;

extern void cleanup(int still_running, LibEventCurler * instance);
extern size_t write_handler(char *ptr, size_t size, size_t nmemb, void *userdata);
extern void event_handler(int fd, short event, void *arg);
extern void timer_cb(int fd, short kind, void *userp);
extern void clock_cb(int fd, short kind, void *userp);
extern void setsock(SockInfo*f, curl_socket_t s, CURL*e, int act, LibEventCurler * instance);
extern void addsock(curl_socket_t s, CURL *easy, int action, LibEventCurler * instance);
extern int socket_callback_from_curl(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
extern int multi_timer_cb(CURLM *multi, long timeout_ms, void * data);


#endif /* LIBEVENTCURLERCALLBACKS_H_ */
