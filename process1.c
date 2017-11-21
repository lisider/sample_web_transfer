#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "print_color.h"
#include <unistd.h>



struct shm *shms;//结构体指针定义 

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

	//因为这个程序是后于 ws_client 做的, ws_client 已经做了这个初始化,所以不需要
	//在 这里再做一次
//	shms->shm_state = NUMBER_OF_MEMBERS; //这个状态不用来进行逻辑判断

	return 0;
}



void fun_main(void){
	while(1){
		//read
	//	printf("shm state : %s\n", shms->shm_state == 0 ?("WRITEABLE"):(shms->shm_state == 1?("READABLE"):("NUMBER_OF_MEMBERS")));
		if(shms->shm_state == READABLE){

			sem_wait(&(shms->sem));
			//接下来要判断是不是自己的数据  ver2
				printf(YELLOW "del with buff_to_recv form ws_client\n" NONE);
				//read buff_to_recv

				shms->shm_state = NUMBER_OF_MEMBERS;

				//write 

				{


					// fill in buff_to_send with data
					shms->shm_state = WRITEABLE;
					printf(GREEN "send buff_to_send form process1 to ws_client\n" NONE);
				}

			sem_post(&(shms->sem));

		}
	}

	return ;
}

int main(int argc, const char *argv[]){

	int ret = 0;

	ret = shm_init();
	if(ret)
		printf(RED "shm_init failed\n" NONE);

	fun_main();

	return 0;

	//通过最后一个函数(共享内存的映射),shms指针指向的结构体成为共享内存中的结构体.
	//可以通过shms指针来对结构体里面的数据进行读写.
}
