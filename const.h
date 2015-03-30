#ifndef __CONST_H__
#define __CONST_H__

/*******************************
GLOBAL definition
*******************************/
#define BCD_VERSION             "v4.2"

#define DEVAULT_OUTPUT_DEVICE   "/dev/console"
#define DEFAULT_FACTORY_CONFIG  "/org/factory_conf/factory.conf"

#define SPACE_CHARACTER		('\"')
#define	ALIAS_CHARACTER		"ALIAS_"

//for logging system
#define	LOG_START				"LogStart"
#define	LOG_HEADER				"log_"
#define	LOG_START_NUM			(1)
#define	LOG_MAX_NUM				(40)
#define	LOG_MAX_STR				(120)


#define MAX_PRINT_LEN       	(1024)

#define	CONFIG_PATH				"/tmp/"
#define	PRIMARY_CONFIG_FILE		CONFIG_PATH "config_primary"
#define	SECONDARY_CONFIG_FILE	CONFIG_PATH "config_secondary"


//This is magic number.
#define MSG_KEY     (54321)		// RANDOM SEED


//message queue config
//This is for bcd daemon.
#define MSG_COMMIT_INTERVAL         (5)    //periodic commit
#define MSG_COMMIT_MIN_INTERVAL     (5)     //force commit limit (ignore commit)

//for message queue
#define MSG_MAX_NAME_LEN            (128)
#define MSG_MAX_DATA_LEN            (1024)

//for syslog output message
#define SYSLOG_LINE_LENGTH			(256)


//**********************************************************************
//message queue' message type
//1~0x8000 : send message type
#define MTYPE_SELF                  (1)         //self msg. Not used.
#define MTYPE_APP                   (11)        //command line apps.
#define MTYPE_LOG                   (12)        //logging system

//All return mtype is  0x8000 + pid(caller), pid = msg.pid
//0x8000~ : receive message type (with process' pid)
//0x8000 is PID_MAX_DEFAULT for pid_t
#define MTYPE_RET                   (0x8000)
//**********************************************************************


    
//message queue' command type
#define CMD_READ                    (1)
#define CMD_WRITE                   (2)         //add & update
#define CMD_DELETE                  (3)
#define CMD_COMMIT                  (4)
#define CMD_HOLD_COMMIT             (5)         //hold commit for management
#define CMD_RESUME_COMMIT           (6)         //resume commit
#define CMD_RELOAD_FILE             (11)
#define CMD_SAVE_FILE               (12)
#define CMD_SAVE_NVRAM_FILE         (13)
#define CMD_LOG_DELETE				(21)
#define CMD_LOG_SHOW				(22)

#define CMD_RET_OK                  (100)
#define CMD_RET_FAIL                (101)
    

#define NULL_STR                    ""



#endif
