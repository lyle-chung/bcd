#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "linked.h"

/*******************************
prototype
*******************************/
struct list *list_new();
void list_free(struct list *);
struct listnode *listnode_new();
void listnode_add(struct list *, void *);
void listnode_add_sort(struct list *, void *);
void listnode_add_after(struct list *, struct listnode *, void *);
void listnode_delete(struct list *, void *);
struct listnode *listnode_lookup(struct list *, void *);
void *listnode_head(struct list *);
void list_delete(struct list *);
void list_delete_all_node(struct list *);

/*******************************
global variable
*******************************/

/*******************************
locally functions
*******************************/

/* Allocate new header list. */
struct list *list_new()
{
	struct list *new;

	new = malloc(sizeof(struct list));
	memset(new, 0, sizeof(struct list));
	new->cmp = NULL;
	new->del = NULL;
	return new;
}

/* Free header list. */
void list_free(struct list *l)
{
	free(l);
}

/* Allocate new listnode.  Internal use only. */
struct listnode *listnode_new()
{
	struct listnode *node;

	node = malloc(sizeof(struct listnode));
	memset(node, 0, sizeof(struct listnode));
	return node;
}

/* Free listnode. */
void listnode_free(struct listnode *node)
{
	free(node);
}

/* Add new data to the list. */
void listnode_add(struct list *list, void *val)
{
	struct listnode *node;

	node = listnode_new();

	node->prev = list->tail;
	node->data = val;

	if (list->head == NULL)
		list->head = node;
	else
		list->tail->next = node;
	list->tail = node;

	list->count++;
}

/* Add new node with sort function. */
void listnode_add_sort(struct list *list, void *val)
{
	struct listnode *n;
	struct listnode *new;

	new = listnode_new();
	new->data = val;

	if (list->cmp) {
		for (n = list->head; n; n = n->next) {
			if ((*list->cmp) (val, n->data) < 0) {
				new->next = n;
				new->prev = n->prev;

				if (n->prev)
					n->prev->next = new;
				else
					list->head = new;
				n->prev = new;
				list->count++;
				return;
			}
		}
	}

	new->prev = list->tail;

	if (list->tail)
		list->tail->next = new;
	else
		list->head = new;

	list->tail = new;
	list->count++;
}

void listnode_add_after(struct list *list, struct listnode *pp, void *val)
{
	struct listnode *nn;

	nn = listnode_new();
	nn->data = val;

	if (pp == NULL) {
		if (list->head)
			list->head->prev = nn;
		else
			list->tail = nn;

		nn->next = list->head;
		nn->prev = pp;

		list->head = nn;
	} else {
		if (pp->next)
			pp->next->prev = nn;
		else
			list->tail = nn;

		nn->next = pp->next;
		nn->prev = pp;

		pp->next = nn;
	}
}

/* Delete specific date pointer from the list. */
void listnode_delete(struct list *list, void *val)
{
	struct listnode *node;

	for (node = list->head; node; node = node->next) {
		if (node->data == val) {
			if (node->prev)
				node->prev->next = node->next;
			else
				list->head = node->next;

			if (node->next)
				node->next->prev = node->prev;
			else
				list->tail = node->prev;

			list->count--;
			listnode_free(node);
			return;
		}
	}
}

/* Return first node's data if it is there.  */
void *listnode_head(struct list *list)
{
	struct listnode *node;

	node = list->head;

	if (node)
		return node->data;
	return NULL;
}

/* Delete all listnode from the list. */
void list_delete_all_node(struct list *list)
{
	struct listnode *node;
	struct listnode *next;

	for (node = list->head; node; node = next) {
		next = node->next;
		if (list->del)
			(*list->del) (node->data);
		listnode_free(node);
	}
	list->head = list->tail = NULL;
	list->count = 0;
}

/* Delete all listnode then free list itself. */
void list_delete(struct list *list)
{
	struct listnode *node;
	struct listnode *next;

	for (node = list->head; node; node = next) {
		next = node->next;
		if (list->del)
			(*list->del) (node->data);
		listnode_free(node);
	}
	list_free(list);
}

/* Lookup the node which has given data. */
struct listnode *listnode_lookup(struct list *list, void *data)
{
	struct listnode *node;

	for (node = list->head; node; nextnode(node)) {
		if (data == getdata(node)) {
			return node;
    }
  }
	return NULL;
}

//############################
//NODE specific Fn.
//User' API
//############################
/* Create new node structure. */

#if 0

NODE *node_new()
{
	NODE *pNode;

	pNode = malloc(sizeof(NODE));
	memset(pNode, 0, sizeof(NODE));
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
		//special char # --> ' '
		if (*(str + i) == '#')
			*(str + i) = ' ';
		//CR is same as NULL/end of word.
		if (*(str + i) == 0x0d)
			*(str + i) = 0x00;
	}
	return;
}

/* set default node structure. */
/* if noe exist, first create */
NODE *node_update(char *name, char *value)
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
int register_node_handler(NODE * pNode, int (*job) (void *))
{
	if (pNode && job) {
		pNode->job = job;
	} else {
		return -1;
	}
	return 0;
}

#endif
