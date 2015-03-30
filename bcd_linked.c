/* vi: set sw=4 ts=4: */
/*-----------------------------------------------------------------------------
 *
 * 
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>

#include "const.h"
#include "linked.h"
#include "flash.h"

/*******************************
prototype
*******************************/
extern void PRINT_MSG(const char *format, ...);
extern int get_bootcfg (char *name, char *val);
extern int set_bootcfg (unsigned char *name, unsigned char *vals);
extern void PRINT_SYSLOG(int priority, const char *format, ...);
extern int default_node_handler(unsigned char *);

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
NODE     *node_new()
{
	NODE     *pNode;

	pNode = malloc(sizeof(NODE));
	memset(pNode, 0, sizeof(NODE));
	return pNode;
}

/* existance check */
NODE     *node_lookup_by_name(unsigned char *name)
{
	struct listnode *node;
	NODE     *pNode;

	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		if (strcmp(pNode->name, name) == 0)
			return pNode;
	}
	return NULL;
}

void string_check_and_conv(unsigned char *name, unsigned char *str) {
	int i;

	strcpy(str, name);
	for(i=0; i<strlen(str); i++) {
		//special char " --> ' '
		if(*(str+i) == SPACE_CHARACTER)
			*(str+i) = ' ';
		//CR is same as NULL/end of word.
		if(*(str+i) == 0x0d)
			*(str+i) = 0x00;
	}
	return;
}



/* set default node structure. */
/* if noe exist, first create */
NODE     *node_update(unsigned char *name, unsigned char *value)
{
	NODE     *pNode;
	unsigned char	*name_str;
	unsigned char	*value_str;

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
	}
	else {
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

/* register function handler */
int register_node_handler(NODE *pNode, int (*job)(void*))
{
	if (pNode && job) {
		pNode->job = job;
	} else {
		return -1;
	}
	return 0;
}
	






//############################
//Common API
//User' API
//############################

void display_nodelist_all()
{
	struct listnode *node;
	NODE     *pNode;

	//display all node
	PRINT_MSG("-------------------------------------");
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		PRINT_MSG("[%s]--[%s]", pNode->name, pNode->value);
	}
	PRINT_MSG("-------------------------------------");
}

int get_node_from_lowdata(flash_s * flash)
{
	int       i;
	char     *p, *q;
	char      name[128], value[1024];

	//parse env to cache
	p = &(flash->data[0]);
	for (i = 0; i < 1024; i++) {
		if (NULL == (q = strchr(p, '='))) {
			//PRINT_MSG("parsed failed - cannot find '='");
			break;
		}
		*q = '\0';	//strip '='
		//fb[index].cache[i].name = strdup(p);
		strcpy(name, p);

		p = q + 1;	//value
		if (NULL == (q = strchr(p, '\0'))) {
			//PRINT_MSG("parsed failed - cannot find '\\0'\n");
			break;
		}
		//fb[index].cache[i].value = strdup(p);
		strcpy(value, p);

		//PRINT_MSG("name=[%s], value[%s]", name, value);
		node_update(name, value);

		p = q + 1;	//next entry
		//if ((p - flash + 1) >= len)   //end of block
		//  break;
		if (*p == '\0')	//end of env
			break;
	}
	return 0;
}

int set_lowdata_with_node(flash_s * flash)
{
	struct listnode *node;
	NODE     *pNode;
	char     *p;
	int       len;

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
int read_node(unsigned char *name, unsigned char *value)
{
	NODE     *pNode;

	pNode = node_lookup_by_name(name);
	if (!pNode) {
		//not found, set NULL
		//value = NULL;
		return -1;
	}
	else {
		strcpy(value, pNode->value);
		return strlen(value);
	}
}

//WRITE, CREATE & UPDATE
int update_node(unsigned char *name, unsigned char *value)
{
	NODE     *pNode;

	if(strlen(name) > 0) 
		pNode = node_update(name, value);
	return 0;
}

//DELETE NODE
int delete_node(unsigned char *name) {
	NODE     *pNode;

	pNode = node_lookup_by_name(name);
	if(pNode) {
		node_delete(pNode);
		return 0;
	}
	return -1;
}

int write_flag_into_check_file(unsigned char *check_file, unsigned char *str) {
	int	fd;

	fd = open(check_file, O_CREAT | O_WRONLY, 0700);
	if( fd < 0 ) {
		PRINT_SYSLOG(LOG_INFO, "can't create check file[%s], ignored.", check_file);
		return -1;
	}
	write(fd, "%s", str);
	close(fd);

	return 0;
}

int save_nodelist_into_file(char *fname)
{
	struct listnode *node;
	NODE     *pNode;
	FILE     *fp;
	unsigned char	check_file[64];
	int		ret;

#if 0
	//get check file name, create check file and set flag
	if(strncmp(fname, "config_", 7) == 0) {
		sprintf(check_file, "%s_status", fname);
		ret = write_flag_into_check_file(check_file, "pending");
		if(ret != 0) {
			return -1;
		}
	}
#endif

	//check filename
	fp = fopen(fname, "w");
	if (fp == NULL) {
		PRINT_MSG("can't create dump file[%s], ignored.", fname);
		return -1;
	}

	//display all node
	for (node = listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		fprintf(fp, "%s=%s\n", pNode->name, pNode->value);
		//PRINT_MSG("[%s]--[%s]", pNode->name, pNode->value);
	}

	fclose(fp);
	return 0;
}

int save_log_into_file(char *fname)
{
	struct listnode *node;
	NODE     *pNode;
	FILE     *fp;
	int 	start, loop;
	char	buf[8], *ppp;

	//check filename
	fp = fopen(fname, "w");
	if (fp == NULL) {
		PRINT_MSG("can't create dump file[%s], ignored.", fname);
		return -1;
	}

	//get log start pointer
    pNode = node_lookup_by_name(LOG_START);
    if(pNode==NULL) {
        PRINT_MSG("log not found");
		return 0;
	}
	start = atoi(pNode->value);

	//display first half log
	for(loop=start; loop<=LOG_MAX_NUM; loop++) {
		sprintf(buf, "%s%d", LOG_HEADER, loop);
    	pNode = node_lookup_by_name(buf);
		if(pNode==NULL)
			continue;
		
		//found log and display
		if(strlen(pNode->value)>0)
			fprintf(fp, "%s\n", pNode->value);
	}

	//display last half log
	for(loop=LOG_START_NUM; loop<start; loop++) {
		sprintf(buf, "%s%d", LOG_HEADER, loop);
    	pNode = node_lookup_by_name(buf);
		if(pNode==NULL)
			continue;
		
		//found log and display
		if(strlen(pNode->value)>0)
			fprintf(fp, "%s\n", pNode->value);
	}

	fclose(fp);
	return 0;
}

int delete_log_all()
{
	struct listnode *node;
	NODE     *pNode;
	FILE     *fp;
	int 	start, loop;
	char	buf[8];

	//just delete node' value
	for (node=listhead(nodelist); node; nextnode(node)) {
		pNode = getdata(node);
		if(strncmp(pNode->name, LOG_HEADER, strlen(LOG_HEADER)) == 0)  {
			update_node(pNode->name, "");
		}
	}
	
	//update start log
	update_node(LOG_START, "1");
	return 0;
}



int reload_nodelist_from_file(char *fname)
{
	struct listnode *node;
	NODE     *pNode;
	FILE     *fp;
	char      buf[BUF_SIZE];
	char     *p;

	//check filename
	fp = fopen(fname, "r");
	if (fp == NULL) {
		PRINT_MSG("can't read config file[%s], ignored.", fname);
		return -1;
	}

	//-----------------------
	//clean all nodes in list
	//-----------------------
	for (node = listhead(nodelist); node; node = listhead(nodelist)) {
		pNode = getdata(node);
		node_delete(pNode);
	}

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

		update_node(buf, p);
	}

	fclose(fp);
	return 0;
}


int get_log_start() {
    struct listnode *node;
    NODE     *pNode;
	char	buf[8];

    //if not exist logging start, create first.
    pNode = node_lookup_by_name(LOG_START);
    if(pNode==NULL) {
        PRINT_MSG("create new log start.");
		sprintf(buf, "%d", LOG_START_NUM);
        node_update(LOG_START, buf);
		return LOG_START_NUM ;
	} else {
		return atoi(pNode->value);
	}
}

int set_log_start(int start) {
	char	buf[16];

	sprintf(buf, "%d", start);
    node_update(LOG_START, buf);
	return 0;
}


int get_bootcfg_from_flash() {
	char	buf[BUF_SIZE];

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("bdv", &buf[0]);
	node_update("bdv", &buf[0]);
	PRINT_MSG("bootcfg: bdv=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("block1_name", &buf[0]);
	node_update("block1_name", &buf[0]);
	PRINT_MSG("bootcfg: block1_name=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("block2_name", &buf[0]);
	node_update("block2_name", &buf[0]);
	PRINT_MSG("bootcfg: block2_name=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("block1_start", &buf[0]);
	node_update("block1_start", &buf[0]);
	PRINT_MSG("bootcfg: block1_start=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("block2_start", &buf[0]);
	node_update("block2_start", &buf[0]);
	PRINT_MSG("bootcfg: block2_start=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("next_boot_block", &buf[0]);
	node_update("next_boot_block", &buf[0]);
	PRINT_MSG("bootcfg: next_boot_block=[%s]", &buf[0]);

	memset(&buf[0], 0, BUF_SIZE);
	get_bootcfg("ethaddr", &buf[0]);
	node_update("ethaddr", &buf[0]);
	PRINT_MSG("bootcfg: ethaddr=[%s]", &buf[0]);

	return 0;
}




void register_default_handler() {
	NODE	*pNode;
	int		ret = 0;


	PRINT_MSG("Node handler is successfully registered." );

}


void nodelist_init()
{
	//create list header
	nodelist = list_new();
}



