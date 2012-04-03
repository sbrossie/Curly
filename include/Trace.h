/*
 * Trace.h
 *
 *  Created on: Mar 28, 2012
 *      Author: stephane
 */

#ifndef TRACE_H_
#define TRACE_H_

#include <stdarg.h>

#ifndef TRACE_INFO
#define TRACE_INFO
#endif

extern "C" void trace_debug(const char * format, ...) ;

extern "C" void trace_info(const char * format, ...);


#endif /* TRACE_H_ */
