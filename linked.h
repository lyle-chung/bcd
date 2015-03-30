#ifndef __LINKED_H__
#define __LINKED_H__


/*******************************
Linked list function proto (locally function)
*******************************/
struct listnode
{
	struct listnode *next;
	struct listnode *prev;
	void     *data;
};

struct list
{
	struct listnode *head;
	struct listnode *tail;
	unsigned int count;
	int       (*cmp) (void *val1, void *val2);
	void      (*del) (void *val);
};

#define nextnode(X) ((X) = (X)->next)
#define listhead(X) ((X)->head)
#define listcount(X) ((X)->count)
#define list_isempty(X) ((X)->head == NULL && (X)->tail == NULL)
#define getdata(X) ((X)->data)




/* List iteration macro. */
#define LIST_LOOP(L,V,N) \
  for ((N) = (L)->head; (N); (N) = (N)->next) \
    if (((V) = (N)->data) != NULL)

/* List node add macro.  */
#define LISTNODE_ADD(L,N) \
  do { \
    (N)->prev = (L)->tail; \
    if ((L)->head == NULL) \
      (L)->head = (N); \
    else \
      (L)->tail->next = (N); \
    (L)->tail = (N); \
  } while (0)

/* List node delete macro.  */
#define LISTNODE_DELETE(L,N) \
  do { \
    if ((N)->prev) \
      (N)->prev->next = (N)->next; \
    else \
      (L)->head = (N)->next; \
    if ((N)->next) \
      (N)->next->prev = (N)->prev; \
    else \
      (L)->tail = (N)->prev; \
  } while (0)



//############################################
//FLASH entry info.
//############################################
typedef struct __NODE {
	unsigned char	*name;
	unsigned char	*value;
	
	//specific call function
	int (*job)(void*);

} NODE;



/* Prototypes. */
extern struct list *list_new();
extern void      list_free(struct list *);

extern void      listnode_add(struct list *, void *);
extern void      listnode_add_sort(struct list *, void *);
extern void      listnode_add_after(struct list *, struct listnode *, void *);
extern void      listnode_delete(struct list *, void *);
extern struct listnode *listnode_lookup(struct list *, void *);
extern void     *listnode_head(struct list *);

extern void      list_delete(struct list *);
extern void      list_delete_all_node(struct list *);

extern void nodelist_init();
extern NODE   *node_lookup_by_name(unsigned char *name);


#endif
