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
#include "list.h"

#define CLIENT_DEADLINE  7

typedef struct  _list_xxx_t{
    msg_send_t data;
    struct list_head list;
}list_xxx_t;

list_xxx_t list_xxx_head; //用于存放没有发送的节点

/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/


/**************************************************************************************
 * 函数定义区
 **************************************************************************************/

void call_back_1R(int count,char state){

    printf(YELLOW"call_back_1R  count :%d, state:%d\n"NONE,count,state);
    return ;
}

void call_back_2R(int count,char state){

    printf(YELLOW"call_back_2R  count :%d, state:%d\n"NONE,count,state);
    return ;
}

void call_back_3R(int count,char state){

    printf(YELLOW"call_back_3R  count :%d, state:%d\n"NONE,count,state);
    return ;
}

void call_back_4R(int count,char state){

    printf(YELLOW"call_back_4R  count :%d, state:%d\n"NONE,count,state);
    return ;
}

void call_back_AR(int count,char state){

    printf(YELLOW"call_back_AR  count :%d, state:%d\n"NONE,count,state);
    return ;
}


void usage(void){

    printf("./process /tmp/fifox\nthe second param should be different from other process you have forked\n");
    exit(21);
    return ;
}


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
    list_xxx_t *tmp_xxx_node;
    struct list_head *pos,*n;
    int found = 0;
    int i;
    char buf_verify[32];
    switch(arg){

        case SIGINT: //退出信号
            //处理fifo 和共享内存
            lib_exit_sig(SIGINT);
            //处理链表
            printf("clean...\tremove");
            list_for_each_safe(pos,n,&list_xxx_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                if(1){//对链表中的数据进行判断,如果满足条件就删节点
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    free(tmp_xxx_node);//释放数据
                }
            }
            printf("done\n");
            //退出
            exit(0);
            break;
        case SIGUSR1: //中转进程给他的信号

            //读消息
            if(read(fifo_fd,buf,sizeof(buf))<=0){ // 得到的是 第几次返回 哪一次返回 返回的状态
                printf(RED "%d.R read head error,abandon the packet\n" NONE,*buf+1);
                print_error(*buf + 4);
            }
            printf(GREEN "receive a msg,the count encryption:%s,return type:%d,return state::%d\n"NONE,buf+1,buf[0],buf[sizeof(buf)-1]);


            // TODO 添加 RA 时候 的处理,不知道怎么验证 原信息是否存在.有时候不需要验证信息存在.
            //验证原始信息是否存在
            list_for_each_safe(pos,n,&list_xxx_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                bzero(buf_verify,sizeof(buf_verify));
                snprintf(buf_verify,sizeof(buf_verify),"%d",tmp_xxx_node->data.node.key[buf[0]]);
                //printf("%s\n",buf_verify);
                //printf("%s\n",buf+1);

                if(strcmp(buf_verify,buf+1) == 0){//存在 则 做相应的动作
                    printf(GREEN"going to deal with the msg\n"NONE);

                    if(call_back_fun[buf[0]] == NULL)
                        printf(RED "Unable to parse information whose msg type is %d\n"NONE,buf[0]);
                    else
                        call_back_fun[buf[0]](tmp_xxx_node->data.msg_info.count,buf[sizeof(buf)-1]);//这里需要处理两个问题,一个是回复的状态,一个是第几条信息的回复
               
                    //得到状态信息
                    //这里面应该根据不同的消息类型 调用不同的函数,注册的函数应该还是少了些.
                    //这里面最好 是 多定义好几种状态,并 发信号给 某个线程来做事情. 因为在信号处理函数里面最好不要做太多事情
                    //然后做相应的函数
                    return ;
                }
            }
            //不存在则 打印信息,什么都不做
            printf(RED"Original msg has been deleted\n"NONE);
            break;

        case SIGALRM: //退出信号
            //定时删链表
            printf(GREEN "i am going to Traversing chain list\n" NONE);
            list_for_each_safe(pos,n,&list_xxx_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                printf(YELLOW "del with one subtraction , left dead_line :%d\n" NONE,tmp_xxx_node->data.msg_info.dead_line-1);
                if(--tmp_xxx_node->data.msg_info.dead_line == 0){//对链表中的数据进行判断,如果满足条件就删节点
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    free(tmp_xxx_node);//释放数据
                    printf(YELLOW"remove node from list_xxx_head because of dead_line, count is %d\n"NONE,tmp_xxx_node->data.msg_info.count);
                }
            }

            //循环链表,并删除剩余=0 的
            alarm(1);
            break;

        default:
            ;
    }
    return ;
}

extern int shm_init(void);


int main(int argc, const char *argv[]){

    list_xxx_t *tmp_xxx_node;
    struct list_head *pos,*n;
    int ret = 0;
    int count = 0;
    int i;
    call_back_fun_t call_back_fun[5] = {call_back_1R,call_back_2R,call_back_3R,call_back_4R,call_back_AR};

    if(argc != 2)
        usage();

    //1. 初始化部分
    /**************************************************/

    signal(SIGINT, sig_handler);
    signal(SIGUSR1,sig_handler);
    signal(SIGALRM, sig_handler);

    ret = shm_init();
    if(ret)
        printf(RED "shm_init failed\n" NONE);

    call_back_register(call_back_fun,5);

    creat_fifo(argv[1]);

    INIT_LIST_HEAD(&list_xxx_head.list);  

    /**************************************************/

    // 2. 动态申请内存
    tmp_xxx_node = (list_xxx_t *)malloc(sizeof(list_xxx_t));
    if(tmp_xxx_node == NULL){
        perror("tmp_xxx_node");
        exit(23);
    }

    alarm(1);
    while(1){

        count ++;

        // 3. 填充 tmp_xxx_node->data //具体填充什么看结构体定义
        // write 1 填充 node                                                                    
        //


        bzero((void *)&(tmp_xxx_node->data.node),sizeof(node_t));                               
        //strcpy((char *)(tmp_xxx_node->data.node.context),"i am node context");                  
        //tmp_xxx_node->data.node.pid = getpid();                                                 
        tmp_xxx_node->data.node.key[0] = count+30;                                              
        tmp_xxx_node->data.node.key[1] = count+60;                                              
        tmp_xxx_node->data.node.key[2] = count+90;                                              
        tmp_xxx_node->data.node.key[3] = count+120;                                              
        //tmp_xxx_node->data.msg_info.dead_line = CLIENT_DEADLINE;
        //strcpy((char *)(tmp_xxx_node->data.node.fifo_path),argv[1]);
        //printf(GREEN "node :%s\n" NONE,tmp_xxx_node->data.node.context);                        

        //填充  msg_info                                                                        
        bzero((void *)&(tmp_xxx_node->data.msg_info),sizeof(msg_info_t));                       
        strcpy((char *)(tmp_xxx_node->data.msg_info.context),"i am node context");
        tmp_xxx_node->data.msg_info.dead_line = CLIENT_DEADLINE;
        tmp_xxx_node->data.msg_info.process_type = BLUETOOTH ;
        tmp_xxx_node->data.msg_info.count = count;
        //tmp_xxx_node->data.msg_info.key_1R = count;                                             
        tmp_xxx_node->data.msg_info.pid = getpid();                                             
        strcpy((char *)(tmp_xxx_node->data.msg_info.fifo_path),argv[1]);                      
        //printf(GREEN "msg_info :%s\n" NONE,tmp_xxx_node->data.msg_info.context);                



        // 4.发送及接收,接收 在回调中 
        ret = pkt_send(&(tmp_xxx_node->data),sizeof(tmp_xxx_node->data),count);
        if(ret < 0){
            printf(RED "send error\n" NONE);
        }


        list_add_tail(&(tmp_xxx_node->list),&list_xxx_head.list);

        //sleep(1);
        while(1);
    }
    return 0;
}
