/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 *
 * 
 *-----------------------------------------------------------------------------
 */

/*
 * Board Config Daemon 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Descriptions:
 *
 * Rev     Date        Description
 * ==========================================================================
 * 0.1     2010/05/14  created by jaewonc 
 * 0.2     2010/06/07  patched : auto delete CR in entry name
 * 3.1     2012/02/22  updated for filesystme configuration
 * 4.1     2012/??/??  merged bcd for svn (last)
 * 4.2     2013/02/15  added parameter alias 
 *
 *
 * will be updated
 * ==========================================================================
 * manage bootloader config
 * add meta tag ex) R(read only), H(handler), I(interrupt), E(event), P(polling)..
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/time.h>

#include "const.h"
#include "message.h"
#include "linked.h"
#include "flash.h"

/*******************************
GLOBAL variable
*******************************/

//view output message, if daemon mode, use fd.
int       PrintMsgView = 0;
//send & receive message queue' ID
int       MessageID;
//flash config buffer
flash_s   Flash;
//output device name : default is console.
char      OutputDevice[128];
//check value for commit
int       Commit = 0;
//check value for force commit / reset interval commit
int       ForceCommit = 0;
//set next commit flash block
//while commiting, next commit command MUST be ignored.
int       SingleCommit = 0;
//Thread Mutex
pthread_mutex_t	Mutex;
//select storage : flash or file
int     FlashConfig = 1;        
//syslog enable flag
int		SyslogEnable = 0;


/*******************************
extern
*******************************/
extern int set_lowdata_with_node(flash_s * flash);
extern int get_node_from_lowdata(flash_s * flash);
extern int update_node(unsigned char *name, unsigned char *value);
extern int delete_node(unsigned char *name);
extern int get_lowdata_from_flash(flash_s * flash, int active);
extern int set_flash_with_lowdata(flash_s * flash, int active);
extern int reload_nodelist_from_file(char *fname);
extern int save_nodelist_into_file(char *fname);
extern int save_log_into_file(char *fname);
extern int delete_log_all();
extern void get_bootcfg_from_flash(void);
extern void PRINT_SYSLOG(int priority, const char *format, ...);
extern void register_default_handler(void);
extern int get_log_start();


/*******************************
Fn.
*******************************/
//display message into file descriptor
//void PRINT_MSG(int fd, const char *format, ...) {
//available socket, 
void PRINT_MSG(const char *format, ...)
{
	static int counter = 1;
	va_list   args;
	int       len, headlen, fd, ret;

	//display mode
	if (PrintMsgView) {
		static unsigned char buf[MAX_PRINT_LEN];
		static unsigned char head[10];
		static struct stat status;
		static mode_t filemode;
		static off_t filesize;

		va_start(args, format);
		len = vsnprintf(buf, sizeof buf, format, args);
		va_end(args);
		if (len < 0 || len > MAX_PRINT_LEN)
			return;

		//header
		headlen = sprintf(head, "\n<%04x>", counter++);

		ret = stat(OutputDevice, &status);
		if (ret == -1) {
			//not exist -> file
			fd = open(OutputDevice, O_CREAT | O_RDWR);
		}
		else {
			filemode = status.st_mode;
			if (S_ISCHR(filemode) || S_ISBLK(filemode)) {
				//if char/block device
				fd = open(OutputDevice, O_RDWR | O_NOCTTY);
			}
			else {
				//if regular file
				//check file size
				filesize = status.st_size;
				fd = open(OutputDevice, O_RDWR);
			}
		}

		if (fd > 0) {
			//goto file' bottom
			lseek(fd, 0, SEEK_END);
			//write buffer
			write(fd, head, headlen);
			write(fd, (unsigned char *) buf, len);
			close(fd);
		}
	}

	return;
}

int STRCPY(unsigned char *dest, unsigned char *src)
{
	if (dest && src) {
		strcpy(dest, src);
		return 0;
	}
	else {
		if (dest)
			PRINT_MSG("dest is NULL.");
		if (src)
			PRINT_MSG("src is NULL.");
		return -1;
	}
}

//Daemonize myself. 
int daemonize(int nochdir, int noclose)
{
	pid_t     pid;

	pid = fork();

	/* In case of fork is error. */
	if (pid < 0) {
		perror("fork");
		return -1;
	}

	/* In case of this is parent process. */
	if (pid != 0)
		exit(0);

	/* Become session leader and get pid. */
	pid = setsid();
	if (pid < -1) {
		perror("setsid");
		return -1;
	}

	/* Change directory to root. */
	if (!nochdir)
		chdir("/");

	/* File descriptor close. */
	if (!noclose) {
		int       fd;

		fd = open("/dev/null", O_RDWR, 0);
		if (fd != -1) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			if (fd > 2)
				close(fd);
		}
	}
	umask(0027);

	return 0;
}


void *commit_thread_fn(void *data) {

	pthread_mutex_lock(&Mutex);

	//set NODe info from list
	if (FlashConfig) {
		set_lowdata_with_node(&Flash);

		//save nodes into flash
		//always write into primary at first and next.
		PRINT_MSG("writing config into primary.");
		set_flash_with_lowdata(&Flash, 0);
		PRINT_MSG("writing config into secondary.");
		set_flash_with_lowdata(&Flash, 1);
	} else {
		PRINT_SYSLOG(LOG_INFO, "writing config into primary file.");
		save_nodelist_into_file(PRIMARY_CONFIG_FILE);
		PRINT_SYSLOG(LOG_INFO, "writing config into secondary file.");
		save_nodelist_into_file(SECONDARY_CONFIG_FILE);
	}

	//display
	//display_nodelist_all();

	Commit = 0;
	SingleCommit = 0;

	//syslog:
	PRINT_SYSLOG(LOG_INFO, "The configuration was saved.");

	pthread_mutex_unlock(&Mutex);
	pthread_exit(0);
	//return 0;
}

int commit_process()
{
	pthread_t	thread;
	int			pid;

	if(SingleCommit==0) {
		SingleCommit = 1;

		pid = pthread_create(&thread, NULL, commit_thread_fn, NULL);
		if(pid<0) {
			PRINT_MSG("can't execute flash thread.");
		}		
		pthread_detach(thread);
		//pthread_join(thread, NULL);
	}		

	return 0;
}

int	is_alias(unsigned char	*value) {
	//check len
	if(strlen(value) <= strlen(ALIAS_CHARACTER)) {
		return 0;
	}
	//check alias
	if(strncmp(value, ALIAS_CHARACTER, strlen(ALIAS_CHARACTER)) == 0) {
		//this is alias
		return 1;
	} 
	return 0;
}

int process_message(int mid, MSG * msg)
{
	NODE     *pNode;
	int      ret;
	unsigned char	alias[MSG_MAX_NAME_LEN];
	

	//search node
	pNode = node_lookup_by_name(msg->name);

	//generate receiver msg type
	msg->mtype = get_mtype_for_reply(msg->pid);;

	switch (msg->cmd) {
	case CMD_READ:
		//re-gen reply message
		if (pNode) {
			//check alias
			if(is_alias(pNode->value)) {
				STRCPY(alias, pNode->value);
				pNode = node_lookup_by_name(alias+strlen(ALIAS_CHARACTER));	
			}
			if(pNode) {
			//found
			STRCPY(msg->data, pNode->value);
			}
			msg->cmd = CMD_RET_OK;
		}
		else {
			//not found
			msg->cmd = CMD_RET_OK;
			//PRINT_MSG("unknown msg name[%s]", msg->name);
		}
		break;

	case CMD_WRITE:
		if(pNode && is_alias(pNode->value)) {
			STRCPY(alias, pNode->value);
			pNode = node_lookup_by_name(alias+strlen(ALIAS_CHARACTER));	
			if(pNode) {
				//create or update
				update_node(pNode->name, msg->data);
			} else {
				//no process;; maybe misspelling node name
			}
		} else {
		//create or update
		update_node(msg->name, msg->data);
		}

		msg->cmd = CMD_RET_OK;

		//commit at next interval.
		//if name is READ_*, DO NOT COMMIT.
		if(strncmp(msg->name, "READ_", 5) == 0) {
		} else {
			//normal commit.
		Commit = 1;
		}

		//logging
		PRINT_SYSLOG(LOG_INFO, "<%s>'value was changed to <%s>", msg->name, msg->data);
		break;

	case CMD_DELETE:
		//delete node from list
		delete_node(msg->name);
		msg->cmd = CMD_RET_OK;
		//commit at next interval.
		Commit = 1;

		//logging
		PRINT_SYSLOG(LOG_INFO, "<%s>'value was deleted.", msg->name);
		break;

	case CMD_COMMIT:
		msg->cmd = CMD_RET_OK;
		//enable force commit & clear interval commit' timer
		ForceCommit = 1;
		commit_process();

		//logging
		PRINT_SYSLOG(LOG_INFO, "Manually committed.");
		break;

	case CMD_LOG_SHOW:
		//can see and save logs into file or device
		save_log_into_file(msg->data);
		msg->cmd = CMD_RET_OK;
		break;

	case CMD_LOG_DELETE:
		delete_log_all();
		msg->cmd = CMD_RET_OK;
		//commit at next interval.
		Commit = 1;
		break;

	case CMD_HOLD_COMMIT:
		break;

	case CMD_RESUME_COMMIT:
		break;

	case CMD_RELOAD_FILE:
		//first, clear and reload nodes from file
		ret = reload_nodelist_from_file(msg->data);

		if(ret<0) {
			msg->cmd = CMD_RET_FAIL;
			PRINT_MSG("unknown file name[%s]", msg->name);
		} else {
			//overwrite or update nodes using bootcfg : DEFAULT
			get_bootcfg_from_flash();

			//register node handler
			register_default_handler();

			msg->cmd = CMD_RET_OK;
			//enable force commit & clear interval commit' timer
			ForceCommit = 1;
			commit_process();
		}

		//logging
		PRINT_SYSLOG(LOG_INFO, "Reloaded configuration using <%s>", msg->data);
		break;

	case CMD_SAVE_FILE:
		//can see and save nodes into file or device
		save_nodelist_into_file(msg->data);
		msg->cmd = CMD_RET_OK;

		//logging
		PRINT_SYSLOG(LOG_INFO, "The configuration was saved into <%s>", msg->data);
		break;

	case CMD_RET_OK:
		break;

	case CMD_RET_FAIL:
		break;

	default:
		return 0;
	}

	//debug
	//display_nodelist_all();
	return 0;
}

void usage(char *progname)
{
	printf("\nUsage: %s [options..]", progname);
	printf("\n\t-d\tdaemonize process");
	printf("\n\t-f\tuse file configuration, NOT flash");
	printf("\n\t-v\tif defined, display messages into output device");
	printf("\n\t-o\tif v is defined, redirect output messages, default is console");
	printf("\n\t-h\tdisplay tihs usage");
	printf("\n\n");
	exit(0);
}

void nothing_signal_handler(int sig) {
	//nothing
	PRINT_SYSLOG(LOG_CRIT, "received unknown signal [%d]", sig);
}

void unknown_signal_handler(int sig) {
	//nothing
	PRINT_SYSLOG(LOG_CRIT, "received unknown signal [%d]", sig);
}

int turn_off_signal() {
	//register all signal
	signal(SIGUSR1, nothing_signal_handler);
	signal(SIGUSR2, nothing_signal_handler);
	signal(SIGINT,  nothing_signal_handler);
	signal(SIGTERM, nothing_signal_handler);
	signal(SIGQUIT, nothing_signal_handler);
	signal(SIGTRAP, nothing_signal_handler);
	signal(SIGPIPE, nothing_signal_handler);

	return 0;
}

int process_check() {
	FILE *fp;
	char *output;
	char *ptr;
	pid_t	pid;

	fp = popen("pidof bcd", "r");
	output = malloc(1024);
	fread(output, 1, 1024, fp);
	pclose(fp);

	//get current pid
	pid = getpid();
	ptr = strchr(output, ' ');
	if(ptr==NULL)
		return 0;
	else
		return -1;


}

int main(int argc, char *argv[])
{
	int       ret;
	MSG       msg;
	char     *progname;
	int       daemon_mode = 0;
	int       display_mode = 0;
	int       output_mode = 0;
	char     *p;

	//for logging
	int		log_start;
	char	date[MSG_MAX_DATA_LEN+16];

	//for time
	time_t	current_time, last_time;
	struct	tm *t;

	progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);
	while (1) {
		int       opt;

		opt = getopt(argc, argv, "dfvo:lh?");
		if (opt == EOF)
			break;

		switch (opt) {
		case 0:
			break;

		case 'd':
			daemon_mode = 1;
			break;

		case 'l':
			SyslogEnable = 1;
			break;

		case 'f':
			//use File config, not Flash config
			FlashConfig = 0;
			break;

		case 'v':
			display_mode = 1;
			PrintMsgView = 1;
			break;

		case 'o':
			output_mode = 1;
			strcpy(OutputDevice, optarg);
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


	//overwrite output device or file
	if (!output_mode) {
		strcpy(OutputDevice, DEVAULT_OUTPUT_DEVICE);
	}

	//if bcd is already running, exit this process.
	ret = process_check();
	if(ret<0) {
		PRINT_MSG("duplicated bcd.. exit");
		return -1;
	} else {
		PRINT_MSG("starting bcd (%s)", BCD_VERSION);
	}

	//OK, first turn ALL signal off
	turn_off_signal();

	//NODE list init
	nodelist_init();

	//thread init
	pthread_mutex_init(&Mutex, NULL);

	//for receive message at block mode
	//moved to message.c
	//signal(SIGALRM, (void*)message_signal);

	if (FlashConfig) {
	//get NODE info from primary flash
	ret = get_lowdata_from_flash(&Flash, 0);
	if (ret < 0) {
		//get NODE info from secondary flash
		ret = get_lowdata_from_flash(&Flash, 1);
		if (ret < 0) {
			//flash is empty, reload factory default
			PRINT_MSG("loading config from factory default.");
			//first, clear and reload nodes from file
			reload_nodelist_from_file(DEFAULT_FACTORY_CONFIG);
			//enable force commit & clear interval commit' timer
			ForceCommit = 1;
		} else {
			PRINT_MSG("loading config from secondary.");
		}
	} else {
		PRINT_MSG("loading config from primary.");
	}

	//assign flash info into list
	get_node_from_lowdata(&Flash);

	//overwrite or update nodes using bootcfg
	get_bootcfg_from_flash();
	} else {
		//get NODE info from primary file config
		ret = reload_nodelist_from_file(PRIMARY_CONFIG_FILE);
		if (ret < 0) {
			//get NODE info from secondary file config
			ret = reload_nodelist_from_file(SECONDARY_CONFIG_FILE);
			if (ret < 0) {
				//get NODE info from facroty file config
				ret = reload_nodelist_from_file(DEFAULT_FACTORY_CONFIG);
				//flash is empty, reload factory default
				PRINT_MSG("loading config from factory default.");
				ForceCommit = 1;
			} else {
				PRINT_MSG("loading config from secondary (file).");
			}
		} else {
			PRINT_MSG("loading config from primary (file).");
		}
	}


	//register node handler
	register_default_handler();



	//for send/receive message : server mode
	MessageID = get_message_id(IPC_CREAT | IPC_EXCL | 0666);

	//set last Commit time
	last_time = time(NULL);

	//background mode
	if (daemon_mode)
		daemonize(0, 0);

	//Now, processing message 
	while (1) {
		ret = receive_message_with_alarm(MessageID, &msg, -MTYPE_RET, 1);
		if (ret > 0) {
			//PRINT_MSG("received mtype[%d], pid[%d], cmd[%d]", msg.mtype, msg.pid, msg.cmd);
			//PRINT_MSG("received name[%s]", msg.name);
			//PRINT_MSG("received data[%s]", msg.data);

			switch (msg.mtype) {
			case (MTYPE_APP):
				//general read/write command
				process_message(MessageID, &msg);
				send_message(MessageID, &msg);
				break;

			case (MTYPE_LOG):
				//log system message type

				//pre-processing log message : change message' name
				log_start = get_log_start();	
				sprintf(msg.name, "%s%d", LOG_HEADER, log_start);

				//increment log_start
				log_start++;
				if(log_start > LOG_MAX_NUM)
					log_start = LOG_START_NUM;
				set_log_start(log_start);
				
				//get current time
				current_time = time(NULL);
				t = localtime(&current_time);
				
				//update logging date
				//if date is not updated, display 0000.00.00
				if( (t->tm_year + 1900) >=2010 ) {
					sprintf(date, "%04d.%02d.%02d-%02d:%02d:%02d %s", 
						t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
						t->tm_hour, t->tm_min, t->tm_sec,
						msg.data);
					strcpy(msg.data, date);
				} else {
					sprintf(date, "0000.00.00-%02d:%02d:%02d %s", 
						t->tm_hour, t->tm_min, t->tm_sec,
						msg.data);
					strcpy(msg.data, date);
				}


				//process message like as general command
				process_message(MessageID, &msg);
				send_message(MessageID, &msg);
				break;

			default:
				PRINT_MSG("unknown message type.");
			}
		}

		//if force commit, clear interval commit.
		if (ForceCommit) {
			commit_process();
			ForceCommit = 0;
			Commit = 0;
			PRINT_MSG("Force commit");
		} else {
			//Interval Commit (Normal auto commit)
			//get current time
			current_time = time(NULL);
			if(current_time > (last_time+MSG_COMMIT_INTERVAL)) {
			//check whether to commit or skip
			//if there is NO commit, default LAST config is SECONDARY and next_block is PRIMARY.
				if (Commit) 
				commit_process();
				//update last time
				last_time = current_time;
			}
		}
	}

	return 0;
}
