/*
 * SelectCurler.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#include <sys/select.h>

#include "Trace.h"
#include "AbortIf.h"
#include "SelectCurler.h"



typedef struct url_transfer_private_ {
	char * url;
	char * file_name;
	FILE * f;
} url_transfer_private;


static size_t write_handler(char *ptr, size_t size, size_t nmemb, void *userdata) {

	int received = size * nmemb;

	CURL *curl = (CURL *) userdata;

	char* priv_in  = NULL;
	CURLcode err = curl_easy_getinfo(curl, CURLINFO_PRIVATE, &priv_in);
	ABORT_IF_CURL_FAIL("curl_easy_getinfo (CURLINFO_PRIVATE)", err);
	url_transfer_private * priv = (url_transfer_private *) priv_in;

	trace_debug("write_handler : received = %d bytes for %s\n", received, (char *) priv->url);
	size_t res = fwrite(ptr, size, nmemb, priv->f);
	(void) fflush(priv->f);
	return res;
}

SelectCurler::SelectCurler() {
	activeHandles = 0;
	doneWithAllConnections = 0;
	stUtil = new StringUtil();
	curlm = curl_multi_init();
	ABORT_IF_NULL("curl_multi_init", curlm);
}

SelectCurler::~SelectCurler() {
	if (stUtil != NULL) {
		delete(stUtil);
	}
	curlm = NULL;
	activeHandles = 0;
}

int
SelectCurler::turnTheCrank() {

	trace_debug("turn_the_crank <START>\n");

	int running_handles = -1;

	CURLMcode err;
	int max_retry = 3;
	do {
		err = curl_multi_perform(curlm, &running_handles);
		if (err == CURLM_CALL_MULTI_PERFORM) {
			// STEPH should wait in between ?
			trace_debug("turn_the_crank got CURLM_CALL_MULTI_PERFORM, retrying retry = %d\n", max_retry);
		}
		ABORT_IF_CURLM_FAIL("curl_multi_perform", err);
	} while (err == CURLM_CALL_MULTI_PERFORM && max_retry -- > 0);

	trace_debug("turn_the_crank running_handles = %d <END>\n", running_handles);

	cleanupIfNeeded(running_handles);
	return running_handles;
}

CURL *
SelectCurler::createConnection(const char *url) {
	CURL *curl = curl_easy_init();
	ABORT_IF_NULL("curl_easy_init", curl);
	if (curl) {

		url_transfer_private * priv =  (url_transfer_private *) calloc(1, sizeof(url_transfer_private));
		ABORT_IF_NULL("calloc (url_transfer_private)", priv);
		priv->url = (char *) url;
		priv->file_name = (char *) stUtil->allocNameFrom(priv->url);

		trace_debug("Opening file %s\n", priv->file_name);

		priv->f = fopen(priv->file_name, "a");
		if (priv->f == NULL) {
			fprintf(stderr, "Failed to open file %s\n", priv->file_name);
			perror(NULL);
			exit(1);
		}

		CURLcode err = curl_easy_setopt(curl, CURLOPT_URL, url);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_URL)", err);

		err = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION , write_handler);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_WRITEFUNCTION)", err);

		err = curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_WRITEDATA)", err);

		err = curl_easy_setopt(curl, CURLOPT_PRIVATE, priv);
		ABORT_IF_CURL_FAIL("curl_easy_setopt (CURLOPT_PRIVATE)", err);


		trace_debug("Setting easy options for  %s <END>\n", url);
	}
	return curl;
}

void
SelectCurler::addConnections(char *connections[]) {


	int cur = 0;
	char * url = NULL;
	do {
		url = connections[cur];
		if (url != NULL) {
			CURL * easy = createConnection(url);
			CURLMcode errm = curl_multi_add_handle(curlm, easy);
			ABORT_IF_CURLM_FAIL("curl_multi_add_handle", errm);
			activeHandles++;
			cur++;
		}
	} while (url != NULL);
	turnTheCrank();
}

void
SelectCurler::cleanupIfNeeded(int running_handles) {

	trace_debug("cleanup running_handles = %d\n", running_handles);

	while (running_handles < activeHandles) {
		int msgs_in_queue;
		CURLMsg * curl_msg = curl_multi_info_read(curlm, &msgs_in_queue);
		if (curl_msg == NULL) {
			fprintf(stderr, "Curl curl_multi_info_read failed with\n");
			exit(1);
		}

		if (curl_msg->msg == CURLMSG_DONE) {
			CURL * easy = curl_msg->easy_handle;

			long http_err_code;
			CURLcode err = curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_err_code);
			ABORT_IF_CURL_FAIL("curl_easy_getinfo (CURLINFO_RESPONSE_CODE)", err);

			char* priv_in  = NULL;
			err = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &priv_in);
			ABORT_IF_CURL_FAIL("curl_easy_getinfo (CURLINFO_PRIVATE)", err);
			url_transfer_private * priv = (url_transfer_private *) priv_in;

			trace_debug("Found completed connection for url %s http_err = %ld\n", priv->url, http_err_code);

			fflush(priv->f);
			fclose(priv->f);
			free(priv->file_name);
			free(priv);

			CURLMcode errm = curl_multi_remove_handle(curlm, easy);
			ABORT_IF_CURLM_FAIL("curl_multi_remove_handle", errm);
			curl_easy_cleanup(easy);
			activeHandles--;
		}

	};

	if (activeHandles == 0) {
		CURLMcode errm = curl_multi_cleanup(curlm);
		ABORT_IF_CURLM_FAIL("curl_multi_cleanup", errm);
		doneWithAllConnections = 1;
	}
}

int
SelectCurler::doSelect(fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set) {

	CURLMcode errm;

	int max_fd;
	int max_retry = 5;
	do {
		errm = curl_multi_fdset(curlm, read_fd_set, write_fd_set, exc_fd_set, &max_fd);
		ABORT_IF_CURLM_FAIL("curl_multi_fdset", errm);
		if (max_fd == -1) {
			trace_debug("curl_multi_fdset max_fd = %d retry = %d\n", max_fd, max_retry);
			usleep(10000); // 100 ms
			turnTheCrank();
		}
	} while (max_fd == -1 && max_retry-- > 0);

	trace_debug("max_fd = %d read_fd_set = %d, write_fd_set = %d, exc_fd_set =%d\n", max_fd, *(int32_t *) read_fd_set, *(int32_t * ) write_fd_set, *(int32_t *) exc_fd_set);

	long timeout_ms;
	errm = curl_multi_timeout(curlm, &timeout_ms);
	ABORT_IF_CURLM_FAIL("curl_multi_timeout", errm);

	struct timeval timeout = {};
	if (timeout_ms > 0) {
		if (timeout_ms >= 1000) {
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
		} else {
			timeout.tv_sec = 0;
			timeout.tv_usec = timeout_ms * 1000;
		}
	}
	trace_debug("Calling with timeout_ms = %ld, ts = %ld, ts_usec = %d\n", timeout_ms, timeout.tv_sec, timeout.tv_usec);

	int err = select(max_fd + 1, read_fd_set, write_fd_set, exc_fd_set, &timeout);
	ABORT_IF_SYSCALL_FAIL("select", err);

	turnTheCrank();

	trace_debug("do_select  done_with_all_connections = %d <END>\n", doneWithAllConnections);

	return doneWithAllConnections;
}
