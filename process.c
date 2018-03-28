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

/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/

msg_send_t     *buff_to_send = NULL;

/**************************************************************************************
 * 函数定义区
 **************************************************************************************/

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


void usage(void){

    printf("./process /tmp/fifox\nthe second param should be different from other process you have forked\n");
    exit(21);
    return ;
}

char buf_verify[4][32];

extern int fifo_fd;

extern call_back_fun_t call_back_fun[4];



void print_error(int errnum){

    switch(errnum){
        case 1:
            printf(RED "1R_NOT_RECEVE\n" NONE);
            break;
        case 2:
            printf(RED "2R_NOT_RECEVE\n" NONE);
            break;

        case 3:
            printf(RED "3R_NOT_RECEVE\n" NONE);
            break;

        case 4:
            printf(RED "4R_NOT_RECEVE\n" NONE);
            break;

        case 5:
            printf(RED "1R_READ_ERROR\n" NONE);
            break;

        case 6:
            printf(RED "2R_READ_ERROR\n" NONE);
            break;

        case 7:
            printf(RED "3R_READ_ERROR\n" NONE);
            break;

        case 8:
            printf(RED "4R_READ_ERROR\n" NONE);
            break;

        case 9:
            printf(RED "1R_WRONG_ACK\n" NONE);
            break;

        case 10:
            printf(RED "2R_WRONG_ACK\n" NONE);
            break;

        case 11:
            printf(RED "3R_WRONG_ACK\n" NONE);
            break;

        case 12:
            printf(RED "4R_WRONG_ACK\n" NONE);
            break;

        default:
            break;
    }
}



static void sig_handler(int arg){

    char rm_fifo_str[40];
    char buf[32];	// 用来接收fifo 的 信息
    bzero(buf,sizeof(buf));
    msg_type_t state;
    switch(arg){

        case SIGINT: //退出信号
            lib_exit_sig(SIGINT);
            free(buff_to_send);
            exit(0);

            break;
        case SIGUSR1: //中转进程给他的信号


            if(read(fifo_fd,buf,1)<=0){
                printf(RED "%d.R read head error,abandon the packet\n" NONE,*buf+1);
                print_error(*buf + 4);
            }

            //得到是 发送的第几次返回
            state = buf[0];

            //state 对 state 进行判断,并做相应的动作

            printf("verify %d\n",state);
            //重新初始化buf
            buf[0] = 0;


            //得到是 哪一次的发送的返回
            if(read(fifo_fd,buf,sizeof(buf)-1) <=0){
                printf(RED "%d.R read body error,abandon the packet\n" NONE,state+1);
                print_error(state + 4);
            }


            // 根据 *buf 进行校验

            //printf("Receive %d\n",*(int *)(buf+1));
     
            //判断状态,然后进行比较.
            if(strcmp(buf_verify[state],buf) == 0){
                printf(GREEN "get right ack %d.R\n" NONE,state+1);
            }else
            {
                printf(RED "get wrong ack %d.R: %s\n" NONE,state+1,buf+1);
                print_error(state + 8);
            }

            //得到状态信息
            printf("ack_state : %d\n",buf[sizeof(buf)-2]);


            //然后做相应的函数
            call_back_fun[state]();

            break;

        case SIGALRM: //退出信号
            //定时删链表


            break;

        default:
            ;
    }
    return ;
}

extern int shm_init(void);


int main(int argc, const char *argv[]){

    int ret = 0;
    int count = 0;
    call_back_fun_t call_back_fun[4] = {call_back_1R,call_back_2R,call_back_3R,call_back_4R};

    if(argc != 2)
        usage();

    //1. 初始化部分
    /**************************************************/

    signal(SIGINT, sig_handler);
    signal(SIGUSR1,sig_handler);

    ret = shm_init();
    if(ret)
        printf(RED "shm_init failed\n" NONE);

    call_back_register(call_back_fun,4);

    creat_fifo(argv[1]);

    /**************************************************/

    // 2. 动态申请内存
    buff_to_send = (msg_send_t *)malloc(sizeof(msg_send_t));
    if(buff_to_send == NULL){
        perror("buff_to_send");
        exit(23);
    }

    while(1){

        count ++;

        // 3. 填充 buff_to_send //具体填充什么看结构体定义
        // write 1 填充 node                                                                    
        bzero((void *)&(buff_to_send->node),sizeof(node_t));                               
        //strcpy((char *)(buff_to_send->node.context),"i am node context");                  
        //buff_to_send->node.pid = getpid();                                                 
        buff_to_send->node.key[0] = count+30;                                              
        buff_to_send->node.key[1] = count+60;                                              
        buff_to_send->node.key[2] = count+90;                                              
        //strcpy((char *)(buff_to_send->node.fifo_path),argv[1]);
        //printf(GREEN "node :%s\n" NONE,buff_to_send->node.context);                        

        //填充  msg_info                                                                        
        bzero((void *)&(buff_to_send->msg_info),sizeof(msg_info_t));                       
        strcpy((char *)(buff_to_send->msg_info.context),"i am node context");
        //buff_to_send->msg_info.key_1R = count;                                             
        buff_to_send->msg_info.pid = getpid();                                             
        strcpy((char *)(buff_to_send->msg_info.fifo_path),argv[1]);                      
        //printf(GREEN "msg_info :%s\n" NONE,buff_to_send->msg_info.context);                

        // 4.发送及接收,接收 在回调中 
        pkt_send(buff_to_send,sizeof(buff_to_send),count);

        //sleep(100);
        while(1);
    }
    return 0;
}
