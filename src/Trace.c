/*
 * Trace.c
 *
 *  Created on: Mar 28, 2012
 *      Author: stephane
 */

#include "Trace.h"
#include <stdarg.h>
#include <stdio.h>



void trace_debug(const char * format, ...) {
#ifdef TRACE_DEBUG
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
#endif
}


void trace_info(const char * format, ...) {
#ifdef TRACE_INFO
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
#endif
}

