#include "src/misc/log.h"

extern Node node;

va_list args;

void _log_e(const char *module, const int line, const char *message, ...) {
	char buffer[200];
	sprintf(buffer, "ERROR: [Rank: %d] | Message: %s | Module: %s | Line: %d\n", node.rank, message, module, line);
	
	va_start(args, buffer);
	vfprintf(stdout, buffer, args);
	va_end(args);
}

void _log_i(const char *module, const int line, const char *message, ...) {
	char buffer[200];
	sprintf(buffer, "INFO: [Rank: %d] | Message: %s | Module: %s | Line: %d\n", node.rank, message, module, line);

	va_start(args, buffer);
	vfprintf(stdout, buffer, args);
	va_end(args);
	
}
