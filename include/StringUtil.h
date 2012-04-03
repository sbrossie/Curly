/*
 * StringUtil.h
 *
 *  Created on: Mar 12, 2012
 *      Author: stephane
 */

#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

class StringUtil {
public:
	StringUtil();
	virtual ~StringUtil();

	char* allocReplaceDotWithUnderscore(char* input);
	char* extractSubdomain(char* url);
	char* allocNameFrom(char* url);
};

#endif /* STRINGUTIL_H_ */
