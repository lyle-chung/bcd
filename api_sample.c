/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 *
 * 
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>

#include "const.h"
#include "message.h"


int main(int argc, char *argv[]) {
	int ret;
	unsigned char	*data;

	ret = nvram_commit(0);
	printf("\n commit = [%d]", ret); fflush(stdout);

	if(argc==2) {
		data = nvram_bufget(0, argv[1]);
		printf("\n read  = [%s]", data); fflush(stdout);
	}

	if(argc==3) {
		nvram_bufset(0, argv[1], argv[2]);
		printf("\n write = [%s]", argv[2]); fflush(stdout);
	}


	return 0;
}


