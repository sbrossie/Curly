/*
 * AbortIf.h
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

#ifndef ABORTIF_H_
#define ABORTIF_H_

#include "stdlib.h"
#include <stdio.h>

#define ABORT_IF_NULL(call, val)  do { \
	if ((val) == NULL) { \
		fprintf(stderr, "Curl call %s failed with error NULL \n", (call)); \
		exit(1); \
	} \
} while (0)

#define ABORT_IF_CURL_FAIL(call, err) do { \
	if ((err) != CURLE_OK) { \
		fprintf(stderr, "Curl call %s failed with error %d\n", (call), (err)); \
		exit(1); \
	} \
} while (0)

#define ABORT_IF_CURLM_FAIL(call, err) do { \
	if ((err) != CURLM_OK && (err) != CURLM_CALL_MULTI_PERFORM) { \
		fprintf(stderr, "Curl call %s failed with error %d\n", (call), (err)); \
		exit(1); \
	} \
} while (0)

#define ABORT_IF_SYSCALL_FAIL(call, err) do { \
	if ((err) == -1) { \
		fprintf(stderr, "System call %s failed with error %d\n", (call), (err)); \
		perror(NULL); \
	exit(1); \
	} \
} while (0)


#endif /* ABORTIF_H_ */
