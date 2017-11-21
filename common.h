#ifndef __COMMON_H__
#define __COMMON_H__

#include <semaphore.h>


#define BUFFSIZE 64 * 1024  //实际上要求 64KB 也就是 64 * 1024
#define SHM_PATH "/tmp/shm1"

typedef enum {
	WRITEABLE,
	READABLE,
	NUMBER_OF_MEMBERS,
} shm_state_t;




struct shm////共享内存使用的结构体的声明 
{

	sem_t       			sem; //这个不能用 volatile 修饰 ,修饰会报错 ,但不修饰会不会出现错误 //TODO
    volatile shm_state_t 	shm_state; 
	volatile char 			buff_to_send[BUFFSIZE]; // 从 ws client 发送 到 ws server 的数据
	volatile char 			buff_to_recv[BUFFSIZE];
};



#endif
