#ifndef __MESAAGE_H__
#define __MESAAGE_H__

#include "const.h"

#ifdef __cplusplus
extern "C" {
#endif



/*******************************
STRUCTURE
*******************************/
typedef struct {
	//distinguish process : send/receive
	long mtype;

	//command
	char cmd;
	pid_t pid;
	char name[MSG_MAX_NAME_LEN];
	char data[MSG_MAX_DATA_LEN];
} __attribute__ ((packed)) MSG;

/*******************************
PROTOTYPE
*******************************/
extern int get_message_id(int);
extern int get_mtype_for_reply(pid_t);
extern int send_message(int mid, MSG * msg);
extern int receive_message_with_alarm(int mid, MSG *, int, int);
extern void reset_alarm_fn(int);

extern int config_commit();
extern char *config_get(const char *);
extern int config_set(const char *, const char *);
extern int bcd_logging(const char *format, ...);

extern int config_get_int(const char *name);
extern short config_get_int16(const char *name);
extern char config_get_int8(const char *name);
double config_get_float(const char *name); 
double config_get_double(const char *name); 
extern int config_set_int(const char *name, const int value);
extern int config_set_int16(const char *name, const short value);
extern int config_set_int8(const char *name, const char value);
extern int config_set_float(const char *name, const double value);
extern int config_set_double(const char *name, const double value);


#ifdef __cplusplus
}
#endif


#endif
