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

typedef enum {
    BLUETOOTH,
} process_type_t;


typedef enum{
    INIT_STATE,
    SUCCESS_ACK,
    SENDFAIL,
    NORESPONSE, // DEADLINE
}ack_state_t;

typedef struct {
//	char fifo_path[32];
//	pid_t  pid;
	int key[4]; //这个 key[0] 用于验证 2.R ,key[1] 用于验证 3.R key[2] 用于验证 4.R
//	char context [32];
} node_t;

typedef struct {
    char context[64*1024];
    char fifo_path[32];
    pid_t pid;
    process_type_t process_type;
    int count;
    int dead_line;
} msg_info_t;


//buff_to_send.msg_info 
//这个数据 是用来判断接下来做什么事情的
//buff_to_send.node
//这个数据是用来插入节点的
typedef struct {
    //node_t  node;  //因为是共享内存中的东西,不能用A进程访问 B 进程 malloc 的东西,所以不用指针
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

    //	volatile pthread_rwlock_t lock;

    volatile sem_t       			sem; //这个不能用 volatile 修饰 ,修饰会报错 ,但不修饰会不会出现错误 //TODO
    volatile char read_write_state; //对 发送buf 和 接收 buf 的 状态管理
    volatile char unwriteable_times_send; // 想写 buff_to_send 的次数
    volatile char unwriteable_times_recv; // 想写 buff_to_recv 的次数  //目前buff_to_recv 已经做到 管道里面了,但仍然想用 unwriteable_times_recv 来维护,但是还未做
    volatile msg_send_t 	buff_to_send; // 从 ws client 发送 到 ws server 的数据
    //sem_t       			sem; //这个不能用 volatile 修饰 ,修饰会报错 ,但不修饰会不会出现错误 //TODO
    //volatile shm_state_t 	shm_state; 
    //shm_state_t 	shm_state; 
    //volatile char 			buff_to_recv[BUFFSIZE];
    //msg_send_t 	buff_to_send; // 从 ws client 发送 到 ws server 的数据
    //char 			buff_to_recv[BUFFSIZE];
};



#if 0
typedef enum {
    R1,
    R2,
    R3,
    RA,//active
    Unknown_state
} msg_type_t;

#endif
typedef enum {
    RECV_SEND,
    RECV_ACK,
    RECV_ACTIVE,//active
    Unknown_state
} msg_type_t;

#endif
