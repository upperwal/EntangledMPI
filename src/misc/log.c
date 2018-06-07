#include "src/misc/log.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

extern Node node;

va_list args;

void _log_e(const char *module, const int line, const char *message, ...) {
	char buffer[200];
	sprintf(buffer, "%sERROR:%s [Rank: %d] | Message: %s | Module: %s | Line: %d\n", KRED, KNRM, node.static_rank, message, module, line);
	
	va_start(args, buffer);
	vfprintf(stdout, buffer, args);
	va_end(args);
}

void _log_i(const char *module, const int line, const char *message, ...) {
	char buffer[200];
	sprintf(buffer, "%sINFO:%s [Rank: %d] | Message: %s | Module: %s | Line: %d\n", KGRN, KNRM, node.static_rank, message, module, line);

	va_start(args, buffer);
	vfprintf(stdout, buffer, args);
	va_end(args);
	
}
