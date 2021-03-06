/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 * Confidential
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

//const.h is First.
#include "const.h"
#include "message.h"
#include "linked.h"

/*******************************
GLOBAL
*******************************/
struct list *bufferlist = NULL;

/*******************************
EXTERN
*******************************/
extern int MessageID;

/*******************************
Buffer APIs : INTERNALLY USE.
*******************************/

static void buffer_init()
{
	bufferlist = list_new();
}

static void string_check_and_conv(char *name, char *str)
{
	int i;

	strcpy(str, name);
	for (i = 0; i < strlen(str); i++) {
		//special char " --> ' '
		if (*(str + i) == SPACE_CHARACTER)
			*(str + i) = ' ';
		//CR is same as NULL/end of word.
		if (*(str + i) == 0x0d)
			*(str + i) = 0x00;
	}
	return;
}

/* Create new node structure. */
static NODE *buffer_new()
{
	NODE *pNode;

	pNode = malloc(sizeof(NODE));
	memset(pNode, 0, sizeof(NODE));
	return pNode;
}

/* existance check */
static NODE *buffer_lookup_by_name(char *name)
{
	struct listnode *node;
	NODE *pNode;

	for (node = listhead(bufferlist); node; nextnode(node)) {
		pNode = getdata(node);
		if (strcmp(pNode->name, name) == 0)
			return pNode;
	}
	return NULL;
}

static char *buffer_update(char *name, char *data)
{
	NODE *pNode;
	char *name_str;
	char *data_str;

	name_str = malloc(128);
	data_str = malloc(1024);

	//init root node
	if (!bufferlist)
		buffer_init();

	//valid check and convert special character: #
	string_check_and_conv(name, name_str);
	string_check_and_conv(data, data_str);

	pNode = buffer_lookup_by_name(name);
	if (!pNode) {
		//create and link
		pNode = buffer_new();
		pNode->name = NULL;
		pNode->value = NULL;
		listnode_add(bufferlist, pNode);
	} else {
		//just update
		//first, free
		if (pNode->name)
			free(pNode->name);
		if (pNode->value)
			free(pNode->value);
	}

	//update
	if (name_str)
		pNode->name = strdup(name_str);
	if (data_str)
		pNode->value = strdup(data_str);

	free(name_str);
	free(data_str);

	//return data pointer for processing
	return (pNode->value);
}

/*******************************
Fn.
*******************************/

int get_message_id(int flags)
{
	int mid;
	int cnt;

	cnt = 0;
	do {
		mid = msgget(MSG_KEY, flags);
		if (mid < 0) {
			char buf[128];
			sprintf(buf, "ipcrm -Q %d", MSG_KEY);
			system(buf);
			cnt++;
		}
		if (cnt > 10) {
			return -1;
		}
	} while (mid < 0);

	return mid;
}

int get_mtype_for_reply(pid_t pid)
{
	return MTYPE_RET + pid;
}

void reset_alarm_fn(int arg)
{
	//

	//just clear alarm
	alarm(0);
}

//block and alarm mode 
int receive_message_with_alarm(int mid, MSG * msg, int mtype, int alrm)
{
	int ret;

	//for receive message at block mode
	signal(SIGALRM, (void *)reset_alarm_fn);

	//start alarm
	alarm(alrm);

	//block receive routine.
	ret = msgrcv(mid, msg, MSG_MAX_DATA_LEN, mtype, 0);

	//clear alarm
	reset_alarm_fn(0);

	return ret;
}

int send_message(int mid, MSG * msg)
{
	int ret, len;

	//Name length is fixed(128bytes.), BUT, msg.data is variable.
	//len = sizeof(char) + sizeof(pid_t) + Name length(128) + strlen(msg->data) + 1;  
	len =
	    sizeof(char) + sizeof(pid_t) + MSG_MAX_NAME_LEN +
	    strlen(msg->data) + 1;
	ret = msgsnd(mid, msg, len, 0);

	//printf("\n[%d][%d][%d][%d]", len, sizeof(char), sizeof(pid_t), strlen(msg->data));
	return ret;
}

/*******************************
APIs
*******************************/
char *config_get(const char *name)
{
	MSG msg, msg_r;
	int pid;
	int mid;
	int cnt;
	int ret;
	char *data;

	//1. get process ID
	pid = getpid();

	//2. get message ID
	mid = get_message_id(0);
	if (mid == -1) {
		return NULL_STR;
	}
	//3. set message
	memset(&msg, 0, sizeof(MSG));
	memset(&msg_r, 0, sizeof(MSG));
	msg.mtype = MTYPE_APP;
	msg.pid = pid;
	sprintf(msg.name, "%s", name);
	msg.cmd = CMD_READ;

	//4: send message to server
	send_message(mid, &msg);

	//5: receive result or ACK
	cnt = 0;
	do {
		ret =
		    receive_message_with_alarm(mid, &msg_r,
					       get_mtype_for_reply(msg.pid), 5);
		if (ret < 0) {
			return NULL_STR;
		} else {
			if (msg_r.cmd == CMD_RET_FAIL) {
				return NULL_STR;
			}
			if (msg_r.cmd == CMD_RET_OK) {
				//****
				//update buffer and return data pointer
				//****
				data = buffer_update(msg_r.name, msg_r.data);
			}
			break;
		}
	} while (cnt++ < 3);

	if (msg_r.cmd == CMD_RET_OK)
		return data;
	else
		return NULL_STR;
}

int config_get_int(const char *name) {
	static char	buf[MSG_MAX_DATA_LEN];

	strcpy(buf, config_get(name));
	return (int)strtol(buf, NULL, 10);
}

short config_get_int16(const char *name) {
	static char	buf[MSG_MAX_DATA_LEN];

	strcpy(buf, config_get(name));
	return (short)strtol(buf, NULL, 10);
}

char config_get_int8(const char *name) {
	static char	buf[MSG_MAX_DATA_LEN];

	strcpy(buf, config_get(name));
	return (char)strtol(buf, NULL, 10);
}

double config_get_float(const char *name) {
	static char	buf[MSG_MAX_DATA_LEN];

	strcpy(buf, config_get(name));
	return (float)strtod(buf, NULL);
}

double config_get_double(const char *name) {
	static char	buf[MSG_MAX_DATA_LEN];

	strcpy(buf, config_get(name));
	return (double)strtod(buf, NULL);
}

int config_set(const char *name, const char *data)
{
	MSG msg, msg_r;
	int pid;
	int mid;
	int cnt;
	int ret;

    //0.
    if (strlen(data) > MSG_MAX_REAL_DATA_LEN) {
        return -1;
    }

	//1. get process ID
	pid = getpid();

	//2. get message ID
	mid = get_message_id(0);
	if (mid == -1) {
		return -1;
	}
	//3. set message
	memset(&msg, 0, sizeof(MSG));
	memset(&msg_r, 0, sizeof(MSG));
	msg.mtype = MTYPE_APP;
	msg.pid = pid;
	sprintf(msg.name, "%s", name);
	sprintf(msg.data, "%s", data);
	msg.cmd = CMD_WRITE;

	//4: send message to server
	send_message(mid, &msg);

	//5: receive result or ACK
	cnt = 0;
	do {
		ret =
		    receive_message_with_alarm(mid, &msg_r,
					       get_mtype_for_reply(msg.pid), 5);
		if (ret < 0) {
			return -1;
		} else {
			if (msg_r.cmd == CMD_RET_FAIL) {
				return -1;
			}
			if (msg_r.cmd == CMD_RET_OK) {
				//receive.
				return 0;
			}
			break;
		}
	} while (cnt++ < 3);

	if (msg_r.cmd == CMD_RET_OK)
		return 0;
	else
		return -1;
}

int config_set_int(const char *name, const int value) {
	static char buf[MSG_MAX_DATA_LEN];

	sprintf(buf, "%d", value);
	config_set(name, buf);

	return 0;
}

int config_set_int16(const char *name, const short value) {
	static char buf[MSG_MAX_DATA_LEN];

	sprintf(buf, "%d", value);
	config_set(name, buf);

	return 0;
}

int config_set_int8(const char *name, const char value) {
	static char buf[MSG_MAX_DATA_LEN];

    memset(buf, 0, sizeof(char) * MSG_MAX_DATA_LEN);
	sprintf(buf, "%d", value);
	config_set(name, buf);

	return 0;
}

int config_set_double(const char *name, const double value) {
	static char buf[MSG_MAX_DATA_LEN];

	sprintf(buf, "%f", value);
	config_set(name, buf);

	return 0;
}

//for logging system
int bcd_logging(const char *format, ...)
{
	MSG msg, msg_r;
	int pid;
	int mid;
	int cnt;
	int ret;
	static char data[MAX_PRINT_LEN];
	va_list args;
	int len;

	va_start(args, format);
	len = vsnprintf(data, sizeof data, format, args);
	va_end(args);
	if (len < 0 || len > MAX_PRINT_LEN)
		return -1;

	//1. get process ID
	pid = getpid();

	//2. get message ID
	mid = get_message_id(0);
	if (mid == -1) {
		return -1;
	}
	//3. set message
	memset(&msg, 0, sizeof(MSG));
	memset(&msg_r, 0, sizeof(MSG));
	msg.mtype = MTYPE_LOG;
	msg.pid = pid;
	sprintf(msg.name, "%s", "log");	//don't care string.
	sprintf(msg.data, "%s", data);
	msg.cmd = CMD_WRITE;

	//4: send message to server
	send_message(mid, &msg);

	//5: receive result or ACK
	cnt = 0;
	do {
		ret =
		    receive_message_with_alarm(mid, &msg_r,
					       get_mtype_for_reply(msg.pid), 5);
		if (ret < 0) {
			return -1;
		} else {
			if (msg_r.cmd == CMD_RET_FAIL) {
				return -1;
			}
			if (msg_r.cmd == CMD_RET_OK) {
				//receive.
				return 0;
			}
			break;
		}
	} while (cnt++ < 3);

	if (msg_r.cmd == CMD_RET_OK)
		return 0;
	else
		return -1;
}

int config_commit()
{
	MSG msg, msg_r;
	int pid;
	int mid;
	int cnt;
	int ret;

	//1. get process ID
	pid = getpid();

	//2. get message ID
	mid = get_message_id(0);
	if (mid == -1) {
		return -1;
	}
	//3. set message
	memset(&msg, 0, sizeof(MSG));
	memset(&msg_r, 0, sizeof(MSG));
	msg.mtype = MTYPE_APP;
	msg.pid = pid;
	msg.cmd = CMD_COMMIT;

	//4: send message to server
	send_message(mid, &msg);

	//5: receive result or ACK
	cnt = 0;
	do {
		ret =
		    receive_message_with_alarm(mid, &msg_r,
					       get_mtype_for_reply(msg.pid), 5);
		if (ret < 0) {
			return -1;
		} else {
			if (msg_r.cmd == CMD_RET_FAIL) {
				return -1;
			}
			if (msg_r.cmd == CMD_RET_OK) {
				return 0;
			}
		}
	} while (cnt++ < 3);
	return -2;
}

