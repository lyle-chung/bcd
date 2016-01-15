/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>

#include "const.h"
#include "linked.h"

int syslog_sw = 0;

void PRINT_SYSLOG(int priority, const char *format, ...)
{
	static char buf[MAX_PRINT_LEN];
	va_list args;
	int len;

	va_start(args, format);
	len = vsnprintf(buf, sizeof buf, format, args);
	va_end(args);
	if (len < 0 || len > MAX_PRINT_LEN)
		return;

	if (syslog_sw == 0) {
		syslog_sw = 1;
		openlog("bcd", LOG_PID, 0);
	}
	syslog(priority, "%s", &buf[0]);

	//closelog();

	return;
}

int default_node_handler(void *str)
{
	if (str)
		PRINT_SYSLOG(LOG_INFO, str);
	return 0;
}
