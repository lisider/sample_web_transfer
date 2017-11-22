#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "print_color.h"
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO_PATH "/tmp/fifo2"


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
	int ret = 0;
	int count = 0;
	int fifo_fd = -1;

	//select 相关
	int retval; 
	fd_set  readfds;  
	struct  timeval tv;  

	char buf_verify[32]; //用来存储将要和接收信息进行对比的 验证 字符串

	char buf[32];	// 用来接收fifo 的 信息
	bzero(buf,sizeof(buf));
	bzero(buf_verify,sizeof(buf_verify));
	if((access(FIFO_PATH,F_OK))!=-1)  //存在
	{
		strcpy(buf,"rm -f ");
		strcat(buf,FIFO_PATH);
		system(buf);
	}

	bzero(buf,sizeof(buf));

	ret = mkfifo(FIFO_PATH,IPC_CREAT|IPC_EXCL|0666);
	if(-1 == ret){
		perror("mkfifo");
	}

	fifo_fd = open(FIFO_PATH,O_RDWR);
	if(0 > fifo_fd){
		perror("open");
	}




	while(1){
		//read
		//printf("shm state : %s\n", shms->shm_state == 0 ?("WRITEABLE"):(shms->shm_state == 1?("READABLE"):("NUMBER_OF_MEMBERS")));
		if(shms->shm_state == READABLE){


#if 0
			sem_wait(&(shms->sem));
	
#else
loop:
			if (sem_trywait(&(shms->sem)) == -1){
				
				perror("sem_trywait");

				goto loop;
			}
#endif



			//接下来要判断是不是自己的数据  ver2
			printf(YELLOW "del with buff_to_recv form ws_client\n" NONE);
			//read buff_to_recv

			//pthread_rwlock_wrlock(&(shms->lock));

			shms->shm_state = NUMBER_OF_MEMBERS;

			//write 

			// write 1 填充 node
			bzero((void *)&(shms->buff_to_send.node),sizeof(node_t));
			strcpy(shms->buff_to_send.node.context,"i am node context");
			printf(GREEN "node :%s\n" NONE,shms->buff_to_send.node.context);


			//填充  msg_info
			bzero((void *)&(shms->buff_to_send.msg_info),sizeof(msg_info_t));
			strcpy(shms->buff_to_send.msg_info.context,"i am node context");
			shms->buff_to_send.msg_info.msg_key = count;
			shms->buff_to_send.msg_info.pid = getpid();
			strcpy(shms->buff_to_send.msg_info.fifo_path,FIFO_PATH);
			printf(GREEN "msg_info :%s\n" NONE,shms->buff_to_send.msg_info.context);

			// fill in buff_to_send with data
			shms->shm_state = WRITEABLE;
			printf(GREEN "send buff_to_send form process1 to ws_client\n" NONE);


			snprintf(buf_verify,sizeof(buf_verify),"%d",shms->buff_to_send.msg_info.msg_key);

			sem_post(&(shms->sem));

			count ++;

			printf(PURPLE "%d msgs send to the ws_client\n" NONE,count);

			//这里验证  与 msg_key 相关的 ,ws_client 得到 msg_key 会
			//发送给我一个值 ,这个值 需要和 msg_key 对比

			printf(RED "waiting for ack\n" NONE);

			// 设置最长等待时间  
			tv.tv_sec  = 1;  
			tv.tv_usec = 0;  

			FD_ZERO(&readfds);//清空集合  
			/*添加监听套接字*/
			FD_SET(fifo_fd,&readfds);//将文件描述符填入集合中
			//void FD_CLR(int fd, fd_set *fdset);   //将一个给定的文件描述符从集合中删除

			retval = select(fifo_fd+1,&readfds,NULL,NULL,&tv); 
			if(retval == 0){  
				printf("Time out!\n");  
				continue;
			}else{
				if(FD_ISSET(fifo_fd,&readfds)){//检查集合中指定的文件描述符是否可以读写 
					if(read(fifo_fd,buf,sizeof(buf)) <=0){
						continue;
					}
				}  
			}



			//	printf("get ack %s\n",buf);



			//pthread_rwlock_unlock(&(shms->lock));

			if(strcmp(buf_verify,buf) == 0){
				printf(GREEN "get right ack 1.R\n" NONE);
			}else
			{
				printf(RED "get wrong ack 1.R: %s\n" NONE,buf);
				exit(24);
			}



			bzero(buf,sizeof(buf));

			puts("\n");
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
