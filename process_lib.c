/**************************************************************************************
 * 头文件区
 **************************************************************************************/
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
#include "process_lib.h"
#include "read_write_state_api.h"


/**************************************************************************************
 * 结构体/枚举体声明区
 **************************************************************************************/

typedef enum {
	READ_INIT,
	READ_1R,
	READ_2R,
	READ_3R,
	READ_4R,
} read_state_t;


/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/

int fifo_fd;
char fifo_path[32];
int shmid;//共享内存id定义
struct shm *shms;//结构体指针定义 
call_back_fun_t call_back_fun[4];

read_state_t read_state =  READ_INIT;



/**************************************************************************************
 * 函数定义区
 **************************************************************************************/


void lib_exit_sig(int arg){
	
	char rm_fifo_str[40];
	switch(arg){
	
		case SIGINT:
			// 1. 删 fifo 文件
			printf("lib sig_handler\n");
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


void call_back_register(call_back_fun_t *p,int count){
	int i = 0;
	for(i = 0;i < count;i++)
		call_back_fun[i] = p[i];
	return ;
}


int shm_init(void){

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


void creat_fifo(const char *path){

	char FIFO_PATH[32];
	char buf[32];	// 用来接收fifo 的 信息
	int ret = 0;

	bzero(fifo_path,sizeof(fifo_path));

	strncpy(fifo_path,path,32);

	bzero(FIFO_PATH,sizeof(FIFO_PATH));
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

}

#if 0
int pkt_service(msg_send_t * send_pkt_p,int size,int count){

	int ret = 0;
	char times_max[4] = {3,3,3,3};
	char times_cur[4] = {0};
	char buf_verify[4][32];

	//select 相关
	int retval; 
	fd_set  readfds;  
	struct  timeval tv;  


	char buf[32];	// 用来接收fifo 的 信息
	bzero(buf,sizeof(buf));
	bzero(buf_verify,sizeof(buf_verify));


	bzero(buf_verify,sizeof(buf_verify));
	//printf("shm state : %s\n", shms->shm_state == 0 ?("WRITEABLE"):(shms->shm_state == 1?("READABLE"):("NUMBER_OF_MEMBERS")));

#if 0
	sem_wait((sem_t *)&(shms->sem)); //这里会出问题
#else

loop:
	if (sem_trywait((sem_t *)&(shms->sem)) == -1){

		perror("sem_trywait");

		goto loop;
	}
#endif

	printf("------------------------------------------------------------------\n");

	//pthread_rwlock_wrlock(&(shms->lock));

	printf(GREEN "send buff_to_send->form process to ws_client\n" NONE); 
	memcpy((char *)&(shms->buff_to_send),send_pkt_p,sizeof(msg_send_t));

	shms->shm_state = WRITEABLE;                                                            

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
				return read_state + 1;
			}
		}else{
			if(FD_ISSET(fifo_fd,&readfds)){//检查集合中指定的文件描述符是否可以读写 

				bzero(buf,sizeof(buf));
				if(read(fifo_fd,buf,sizeof(buf)) <=0){

					printf(RED "%d.R read error,abandon the packet\n" NONE,read_state+1);
					return read_state + 4;
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
			return read_state + 8;
		}

		//1.R
		//do 1.R

		call_back_fun[read_state]();

	}

	printf("------------------------------------------------------------------\n");
	puts("\n");
	return  0;
}
#endif


extern char buf_verify[4][32];

int pkt_send(msg_send_t * send_pkt_p,int size,int count){

	int ret = 0;
        int i;
	bzero(buf_verify,sizeof(buf_verify));

	//printf("shm state : %s\n", shms->shm_state == 0 ?("WRITEABLE"):(shms->shm_state == 1?("READABLE"):("NUMBER_OF_MEMBERS")));

loop:
	if (sem_trywait((sem_t *)&(shms->sem)) == -1){

		perror("sem_trywait");

		goto loop;
	}
	printf("------------------------------------------------------------------\n");

	//pthread_rwlock_wrlock(&(shms->lock));

	printf(GREEN "send buff_to_send->form process to ws_client\n" NONE); 
	memcpy((char *)&(shms->buff_to_send),send_pkt_p,sizeof(msg_send_t));

        while(!is_writeable_send(shms->read_write_state)){
            shms->unwriteable_times_send += 1;
            
            if(shms->unwriteable_times_send > 5)
            {
                shms->unwriteable_times_send = 0;
                break;
            }

            sleep(1);

        }
	//shms->shm_state = WRITEABLE;                                                            
        disable_writeable_send(&(shms->read_write_state));
	//用于验证1.R 和 验证 2.R 3.R 4.R 的变量放的位置不同,所以
	//不能在一块做



        for(i = 0;i < 3;i++){

            bzero(buf_verify[i],sizeof(buf_verify[i]));
            snprintf(buf_verify[i],sizeof(buf_verify[i]),"%d",shms->buff_to_send.node.key[i]);

        }


        sem_post((sem_t *)&(shms->sem));
        alarm(5);

        printf(PURPLE "%dth msg to the ws_client\n" NONE,count);

        return  0;
}

