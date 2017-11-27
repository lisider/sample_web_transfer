#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "print_color.h"
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#define FIFO_PATH "/tmp/fifo1"

struct shm *shms;//结构体指针定义 
int shmid;//共享内存id定义

char fifo_path[32];

void sig_handler(int arg){
	
	char rm_fifo_str[40];
	switch(arg){
	
		case SIGINT:


			// 1. 删 fifo 文件
			bzero(rm_fifo_str,sizeof(rm_fifo_str));
			strcpy(rm_fifo_str,"rm -f ");
			strcat(rm_fifo_str,fifo_path);
			system(rm_fifo_str);

			// 2. 取消共享内存的映射
			shmdt(shms);
			exit(23); //这个23需要定义
			
			break;

		default:
			;
	}
	return ;
}


void call_back_1R(void){
	
	printf("i am call_back_1R \n");
	return ;
}

void call_back_2R(void){
	
	printf("i am call_back_2R\n");
	return ;
}

void call_back_3R(void){
	
	printf("i am call_back_3R\n");
	return ;
}

void call_back_4R(void){
	
	printf("i am call_back_4R\n");
	return ;
}

typedef void (*call_back_fun_t)(void);

call_back_fun_t call_back_fun[4] = {call_back_1R,call_back_2R,call_back_3R,call_back_4R};


static int shm_init(void){

	key_t key;//key定义

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


typedef enum {
	READ_INIT,
	READ_1R,
	READ_2R,
	READ_3R,
	READ_4R,
} read_state_t;

read_state_t read_state =  READ_INIT;


void fun_main(const char *path){
	int ret = 0;
	int count = 1;
	int fifo_fd = -1;
	char FIFO_PATH[32];

	char times_max[4] = {3,3,3,3};
	char times_cur[4] = {0};

	char buf_verify[4][32];
	

	//select 相关
	int retval; 
	fd_set  readfds;  
	struct  timeval tv;  


	char buf[32];	// 用来接收fifo 的 信息
	bzero(buf,sizeof(buf));
	bzero(FIFO_PATH,sizeof(FIFO_PATH));
	bzero(buf_verify,sizeof(buf_verify));

	strcpy(FIFO_PATH,path);
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




	//while(1) 里面处理的是一次完整的收发过程
	while(1){

		//每一次循环的初始化
		times_cur[0] = 0;
		times_cur[1] = 0;
		times_cur[2] = 0;
		times_cur[3] = 0;

		bzero(buf_verify,sizeof(buf_verify));
		//read
		//printf("shm state : %s\n", shms->shm_state == 0 ?("WRITEABLE"):(shms->shm_state == 1?("READABLE"):("NUMBER_OF_MEMBERS")));

#if 0

			sem_wait(&(shms->sem)); //这里会出问题
#else


loop:
			if (sem_trywait((sem_t *)&(shms->sem)) == -1){
				
				perror("sem_trywait");

				goto loop;
			}
#endif

			printf("------------------------------------------------------------------\n");
			//接下来要判断是不是自己的数据  ver2
			printf(YELLOW "del with buff_to_recv form ws_client\n\n" NONE);
			//read buff_to_recv

			//pthread_rwlock_wrlock(&(shms->lock));

			shms->shm_state = NUMBER_OF_MEMBERS;

			//write 

			// write 1 填充 node
			bzero((void *)&(shms->buff_to_send.node),sizeof(node_t));
			strcpy((char *)(shms->buff_to_send.node.context),"i am node context");
			shms->buff_to_send.node.pid = getpid();
			shms->buff_to_send.node.key[0] = count+30;
			shms->buff_to_send.node.key[1] = count+60;
			shms->buff_to_send.node.key[2] = count+90;
			strcpy((char *)(shms->buff_to_send.node.fifo_path),FIFO_PATH);
			printf(GREEN "node :%s\n" NONE,shms->buff_to_send.node.context);


			//填充  msg_info
			bzero((void *)&(shms->buff_to_send.msg_info),sizeof(msg_info_t));
			strcpy((char *)(shms->buff_to_send.msg_info.context),"i am node context");
			shms->buff_to_send.msg_info.key_1R = count;
			shms->buff_to_send.msg_info.pid = getpid();
			strcpy((char *)(shms->buff_to_send.msg_info.fifo_path),FIFO_PATH);
			printf(GREEN "msg_info :%s\n" NONE,shms->buff_to_send.msg_info.context);

			// fill in buff_to_send with data
			shms->shm_state = WRITEABLE;
			printf(GREEN "send buff_to_send form process1 to ws_client\n" NONE);


			//用于验证1.R 和 验证 2.R 3.R 4.R 的变量放的位置不同,所以
			//不能在一块做

			bzero(buf_verify[0],sizeof(buf_verify[0]));
			snprintf(buf_verify[0],sizeof(buf_verify[0]),"%d",shms->buff_to_send.msg_info.key_1R);

		
			{
				int i = 0;

				for(i = 0;i < 3;i++){

					bzero(buf_verify[i+1],sizeof(buf_verify[i+1]));
					snprintf(buf_verify[i+1],sizeof(buf_verify[i+1]),"%d",shms->buff_to_send.node.key[i]);
					
				}
			}

			sem_post((sem_t *)&(shms->sem));


			printf(PURPLE "%dth msg to the ws_client\n" NONE,count);

			//这里验证  与 key_1R 相关的 ,ws_client 得到 key_1R 会
			//发送给我一个值 ,这个值 需要和 key_1R 对比

	//		printf(RED "waiting for ack\n" NONE);

			
			for(read_state = READ_INIT;READ_INIT < READ_4R;read_state++){
			
READ_AGGIN:
				// 读 1.R
				tv.tv_sec  = 1;  
				tv.tv_usec = 0;  

				FD_ZERO(&readfds);//清空集合  
				/*添加监听套接字*/
				FD_SET(fifo_fd,&readfds);//将文件描述符填入集合中
				//void FD_CLR(int fd, fd_set *fdset);   //将一个给定的文件描述符从集合中删除

				retval = select(fifo_fd+1,&readfds,NULL,NULL,&tv); 
				if(retval == 0){  
					printf("wait %d.R Time out!\n",read_state +1);  
					//times_1R_real ++;
					times_cur[read_state]++;
					if(times_cur[read_state] < times_max[read_state])
						goto READ_AGGIN;
					else
					{
						printf(RED "%d.R not receive ,abandon the packet\n" NONE,read_state+1);
						break;//这时,放弃 整个数据的传输,这时,要告诉 发送者,这个包不被传输//TODO
					}
				}else{
					if(FD_ISSET(fifo_fd,&readfds)){//检查集合中指定的文件描述符是否可以读写 

						bzero(buf,sizeof(buf));
						if(read(fifo_fd,buf,sizeof(buf)) <=0){

							printf(RED "%d.R read error,abandon the packet\n" NONE,read_state+1);
							break;//这时,放弃 整个数据的传输,这时,要告诉 发送者,这个包不被传输//TODO
						}
					}  
				}


				//	printf("get ack %s\n",buf);

				//pthread_rwlock_unlock(&(shms->lock));

				if(strcmp(buf_verify[read_state],buf) == 0){
					printf(GREEN "get right ack %d.R\n" NONE,read_state+1);
				}else
				{
					printf(RED "get wrong ack %d.R: %s\n" NONE,read_state+1,buf);
					exit(24);
				}

				//1.R
				//do 1.R

				call_back_fun[read_state]();

			}

			printf("------------------------------------------------------------------\n");
			count ++;
			puts("\n");

	}

	return ;
}

void usage(void){

	printf("./process /tmp/fifox\nthe second param should be different from other process you have forked\n");
	exit(21);
	return ;
}

int main(int argc, const char *argv[]){

	int ret = 0;

	if(argc != 2)
		usage();

	bzero(fifo_path,sizeof(fifo_path));

	strncpy(fifo_path,argv[1],32);

	signal(SIGINT, sig_handler);

	ret = shm_init();
	if(ret)
		printf(RED "shm_init failed\n" NONE);

	fun_main(argv[1]);

	return 0;

	//通过最后一个函数(共享内存的映射),shms指针指向的结构体成为共享内存中的结构体.
	//可以通过shms指针来对结构体里面的数据进行读写.
}
