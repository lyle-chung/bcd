#ifndef __MESAAGE_H__
#define __MESAAGE_H__

#include "const.h"

/*******************************
STRUCTURE
*******************************/
typedef struct {
	//distinguish process : send/receive
    long    mtype;

	//command
    unsigned char	cmd;
	pid_t			pid;
	unsigned char	name[MSG_MAX_NAME_LEN];
	unsigned char	data[MSG_MAX_DATA_LEN];
} __attribute__ ((packed)) MSG;






/*******************************
PROTOTYPE
*******************************/
extern int	get_message_id();
extern int get_mtype_for_reply(pid_t);
extern int send_message(int mid, MSG *msg);
extern int receive_message_with_alarm(int mid, MSG*, int, int);
extern void reset_alarm_fn(int);


extern int config_commit();
extern unsigned char* config_get(unsigned char*);
extern int config_set(unsigned char*, unsigned char*);

extern int nvram_commit(int);
extern unsigned char* nvram_bufget(int, unsigned char*);
extern int nvram_bufset(int, unsigned char*, unsigned char*);

extern unsigned char* nvram_get(int, unsigned char*);
extern int nvram_set(int, unsigned char*, unsigned char*);

extern int bcd_logging(const char *format, ...);
#endif
