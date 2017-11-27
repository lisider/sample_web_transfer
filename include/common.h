#ifndef __COMMON_H__
#define __COMMON_H__

#include <semaphore.h>
#include "linklist.h"
#include <sys/types.h>
#include <unistd.h>


#define BUFFSIZE 64 * 1024  //实际上要求 64KB 也就是 64 * 1024
#define SHM_PATH "/tmp/ws_client_shm"

typedef enum {
	WRITEABLE,
	READABLE,
	NUMBER_OF_MEMBERS,
} shm_state_t;


typedef struct {

	char fifo_path[32];

	int key_1R; 

	pid_t pid;

	char context[64*1024];

} msg_info_t;


//buff_to_send.msg_info 
//这个数据 是用来判断接下来做什么事情的
//buff_to_send.node
//这个数据是用来插入节点的
typedef struct {
	node_t  node;  //因为是共享内存中的东西,不能用A进程访问 B 进程 malloc 的东西,所以不用指针
	msg_info_t msg_info;//同上
} msg_send_t; // 这个类型 至少要包括数据节点的类型


/*
typedef struct {
	char * tmp;
	pid_t  pid;
	char context [30];
} node_t;

*/


struct shm////共享内存使用的结构体的声明 
{

	volatile pthread_rwlock_t lock;

	volatile sem_t       			sem; //这个不能用 volatile 修饰 ,修饰会报错 ,但不修饰会不会出现错误 //TODO
	//sem_t       			sem; //这个不能用 volatile 修饰 ,修饰会报错 ,但不修饰会不会出现错误 //TODO
    volatile shm_state_t 	shm_state; 
    //shm_state_t 	shm_state; 
	volatile msg_send_t 	buff_to_send; // 从 ws client 发送 到 ws server 的数据
	volatile char 			buff_to_recv[BUFFSIZE];
	//msg_send_t 	buff_to_send; // 从 ws client 发送 到 ws server 的数据
	//char 			buff_to_recv[BUFFSIZE];
};



#endif
