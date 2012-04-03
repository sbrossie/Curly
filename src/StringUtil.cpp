/*
 * StringUtil.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

#include <stdio.h>
#include <string.h>

#include "StringUtil.h"
#include "AbortIf.h"

StringUtil::StringUtil() {
}

StringUtil::~StringUtil() {
}

char *
StringUtil::allocReplaceDotWithUnderscore(char * input) {

	char *copy = strdup(input);
	ABORT_IF_NULL("strdup", copy);
	char *p = &copy[0];
	while (*p != '\0') {
		if (*p == '.') {
			*p = '_';
		}
		p++;
	}
	return copy;
}

char *
StringUtil::extractSubdomain(char * url) {

	char *p = &url[0];
	while (*p != '\0') {
		if (*p == 'w') {
			return p;
		}
		p++;
	}
	return NULL;
}

char * StringUtil::allocNameFrom(char * url) {
	char * tmp = extractSubdomain(url);
	if (tmp != NULL) {
		tmp = allocReplaceDotWithUnderscore(tmp);
	}
	return tmp;
}

