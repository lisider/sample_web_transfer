#include <stdio.h>
#include <stdlib.h>
#include "linklist.h"
#include <string.h>
#include "print_color.h"
//创建一个节点

linked_list_t * create_linkedlist(void)
{
	linked_list_t * list_pointer = malloc(sizeof(struct LIST));
	list_pointer->next = NULL;
	return list_pointer;
}

//判空

int isnull(linked_list_t * list_pointer)
{
	return list_pointer->next ==NULL;
}

//插入节点之从前插入

int add_front_linkedlist(linked_list_t * list_pointer,node_t data)
{
	linked_list_t * newnode = malloc(sizeof(struct LIST));
	newnode->next = list_pointer->next;
	list_pointer->next = newnode;
	newnode->data = data;
	return 0;
}

//插入节点之从后插入

int add_rear_linkedlist(linked_list_t *list_pointer,node_t * data,int lenth)
{
	linked_list_t * tmp_pointer = list_pointer;
	linked_list_t * newnode = malloc(sizeof(struct LIST));
	while(tmp_pointer->next != NULL)
	{
		tmp_pointer = tmp_pointer->next;
	}
	newnode->next = NULL;
	tmp_pointer->next = newnode;
	memcpy(&(newnode->data),data, lenth);
	return 0;
}

//插入节点之从指定位置后插入

int add_postition_spec_rear_linkedlist(linked_list_t * list_pointer,linked_list_t * position,node_t data)
{
	linked_list_t * tmp_pointer = list_pointer;
	if(list_pointer == NULL)
		return -1;
	if(position == NULL)
		return -1;
	while(tmp_pointer->next != NULL)
	{	
		tmp_pointer = tmp_pointer->next;
		if(tmp_pointer == position)
		{
			linked_list_t * newnode = malloc(sizeof(struct LIST));
			newnode->next = tmp_pointer->next;
			tmp_pointer->next = newnode;
			newnode->data = data;
			return 1;
		}
	}
	return 0;
}

//插入节点之从指定位置前插入

int add_postition_spec_front_linkedlist(linked_list_t * list_pointer,linked_list_t * position,node_t data)
{
	linked_list_t * tmp_pointer = list_pointer;
	if(list_pointer == NULL)
		return -1;
	if(position == NULL)
		return -1;
	while(tmp_pointer->next != NULL)
	{	
		if(tmp_pointer->next == position)
		{
			linked_list_t * newnode = malloc(sizeof(struct LIST));
			newnode->next = tmp_pointer->next;
			tmp_pointer->next = newnode;
			newnode->data = data;
			return 1;
		}
		tmp_pointer = tmp_pointer->next;
	}
	return 0;
}

//删除节点之从前删

node_t * del_front_linkedlist(linked_list_t * list_pointer,node_t * data_pointer)
{
	if(isnull(list_pointer) || list_pointer == NULL)
		return NULL;

	linked_list_t * tmp_pointer_free = NULL;
	tmp_pointer_free = list_pointer->next;

	//得到第一个节点的数据
	if(data_pointer != NULL)
		*data_pointer = list_pointer->next->data;
	list_pointer->next = list_pointer->next->next;
	//释放节点
	free(tmp_pointer_free);
	return data_pointer;
}

//删除节点之从后删

node_t * del_rear_linkedlist(linked_list_t * list_pointer,node_t * data_pointer)
{
	if(isnull(list_pointer) || list_pointer == NULL)
		return NULL;

	linked_list_t * tmp_pointer_free = NULL;

	linked_list_t * tmp_pointer = list_pointer;
	while(tmp_pointer->next->next != NULL)
	{
		tmp_pointer = tmp_pointer->next;
	}

	tmp_pointer_free = tmp_pointer->next;

	if(data_pointer != NULL)
		*data_pointer = tmp_pointer->next->data;
	tmp_pointer->next = tmp_pointer->next->next;

	free(tmp_pointer_free);

	return data_pointer;
}

//删除节点之从指定位置删

int del_position_spec_linkedlist(linked_list_t * list_pointer,linked_list_t * position)
{
	if(isnull(list_pointer) || list_pointer == NULL)
		return -1;
	if(position == NULL)
		return -1;

	linked_list_t * tmp_pointer_free = NULL;

	linked_list_t * tmp_pointer = list_pointer;
	while(tmp_pointer->next != NULL)
	{
		if(tmp_pointer->next == position)
		{
			tmp_pointer_free = tmp_pointer->next;
				
			tmp_pointer->next = tmp_pointer->next->next;

			free(tmp_pointer_free);	
			return 0;
		}
		tmp_pointer = tmp_pointer->next;	
	}
	return -1;
}

//查找节点之按数据查找

linked_list_t * locate_linkedlist(linked_list_t * list_pointer,node_t * data, int lenth)
{
	if(isnull(list_pointer) || list_pointer == NULL)
		return NULL;
	linked_list_t * tmp_pointer = list_pointer;
	while(tmp_pointer->next != NULL)
	{
		tmp_pointer = tmp_pointer->next;

       //int memcmp(const void *s1, const void *s2, size_t n);
		if(memcmp(&(tmp_pointer->data),&data,lenth) == 0)
		{
			return tmp_pointer;
		}
	}
	return NULL;
}

//改变节点之按位置改

int alter_linkedlist(linked_list_t * list_pointer,linked_list_t * position,node_t data)
{
	if(isnull(list_pointer) || list_pointer == NULL)
		return -1;
	if(position == NULL)
		return -1;
	linked_list_t * tmp_pointer = list_pointer;
	while(tmp_pointer->next != NULL)
	{
		if(tmp_pointer->next == position)
		{
			tmp_pointer->next->data = data;
			return 0;
		}
		tmp_pointer = tmp_pointer->next;
	}	
	return -1;
}

//打印节点之顺序打印

int print_sequence_linkedlist(linked_list_t * list_pointer)
{
	int count = 0;
	if(isnull(list_pointer))
	{
		printf(PURPLE "%d nodes in the linklist\n" NONE,count);
		return -1;
	}

	linked_list_t * tmp_pointer = list_pointer;
	do
	{
		//printf("%d\t",tmp_pointer->next->data);
	//	printf("%s\t",tmp_pointer->next->data.context);
		count ++;
		tmp_pointer = tmp_pointer->next;
	}while(tmp_pointer->next != NULL);
//	putchar('\n');
	printf(PURPLE "%d nodes in the linklist\n" NONE,count);
	return 0;
}

//打印 有多少节点

int print_count_linkedlist(linked_list_t * list_pointer)
{
	int count = 0;
	if(isnull(list_pointer))
	{
		printf(PURPLE "%d nodes in the linklist\n" NONE,count);
		return -1;
	}

	linked_list_t * tmp_pointer = list_pointer;
	do
	{
		//printf("%d\t",tmp_pointer->next->data);
	//	printf("%s\t",tmp_pointer->next->data.context);
		count ++;
		tmp_pointer = tmp_pointer->next;
	}while(tmp_pointer->next != NULL);
//	putchar('\n');
	printf(PURPLE "%d nodes in the linklist\n" NONE,count);
	return 0;
}





//打印节点之逆序打印

int print_reverse_linkedlist(linked_list_t * list_pointer)
{
	/*
	 *注意：此函数被引用时，下一句一定要加一个 putchar('\n');
	 * */
	if(isnull(list_pointer))
		return -1;

	if(list_pointer->next == NULL)
		return 0;
	linked_list_t * tmp_pointer = list_pointer;
	if(tmp_pointer->next->next != NULL)
	{
		tmp_pointer = tmp_pointer->next;
		print_reverse_linkedlist(tmp_pointer);
		printf("%s\t",tmp_pointer->data.context);
	}else	
	{
		printf("%s\t",tmp_pointer->next->data.context);
	}
	return 0;
}


int delete_linklist_but_head_node(linked_list_t * list_pointer){
	if(list_pointer == NULL)
		return -1;
	if(list_pointer->next == NULL)
		return 0;
	linked_list_t * tmp_pointer = list_pointer;


	if(tmp_pointer->next->next != NULL)
	{
		tmp_pointer = tmp_pointer->next;
		delete_linklist_but_head_node(tmp_pointer);
		free(tmp_pointer);
	}else	
	{
		free(tmp_pointer->next);
	}
	return 0;
}


int delete_linklist_all(linked_list_t * list_pointer){

	int ret = 0;
	if(list_pointer == NULL)
		return -1;
	ret = delete_linklist_but_head_node(list_pointer);
	if(ret == -1)
		return -1;
	free(list_pointer);
	return 0;

}

//测试

#if 0
int main(int argc, const char * argv[])
{
	node_t data;
	linked_list_t * list_pointer = create_linkedlist();
	int i;
	for(i=1;i<=10;i++)
		add_rear_linkedlist(list_pointer,&i,sizeof(i));
	print_sequence_linkedlist(list_pointer);
#if 1
	add_postition_spec_rear_linkedlist(list_pointer,locate_linkedlist(list_pointer,10),6);
	print_sequence_linkedlist(list_pointer);
	add_postition_spec_front_linkedlist(list_pointer,locate_linkedlist(list_pointer,10),5);
	print_sequence_linkedlist(list_pointer);
	del_front_linkedlist(list_pointer,&data);
	print_sequence_linkedlist(list_pointer);
	printf("\n\n%d\n\n",data);
	del_rear_linkedlist(list_pointer,&data);
	print_sequence_linkedlist(list_pointer);
	printf("\n\n%d\n\n",data);
#endif
	del_position_spec_linkedlist(list_pointer,locate_linkedlist(list_pointer,2));
	print_sequence_linkedlist(list_pointer);
	alter_linkedlist(list_pointer,locate_linkedlist(list_pointer,3),300);
	print_sequence_linkedlist(list_pointer);
	print_reverse_linkedlist(list_pointer);
	putchar('\n');
	return 0;
}
#endif
