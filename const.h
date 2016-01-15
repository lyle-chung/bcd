#ifndef __CONST_H__
#define __CONST_H__

#ifdef __cplusplus
extern "C" {
#endif


/*******************************
GLOBAL definition
*******************************/
#define BCD_VERSION             "v4.2"

#define DEVAULT_OUTPUT_DEVICE   "/dev/console"

#define SPACE_CHARACTER		('\"')
#define	ALIAS_CHARACTER		"ALIAS_"

//for logging system
#define	LOG_START				"LogStart"
#define	LOG_HEADER				"log_"
#define	LOG_START_NUM			(1)
#define	LOG_MAX_NUM				(40)
#define	LOG_MAX_STR				(120)

#define MAX_PRINT_LEN       	(1024)

//default config
#define DEVICE_300C                               "300c"
#define DEVICE_300MC                              "300mc"
#define DEVICE_300L                               "300l"
#define	CONFIG_PATH				                        "/usr/share/ginny"
#define SYSTEM_CONF                               "system.conf"
#define	NETWORK_CONF                              "network.conf"
#define	VERSION_CONF                              "/etc/ginny_version"
//working config
#define WORKING_CONF                              "/tmp/working.conf"
#define USER_CONF                                 "/userdata/user.conf"
#define DEV_CONF                                  "/userdata/dev.conf"

//This is magic number.
#define MSG_KEY     (12345)

//NODE property
#define NODE_SYS                    (0x00)
#define NODE_USER                   (0x01)
#define NODE_NONE                   (0x10)

//message queue config
//This is for bcd daemon.
#define MSG_COMMIT_INTERVAL         (1)	//periodic commit
#define MSG_COMMIT_MIN_INTERVAL     (1)	//force commit limit (ignore commit)

//for message queue
#define MSG_MAX_NAME_LEN            (128)
#define MSG_MAX_DATA_LEN            (1024)
#define MSG_MAX_REAL_DATA_LEN       (800)

//for syslog output message
#define SYSLOG_LINE_LENGTH			(256)

//**********************************************************************
//message queue' message type
//1~0x8000 : send message type
#define MTYPE_SELF                  (1)	//self msg. Not used.
#define MTYPE_APP                   (11)	//command line apps.
#define MTYPE_LOG                   (12)	//logging system

//All return mtype is  0x8000 + pid(caller), pid = msg.pid
//0x8000~ : receive message type (with process' pid)
//0x8000 is PID_MAX_DEFAULT for pid_t
#define MTYPE_RET                   (0x8000)
//**********************************************************************

//message queue' command type
#define CMD_READ                    (1)
#define CMD_WRITE                   (2)	//add & update
#define CMD_DELETE                  (3)
#define CMD_COMMIT                  (4)
#define CMD_HOLD_COMMIT             (5)	//hold commit for management
#define CMD_RESUME_COMMIT           (6)	//resume commit
#define CMD_RELOAD_FILE             (11)
#define CMD_SAVE_FILE               (12)
#define CMD_SAVE_NVRAM_FILE         (13)
#define CMD_UPDATE_FROM_FILE        (14)
#define CMD_LOG_DELETE				(21)
#define CMD_LOG_SHOW				(22)

#define CMD_RET_OK                  (100)
#define CMD_RET_FAIL                (101)

#define NULL_STR                    ""



#ifdef __cplusplus
}
#endif


#endif
