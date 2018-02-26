#include <stdio.h>
#include <stdarg.h>

#include "src/shared.h"

#ifndef __LOG_H__
#define __LOG_H__

#ifdef DEBUG
	#define debug_log_e(a, ...) _log_e(__FILE__, __LINE__, a, ##__VA_ARGS__)
	#define debug_log_i(a, ...) _log_i(__FILE__, __LINE__, a, ##__VA_ARGS__)
#else
	#define debug_log_e(a, ...)
	#define debug_log_i(a, ...)
#endif

#define log_e(a, ...) _log_e(__FILE__, __LINE__, a, ##__VA_ARGS__)
#define log_i(a, ...) _log_i(__FILE__, __LINE__, a, ##__VA_ARGS__)

void _log_e(const char *, const int, const char *, ...);
void _log_i(const char *, const int, const char *, ...);

#endif
