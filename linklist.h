#ifndef __LINKLIST__
#define __LINKLIST__

typedef struct {
	char context [30];
} node_t;


typedef struct LIST
{
	struct LIST * next;
	node_t data;
}linked_list_t;



linked_list_t * create_linkedlist(void);
int print_sequence_linkedlist(linked_list_t * list_pointer);

int add_rear_linkedlist(linked_list_t *list_pointer,node_t * data,int lenth);

int delete_linklist_all(linked_list_t * list_pointer);

#endif
