/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>

#include "const.h"
#include "linked.h"
#include "flash.h"

/*******************************
prototype
*******************************/
extern void PRINT_MSG(const char *format, ...);
extern int get_bootcfg(char *name, char *val);
extern int set_bootcfg(char *name, char *vals);
extern void PRINT_SYSLOG(int priority, const char *format, ...);
extern int default_node_handler(void *);
extern char *config_file;

#define BUF_SIZE	(1024)

/*******************************
global variable
*******************************/
struct list *nodelist;


//############################
//NODE specific Fn.
//User' API
//############################
/* Create new node structure. */
NODE *node_new()
{
	NODE *pNode;

	pNode = malloc(sizeof(NODE));
	memset(pNode, 0, sizeof(NODE));
    pNode->user = NODE_SYS;
	return pNode;
}

/* existance check */
NODE *node_lookup_by_name(char *name)
{
	struct listnode *node;
	NODE *pNode;

	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		if (strcmp(pNode->name, name) == 0)
			return pNode;
	}
	return NULL;
}

void string_check_and_conv(char *name, char *str)
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

/* register function handler */
int register_node_handler(NODE * pNode, int (*job) (void *))
{
	if (pNode && job) {
		pNode->job = job;
	} else {
		return -1;
	}
	return 0;
}

/* set default node structure. */
/* if noe exist, first create */
NODE *node_update(char *name, char *value, char user)
{
	NODE *pNode;
	char *name_str;
	char *value_str;

	name_str = malloc(128);
	value_str = malloc(1024);

	//valid check and convert special character: #
	string_check_and_conv(name, name_str);
	string_check_and_conv(value, value_str);

	pNode = node_lookup_by_name(name_str);
	if (!pNode) {
		//create and update
		pNode = node_new();
		pNode->name = NULL;
		pNode->value = NULL;
		listnode_add(nodelist, pNode);
	} else {
		//just update
		//first, free
		if (pNode->name)
			free(pNode->name);
		if (pNode->value)
			free(pNode->value);
	}

	if (name_str)
		pNode->name = strdup(name_str);
	if (value)
		pNode->value = strdup(value_str);

    //check NODE is SYS or User
    if (user == NODE_USER) 
        pNode->user = user;
    else 
        pNode->user = NODE_SYS;

	//register default node action
	register_node_handler(pNode, default_node_handler);

	free(name_str);
	free(value_str);
	return pNode;
}

/* Delete and free ratestructure. */
void node_delete(NODE * pNode)
{
	if (pNode->name)
		free(pNode->name);
	if (pNode->value)
		free(pNode->value);
	listnode_delete(nodelist, pNode);
	free(pNode);
}

//############################
//Common API
//User' API
//############################

void display_nodelist_all()
{
	struct listnode *node;
	NODE *pNode;

	//display all node
	PRINT_MSG("-------------------------------------");
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		PRINT_MSG("[%s]--[%s]", pNode->name, pNode->value);
	}
	PRINT_MSG("-------------------------------------");
}

int set_lowdata_with_node(flash_s * flash)
{
	struct listnode *node;
	NODE *pNode;
	char *p;
	int len;

	memset(flash->data, 0x0, sizeof(flash->data));

	p = &(flash->data[0]);
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		//PRINT_MSG("\n[%s]--[%s]", pNode->name, pNode->value);

		//add node into buffer
		len = sprintf(p, "%s=%s", pNode->name, pNode->value);
		p = p + len;
		*p = '\0';
		p++;
	}
	return 0;
}

//READ NODE
int read_node(char *name, char *value)
{
	NODE *pNode;

	pNode = node_lookup_by_name(name);
	if (!pNode) {
		//not found, set NULL
		//value = NULL;
		return -1;
	} else {
		strcpy(value, pNode->value);
		return (int)strlen(value);
	}
}

//WRITE, CREATE & UPDATE
int update_node(char *name, char *value, char user)
{
	if (strlen(name) > 0)
		node_update(name, value, user);
	return 0;
}

//DELETE NODE
int delete_node(char *name)
{
	NODE *pNode;

	pNode = node_lookup_by_name(name);
	if (pNode) {
		node_delete(pNode);
		return 0;
	}
	return -1;
}

int save_nodelist_into_file(char *fname)
{
	struct listnode *node;
	NODE *pNode;
	FILE *fp;
	FILE *fp_user;
    char tmp_file[256];
    char bak_file[256];
    char cmd[256];

    if (!fname)
        return -1;

    //get bak filename
    sprintf(tmp_file, "%s.tmp", config_file);
    sprintf(bak_file, "%s.bak", config_file);

	//check filename for ALL config dump file
	fp = fopen(fname, "w");
	if (fp == NULL) {
		PRINT_MSG("can't create dump file[%s], ignored.", fname);
		return -1;
	}
	//check filename for USER/DEV config
	fp_user = fopen(tmp_file, "w");
	if (fp_user == NULL) {
		PRINT_MSG("can't create dump file[%s], ignored.", tmp_file);
		return -1;
	}
	//display all node
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
        //write ALL
   		fprintf(fp, "%s=%s\n", pNode->name, pNode->value);
        //and write USER only
        if (pNode->user == NODE_USER) {
    		fprintf(fp_user, "%s=%s\n", pNode->name, pNode->value);
        }
	}
    
    //flush file
    fflush(fp);
    fflush(fp_user);
	fclose(fp);
	fclose(fp_user);

    //backup config to bak
    if (access(bak_file, F_OK ) == 0 ) {
        sprintf(cmd, "rm %s", bak_file);
        system(cmd);
        sprintf(cmd, "mv %s %s", config_file, bak_file);
        system(cmd);
    }
    //rename
    if (access(tmp_file, F_OK ) == 0 ) {
        sprintf(cmd, "mv %s %s", tmp_file, config_file);
        system(cmd);
    }

	return 0;
}

int save_log_into_file(char *fname)
{
	NODE *pNode;
	FILE *fp;
	int start, loop;
	char buf[16];

	//check filename
	fp = fopen(fname, "w");
	if (fp == NULL) {
		PRINT_MSG("can't create dump file[%s], ignored.", fname);
		return -1;
	}
	//get log start pointer
	pNode = node_lookup_by_name(LOG_START);
	if (pNode == NULL) {
		PRINT_MSG("log not found");
		return 0;
	}
	start = atoi(pNode->value);

	//display first half log
	for (loop = start; loop <= LOG_MAX_NUM; loop++) {
		sprintf(buf, "%s%d", LOG_HEADER, loop);
		pNode = node_lookup_by_name(buf);
		if (pNode == NULL)
			continue;

		//found log and display
		if (strlen(pNode->value) > 0)
			fprintf(fp, "%s\n", pNode->value);
	}

	//display last half log
	for (loop = LOG_START_NUM; loop < start; loop++) {
		sprintf(buf, "%s%d", LOG_HEADER, loop);
		pNode = node_lookup_by_name(buf);
		if (pNode == NULL)
			continue;

		//found log and display
		if (strlen(pNode->value) > 0)
			fprintf(fp, "%s\n", pNode->value);
	}

	fclose(fp);
	return 0;
}

int delete_log_all()
{
	struct listnode *node;
	NODE *pNode;

	//just delete node' value
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		if (strncmp(pNode->name, LOG_HEADER, strlen(LOG_HEADER)) == 0) {
			update_node(pNode->name, "", NODE_NONE);
		}
	}

	//update start log
	update_node(LOG_START, "1", NODE_NONE);
	return 0;
}

int clear_nodelist()
{
	struct listnode *node;
	NODE *pNode;

	//-----------------------
	//clean all nodes in list
	//-----------------------
	for (node = listhead(nodelist); node; node = listhead(nodelist)) {
		pNode = getdata(node);
		node_delete(pNode);
	}
	return 0;
}

int reload_nodelist_from_file(char *fname)
{
	FILE *fp;
	char buf[BUF_SIZE];
	char *p;

	//check filename
	fp = fopen(fname, "r");
	if (fp == NULL) {
		PRINT_MSG("can't read config file[%s], ignored.", fname);
		return -1;
	}

    clear_nodelist();

	//re-new nodes
	while (NULL != fgets(buf, BUF_SIZE, fp)) {
		if (buf[0] == '\n' || buf[0] == '#')
			continue;
		if (NULL == (p = strchr(buf, '='))) {
			continue;
		}
		buf[strlen(buf) - 1] = '\0';	//remove carriage return
		*p++ = '\0';	//seperate the string
		//printf("bufset '%s'='%s'\n", buf, p);

		update_node(buf, p, NODE_NONE);
	}
	fclose(fp);
	PRINT_MSG("reloaded config file[%s]", fname);

	return 0;
}

int update_nodelist_from_file(char *fname, char user)
{
	FILE *fp;
	char name[BUF_SIZE];
	char *value;

	//check filename
	fp = fopen(fname, "r");
	if (fp == NULL) {
		PRINT_MSG("can't read config file[%s], ignored.", fname);
		return -1;
	}

	//re-new nodes
	while (NULL != fgets(name, BUF_SIZE, fp)) {
		if (name[0] == '\n' || name[0] == '#')
			continue;
		if (NULL == (value = strchr(name, '='))) {
			continue;
		}
		name[strlen(name) - 1] = '\0';	//remove carriage return
		*value++ = '\0';	//seperate the string
		//printf("bufset '%s'='%s'\n", name, value);

		update_node(name, value, user);
	}
	fclose(fp);
	PRINT_MSG("loaded config file[%s]", fname);

	return 0;
}

int get_log_start()
{
	NODE *pNode;
	char buf[16];

	//if not exist logging start, create first.
	pNode = node_lookup_by_name(LOG_START);
	if (pNode == NULL) {
		PRINT_MSG("create new log start.");
		sprintf(buf, "%d", LOG_START_NUM);
		node_update(LOG_START, buf, NODE_NONE);
		return LOG_START_NUM;
	} else {
		return atoi(pNode->value);
	}
}

int set_log_start(int start)
{
	char buf[16];

	sprintf(buf, "%d", start);
	node_update(LOG_START, buf, NODE_NONE);
	return 0;
}

int get_boot_env(const char *name, char *value) {
    FILE *fp;
    char *ptr;
    static char buf[128];
    static char output[512];
    int read_size;

    memset(buf, 0, sizeof(buf));
    memset(output, 0, sizeof(output));

    sprintf(buf, "fw_printenv -n %s", name);
    fp = popen(buf, "r");
    read_size = fread(output, 1, 512, fp);
    pclose(fp);
    
    if (read_size <= 0) {
        value = NULL;
        return 0;
    }
    
    ptr = output;
    while (*ptr != '\0') {
        if (*ptr == 0xa || *ptr == 0xd) {
            *ptr = '\0';
            continue;
        }
        ptr++;
    }
    strcpy(value, output);

    return strlen(value);
}

char *get_bootcfg_from_flash()
{
    char name[256];
    char value[256];
    int ret;
    char *device;

    //serial
    sprintf(name, "serial");
    memset(value, 0, sizeof(value));
    ret = get_boot_env(name, value);
    if (!ret)
        node_update(name, "H-00000000", NODE_NONE);
    else
        node_update(name, value, NODE_NONE);
    PRINT_MSG("update: %s = %s", name, value);
    
    //device: valid id= 300C(default), 300MC, 300L
    sprintf(name, "device");
    memset(value, 0, sizeof(value));
    ret = get_boot_env(name, value);
    if (!ret)
        node_update(name, "300C", NODE_NONE);
    else
        node_update(name, value, NODE_NONE);
    PRINT_MSG("update: %s = %s", name, value);
    //update device
    if (strcmp(value, "300C")==0 || strcmp(value, "300c")==0) {
        device = DEVICE_300C;
    } else if (strcmp(value, "300MC")==0 || strcmp(value, "300mc")==0) {
        device = DEVICE_300MC;
    } else if (strcmp(value, "300L")==0 || strcmp(value, "300l")==0) {
        device = DEVICE_300L;
    }
    
    //feature
    sprintf(name, "features");
    memset(value, 0, sizeof(value));
    ret = get_boot_env(name, value);
    if (!ret)
        node_update(name, "C", NODE_NONE);
    else
        node_update(name, value, NODE_NONE);
    PRINT_MSG("update: %s = %s", name, value);
    
    //key(in future)
   

	return device;
}

void register_default_handler()
{
//	PRINT_MSG("Node handler is successfully registered.");
}

void nodelist_init()
{
	//create list header
	nodelist = list_new();
}
