#include <stdio.h>
#include <pthread.h>
#include "print_color.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"



pthread_t   pthid_write;

struct shm *shms;//结构体指针定义 

void thread_main (void){ //这个线程就是读线程
	int i;

	while(1){

		sleep(3);
		// 1. 接收数据

		sem_wait(&(shms->sem));
		for(i= 0;i< 11;i++){
			//printf("get buff_to_recv[%d]\tform ws_server to ws_client\n",i);
			shms->buff_to_recv[i] = (char)i;
		}

		printf(L_CYAN "get buff_to_send form ws_client to ws_client\n" NONE);



		// 2. 设置 可读 标志
		shms->shm_state = READABLE;

		sem_post(&(shms->sem));
	}
	return ;

}


static void * thread_write(void *arg){

	while(1){
		if(shms->shm_state == WRITEABLE){
			//发送数据

			sem_wait(&(shms->sem));
			printf(GREEN "send buff_to_send form process1 to ws_server\n" NONE);

			shms->shm_state = NUMBER_OF_MEMBERS;

			sem_post(&(shms->sem));
		}
	}



	return NULL;
}


static int shm_init(void){

	key_t key;//key定义
	int shmid;//共享内存id定义

	key = ftok(SHM_PATH,'r');//获取key
	if(-1 == key){
		perror("ftok");
		return -1; 
	}   
	shmid = shmget(key,sizeof(struct shm),IPC_CREAT|IPC_EXCL|0666);//共享内存的获取
	if(-1 == shmid){
		if(errno == EEXIST){
			shmid = shmget(key,sizeof(struct shm),0);
		}else{
			perror("shmget");
			return -1; 
		}   
	}   

	shms = shmat(shmid,NULL,0);//共享内存的映射
	if(-1 == *(int *)shms){
		perror("shmat");
		return -1; 
	}

	shms->shm_state = NUMBER_OF_MEMBERS; //这个状态不用来进行逻辑判断

	sem_init(&(shms->sem), 0, 1);

	return 0;
}


int main(int argc, const char *argv[])
{
	int ret = 0;

	ret = shm_init();
	if(ret)
		printf(RED "shm_init failed\n" NONE);

	if(0 != pthread_create(&pthid_write,NULL,thread_write,NULL)){
		perror("pthid1");
		return -1;
	}


	thread_main();
	return 0;
}
