/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
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

void PRINT_MSG(const char *format, ...)
{
	static int counter = 1;
	va_list args;
	int len, headlen;

	char buf[MAX_PRINT_LEN];
	char output[MAX_PRINT_LEN];
	char head[10];

	va_start(args, format);
	len = vsnprintf(buf, sizeof buf, format, args);
	va_end(args);
	if (len < 0 || len > MAX_PRINT_LEN)
		return;

	headlen = sprintf(head, "\n<%04x>", counter++);
#if 0
	//fd = open(CONSOLE_DEVICE, O_RDWR|O_NOCTTY);
	if (fd > 0) {
		write(stdout, (void *)&head[0], (size_t) headlen);
		write((int)stdout, (void *)buf, (size_t) len);
		//close(fd);
	}
#else
	sprintf(output, "%s", &head[0]);
	sprintf(&output[headlen], "%s", buf);
	printf("%s", output);
#endif
	return;
}

void usage(char *progname)
{
	printf("\nUsage: %s [options..]", progname);
	printf("\n\t-n node\t: Node name");
	printf("\n\t-d data\t: if defined, write data into Node name");
	printf("\n\t-c     \t: forcely commit into flash");
	printf("\n\t-s file\t: save nodes into file");
	printf("\n\t-r file\t: reload nodes from file, old config will be replaced");
	printf("\n\t-u file\t: update nodes from file, updated into old config");
	printf("\n\t-D     \t: delete node using name");
	printf
	    ("\n\t-S     \t: script mode, JUST show value for read, none for write");
	printf("\n\t-l file\t: read log message");
	printf("\n\t-h     \t: display tihs usage");
	printf("\n\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int mid;
	int ret;
	MSG msg, msgr;
	pid_t pid;
	int cnt;
	char *progname, *p;
	int name_mode = 0;
	int script_mode = 0;
	int data_mode = 0;
	int commit_mode = 0;
	int save_mode = 0;
	int update_mode = 0;
	int delete_mode = 0;
	int log_mode = 0;
	char name[128], data[1024];

	name[0] = 0x0;
	data[0] = 0x0;

	progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

	while (1) {
		int opt;

		opt = getopt(argc, argv, "SDcl:n:d:s:u:h?");
		if (opt == EOF)
			break;

		switch (opt) {
		case 0:
			break;

		case 'S':
			script_mode = 1;
			break;

		case 'D':
			delete_mode = 1;
			break;

		case 'l':
			log_mode = 1;
			strcpy(name, optarg);
			break;

		case 'n':
			name_mode = 1;
			strcpy(name, optarg);
			break;

		case 'd':
			data_mode = 1;
			strcpy(data, optarg);
			break;

		case 'c':
			commit_mode = 1;
			break;

		case 's':
			save_mode = 1;
			strcpy(name, optarg);
			break;

		case 'u':
			update_mode = 1;
			strcpy(name, optarg);
			break;

		case 'h':
		case '?':
			usage(progname);
			break;

		default:
			usage(progname);
			break;
		}
	}

	//parameters check
	if (!commit_mode && !save_mode && !update_mode) {
		if (!name_mode && !log_mode) {
			usage(progname);
			return 0;
		}
	}

	pid = getpid();

	// 1: get message ID
	mid = get_message_id(0);
	if (mid == -1) {
		printf("\nerror.");
		exit(-1);
	}
	// 2: set message type and command
	memset(&msg, 0, sizeof(MSG));
	msg.mtype = MTYPE_APP;
	msg.pid = pid;
	msg.cmd = CMD_READ;
	sprintf(msg.name, "%s", name);

	if (data_mode) {
		sprintf(msg.data, "%s", data);
		msg.cmd = CMD_WRITE;
	}

	if (commit_mode)
		msg.cmd = CMD_COMMIT;

	if (save_mode) {
		msg.cmd = CMD_SAVE_FILE;
		sprintf(msg.data, "%s", name);
	}
	if (update_mode) {
		msg.cmd = CMD_UPDATE_FROM_FILE;
		sprintf(msg.data, "%s", name);
	}
	if (delete_mode) {
		msg.cmd = CMD_DELETE;
		sprintf(msg.data, "%s", name);
	}
	if (log_mode) {
		sprintf(msg.data, "%s", name);
		if (delete_mode) {
			msg.cmd = CMD_LOG_DELETE;
		} else {
			msg.cmd = CMD_LOG_SHOW;
		}
	}
	// 3: send message to server
	send_message(mid, &msg);
	// 4: receive message from server
	cnt = 0;
	do {
		ret =
		    receive_message_with_alarm(mid, &msgr,
					       get_mtype_for_reply(msg.pid), 5);
		if (ret < 0) {
			printf("\ncan't receive message");
			printf("\ncheck server process.");
			return -1;
		} else {
			if (msgr.cmd == CMD_RET_FAIL) {
				break;
			}
			if (msgr.cmd == CMD_RET_OK) {
				//receive.
				if (data_mode && name_mode) {
					if (!script_mode)
						printf
						    ("client:write: name<%s> data<%s>\n",
						     msgr.name, msgr.data);
				} else if (name_mode) {
					if (!script_mode)
						printf
						    ("client:read: name<%s> data<%s>\n",
						     msgr.name, msgr.data);
					else
						printf("%s", msgr.data);
				}
			}
			break;
		}
	} while (cnt++ < 3);

	if (msgr.cmd == CMD_RET_OK)
		return 0;
	else
		return 1;
}
