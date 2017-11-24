#include <stdio.h>
#include <pthread.h>
#include "print_color.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "linklist.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




pthread_t   pthid_write;

struct shm *shms;//结构体指针定义 
int shmid;//共享内存id定义

linked_list_t * list_h_pointer = NULL;


void sig_handler(int arg){
	
	switch(arg){
	
		case SIGINT:

			printf(RED "Recovering memory...");

			delete_linklist_all(list_h_pointer);

			printf("\tDONE\n" NONE);

			//做一些其他的回收的事情

			shmdt(shms);
			shmctl(shmid, IPC_RMID, NULL);
			exit(23); //这个23需要定义
			
			break;

		default:
			;
	}
	return ;
	
}

void thread_main (void){ //这个线程处理 写入 buff_to_recv
	int i;
	char buff_to_recv[1024];

	while(1){

		usleep(1000 * 100);
		// 1. 接收数据

		for(i= 0;i< 11;i++){
			//printf("get buff_to_recv[%d]\tform ws_server to ws_client\n",i);
			buff_to_recv[i] = (char)i;
		}

		printf(L_CYAN "get buff_to_send form ws_server to ws_client\n" NONE);


		sem_wait(&(shms->sem));


		//pthread_rwlock_wrlock(&(shms->lock));
		//fill in the buff_to_recv



		// 2. 设置 可读 标志
		shms->shm_state = READABLE;

		//pthread_rwlock_unlock(&(shms->lock));
		sem_post(&(shms->sem));
	}
	return ;

}


static void * thread_del_list(void *arg){
	int fifo_fd = -1;
	char buf[32];
	signal(SIGINT,sig_handler);

	//等待 LWS_CALLBACK_CLIENT_ESTABLISHED
	while(1){

		usleep(1000*200);

		print_sequence_linkedlist(list_h_pointer);
			
		if(list_h_pointer->next == NULL)
			continue;


		//1.找到第一个,发出去
		// list_h_pointer->next->data.context  //lws_write
		
		
		// 2. 2.R
		printf(GREEN "send the first node\n");

		// 1.打开
		fifo_fd = open(list_h_pointer->next->data.fifo_path,O_RDWR);
		if(0 > fifo_fd){
			perror("open");
		}

		bzero(buf,sizeof(buf));

		snprintf(buf,sizeof(buf),"%d",list_h_pointer->next->data.key[0]); //KEY_0 是 用于 验证 2.R的


		// 2. 写
		printf(RED "send ack 2.R :%s to pid :%d",buf,list_h_pointer->next->data.pid);
		write(fifo_fd,buf,sizeof(buf));
		printf("   ...send ack 2.R DONE\n" NONE);

		// 3.关闭
		close(fifo_fd);

		// 3. 得到ack

		// 4. 3.R
		// 5. 得到消息
		// 6. 4.R
		
		del_front_linkedlist(list_h_pointer,NULL);

		printf(GREEN "send buff_to_send form linklist to ws_server\n" NONE);
		puts("\n");
	}

	//如果为空,什么都不做
	//
	//如果不为空,做事情
	//
	//发送数据
	//失败,重发
	//失败几次,做某事情
	//
	//成功 ,删节点

	return NULL;
}



static void * thread_insert(void *arg){
	int fifo_fd = -1;
	char buf[32];
	bzero(buf,sizeof(buf));

	while(1){
		if(shms->shm_state == WRITEABLE){
			//数据的互斥
			sem_wait(&(shms->sem));

			//		printf(BLUE "msginfo :%s\n" NONE,shms->buff_to_send.msg_info.context);
			//		printf(BLUE "node :%s\n" NONE,shms->buff_to_send.node.context);


			//pthread_rwlock_wrlock(&(shms->lock));
			// 1. 插入 前的判断
			//对 msginfo 中的数据进行分析
			//
			//将 node  插入 链表 根据 msginfo 的内容进行怎么插入

			// 2. 插入
			//insert the buff_to_send to linklist
			add_rear_linkedlist(list_h_pointer,&(shms->buff_to_send.node),sizeof(node_t));


			// 3. 插入后的事情
			// 3.1  1.R
			// 3.2  回调
			//从 msg_info 中找到跟进程相关的东西 ,暂定 pid ,然后发送给该 pid 一个回应,表示已经接收到
			//该pid 已经对该回应进行 判断, 如果是正确的回应,则确定 已经发送到 ws_client
			//插入成功,做回调

			// shms->buff_to_send.msg_info.pid

			//回应必须只发给他 ,因为 我已经接到了,所以 最好用个保险的方式
			//通知他, 让 客户端的程序最简化


			printf(YELLOW "recv msg form pid : %d" NONE,shms->buff_to_send.msg_info.pid);


			//printf("%s\n",shms->buff_to_send.msg_info.fifo_path);


			// 1.打开
			fifo_fd = open(shms->buff_to_send.msg_info.fifo_path,O_RDWR);
			if(0 > fifo_fd){
				perror("open");
			}

			snprintf(buf,sizeof(buf),"%d",shms->buff_to_send.msg_info.key_1R);


			// 2. 写
			printf(RED "send ack 1.R:%s",buf);
			write(fifo_fd,buf,sizeof(buf));
			printf("   ...send ack 1.R DONE\n" NONE);

			// 3.关闭
			close(fifo_fd);


			bzero(buf,sizeof(buf));

			printf(GREEN "insert buff_to_send into linklist\n" NONE);
			shms->shm_state = NUMBER_OF_MEMBERS;
			//pthread_rwlock_unlock(&(shms->lock));

			sem_post(&(shms->sem));
		}
	}

	return NULL;
}


static int shm_init(void){

	key_t key;//key定义

	 int access(const char *filename, int mode);
	 if(access(SHM_PATH,F_OK) != 0)

	 {
		 printf("mkdir SHM_PATH\n");
		 system("mkdir " SHM_PATH);
	 }
		 



	key = ftok(SHM_PATH,'r');//获取key
	if(-1 == key){
		perror("ftok");
		printf(YELLOW "you can try to solve it by mkdir %s\n" NONE,SHM_PATH);
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

#if 0

static int
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
		                        void *user, void *in, size_t len)
{

	//到达 读状态
	
//	1. 查询 是 4.R 还是 3.R

	//如果是 4.R 是谁的 4.R
	//发送给 它
	
	//如果是 3.R 是谁的 3.R
	//发送给 它
	


	return 0;
}
#endif

int main(int argc, const char *argv[])
{
	int ret = 0;
	ret = shm_init();
	if(ret)
		printf(RED "shm_init failed\n" NONE);

	list_h_pointer = create_linkedlist();

	if(0 != pthread_create(&pthid_write,NULL,thread_insert,NULL)){
		perror("pthid1");
		return -1;
	}

	if(0 != pthread_create(&pthid_write,NULL,thread_del_list,NULL)){
		perror("pthid1");
		return -1;
	}

	thread_main();
	return 0;
}
