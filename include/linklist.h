#ifndef __LINKLIST__
#define __LINKLIST__

typedef struct {
//	char fifo_path[32];
//	pid_t  pid;
	int key[3]; //这个 key[0] 用于验证 2.R ,key[1] 用于验证 3.R key[2] 用于验证 4.R
//	char context [32];
} node_t;

//
// typedef struct LIST
// {
//         struct LIST * next;
//         node_t data;
// }linked_list_t;
//
//
//
// linked_list_t * create_linkedlist(void);
// int print_count_linkedlist(linked_list_t * list_pointer);
//
// int add_rear_linkedlist(linked_list_t *list_pointer,node_t * data,int lenth);
//
// int delete_linklist_all(linked_list_t * list_pointer);
//
// int delete_linklist_but_head_node(linked_list_t * list_pointer);
//
// node_t * del_front_linkedlist(linked_list_t * list_pointer,node_t * data_pointer);
#endif
