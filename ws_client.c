/**************************************************************************************
 * 头文件区
 **************************************************************************************/
#include <stdio.h>
#include <pthread.h>
#include "print_color.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "common.h"
#include "linklist.h"
#include "list.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lws_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <cJSON.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#include <base64.h> 
#include <libwebsockets.h> //必须 找到这个文件
#include "read_write_state_api.h"


/**************************************************************************************
 * 宏定义区
 **************************************************************************************/

#define SERVER_DEADLINE 5
#define LWS_DEFINE 0

/**************************************************************************************
 * 结构体/枚举体声明区
 **************************************************************************************/

enum demo_protocols
{

    PROTOCOL_DUMB_INCREMENT,
    PROTOCOL_LWS_MIRROR,

    /* always last */
    DEMO_PROTOCOL_COUNT
};

struct pthread_routine_tool
{
    struct lws_context *context;
    struct lws *wsi;
};

typedef struct  _list_xxx_t{
    msg_send_t data;
    struct list_head list;
}list_xxx_t;

/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/

static int deny_deflate, longlived, mirror_lifetime, test_post;
static struct lws *wsi_dumb, *wsi_mirror;
static struct lws *wsi_multi[3];
static volatile int force_exit;
static unsigned int opts, rl_multi[3];
static int flag_no_mirror_traffic, justmirror;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
char crl_path[1024] = "";
#endif

static int connection_flag = 0;
static int packet_come_flag = 0;
static char  WatchSID[40] = "0";//WatchEID
static char  WatchFamiGID[40] = "0";
static char  WatchEID[40] = "0";
static char  GMT[40] = "0";

static int cid_send = 0;

char * cjson_to_send = NULL;

enum lws_write_protocol pkt_send_format;

pthread_cond_t  cond_main,cond_write;
pthread_mutex_t mutex;

sem_t       sem_main;
sem_t       sem_write;
char buf_receve[320];

pthread_t   pthid_del_linklist;
pthread_t   pthid_insert;

struct shm *shms;//结构体指针定义 
int shmid;//共享内存id定义

list_xxx_t list_xxx_head;
list_xxx_t list_xxx_head2;

/**************************************************************************************
 * 函数定义区
 **************************************************************************************/

char * processtype(process_type_t process_type){

    switch(process_type){
        case BLUETOOTH:
            return "bluetooth";
            break;
        default:
            break;
    }
}


int file_size(const char * filename)  
{  
    FILE *fp=fopen(filename,"r");  
    if(!fp) return -1;  
    fseek(fp,0L,SEEK_END);  
    int size=ftell(fp);  
    fclose(fp);  

    return size;  
}  

static void generate_login_cjson(char **p)
{
    cJSON *root,*fmt;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"Version","00030000");
    cJSON_AddNumberToObject(root, "SN", 142);
    cJSON_AddNumberToObject(root, "CID", 10211);
    cJSON_AddItemToObject(root, "PL", fmt=cJSON_CreateObject());
    cJSON_AddStringToObject(fmt,"Name", "865843024562586");
    cJSON_AddStringToObject(fmt, "Password", "5F64E333CEDB15AD7182D18FC070F8DB");
    cJSON_AddNumberToObject(fmt, "Type", 200);
    cJSON_AddStringToObject(fmt, "machSerialNo", "562586");
    *p = cJSON_Print(root);
    cJSON_Delete(root);
    return ;
}

static void show_http_content(const char *p, size_t l)
{
    if (lwsl_visible(LLL_INFO))
    {
        while (l--)
            if (*p < 0x7f)
                putchar(*p++);
            else
                putchar('.');
    }
}

//目前测试 写 LWS_WRITE_TEXT 成功 ,不知道 写 LWS_WRITE_BINARY 是否可以成功
static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in,enum lws_write_protocol pkt_send_format)
{

    if (str == NULL || wsi_in == NULL)
        return -1;

    int n;
    int len;
    unsigned char *out = NULL;

    if (str_size_in < 1)
        len = strlen(str);
    else
        len = str_size_in;

    out = (unsigned char *)malloc(sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    //* setup the buffer*/
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
    //* write out*/
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, pkt_send_format);

    //  printf("[websocket_write_back] %s\n", str);
    //* free the buffer*/
    free(out);

    return n;
}

void xun_uint_to_char(unsigned int a ,unsigned char *tab,int len)
{
    int  i;
    char*  pa;
    pa=(char *)&a;  
    for(i=0; i < len; i++)
    {
        tab[len-i-1]=*((char *)pa+i);  
    }
}

void sig_handler(int arg){
    int fifo_fd;
    list_xxx_t *tmp_xxx_node;
    struct list_head *pos,*n;
    char buf[32];
    ack_state_t ack_state = INIT_STATE;

    switch(arg){
        case SIGINT:

            printf(YELLOW "Recovering memory...");
            list_for_each_safe(pos,n,&list_xxx_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                if(1){//对链表中的数据进行判断,如果满足条件就删节点
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    free(tmp_xxx_node);//释放数据
                }
            }
            printf("\tDONE\n" NONE);
            //做一些其他的回收的事情
            force_exit = 1;

            shmdt(shms);
            shmctl(shmid, IPC_RMID, NULL);
            exit(23); //这个23需要定义
            break;
        case SIGALRM:

            printf(GREEN "i am going to Traversing chain list\n" NONE);
            list_for_each_safe(pos,n,&list_xxx_head2.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                printf(GREEN "del sub form process : %s,pid : %d,left %d\n" NONE,\
                        processtype(tmp_xxx_node->data.msg_info.process_type),\
                        tmp_xxx_node->data.msg_info.pid,\
                        tmp_xxx_node->data.msg_info.dead_line-1);

                if(--tmp_xxx_node->data.msg_info.dead_line == 0){//对链表中的数据进行判断,如果满足条件就删节点
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系

                    fifo_fd = open(tmp_xxx_node->data.msg_info.fifo_path,O_RDWR);
                    if(0 > fifo_fd){
                        perror("open");
                    }

                    bzero(buf,sizeof(buf));
                    buf[0] = RECV_ACTIVE;
                    snprintf(buf+1,sizeof(buf)-1,"%d",tmp_xxx_node->data.msg_info.count); //KEY_0 是 用于 验证 2.R的
                    ack_state = NORESPONSE;
                    buf[sizeof(buf)-1] = ack_state;
                    printf(YELLOW "send ack  AR  to pid :%d because of dead_line,count :%d",tmp_xxx_node->data.msg_info.pid,tmp_xxx_node->data.msg_info.count);

                    // 2. 写
                    write(fifo_fd,buf,sizeof(buf));
                    kill(tmp_xxx_node->data.msg_info.pid,SIGUSR1);
                    printf("   ...send ack AR DONE\n" NONE,buf[0]);
                    close(fifo_fd);

                    free(tmp_xxx_node);//释放数据
                    printf(YELLOW"remove node from list_xxx_head2 because of dead_line,from %s,count is %d\n"NONE,\
                            processtype(tmp_xxx_node->data.msg_info.process_type),\
                            tmp_xxx_node->data.msg_info.count);
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

#if 0

    static int
callback_lws_mirror(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len)
{
    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    return 0;
}

    static int
callback_test_raw_client(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len)
{
    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    return 0;
}

#endif

/*
 * dumb_increment protocol
 *
 * since this also happens to be protocols[0], some callbacks that are not
 * bound to a specific protocol also turn up here.
 */


    static int
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len)
{
    printf("sws : %s,%s,line = %d-----------------------callback_dumb_increment------------------one callback\n",__FILE__,__func__,__LINE__);

    cJSON *json;
    cJSON * item;
    char *out;
    int array_count = 0;
    cJSON * array;
    cJSON * item2;
    cJSON * item3;
    const char *which = "http";
    char which_wsi[10], buf[50 + LWS_PRE];
    int n;
    int i = 0;

    switch (reason)
    {

        case LWS_CALLBACK_CLIENT_ESTABLISHED://---------------第三次回调的状态

            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_ESTABLISHED \n",__FILE__,__func__,__LINE__);
            //flag_established = 1;

            // connection_flag = 1;
            // generate_cjson(&cjson_to_send);
            //websocket_write_back(wsi,cjson_to_send,-1);


            connection_flag = 1;
            //发送信号给 write 线程
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            // pthread_cond_signal(&cond_write);
            //printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            //获得锁
            //阻塞
            //     pthread_cond_wait(&cond_main,&mutex);
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            //得到信号,解锁

            // generate_cjson(&cjson_to_send);
            //websocket_write_back(wsi,cjson_to_send,-1);



            //while(1);
            //        websocket_write_back(wsi,p,size);
            lwsl_info("dumb: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
            break;

        case LWS_CALLBACK_CLOSED:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLOSED \n",__FILE__,__func__,__LINE__);
            lwsl_notice("dumb: LWS_CALLBACK_CLOSED\n");
            connection_flag = 0;
            wsi_dumb = NULL;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:

            // 3. 得到ack

            // 4. 3.R
            // 5. 得到消息
            // 6. 4.R

            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_RECEIVE \n",__FILE__,__func__,__LINE__);
            ((char *)in)[len] = '\0';
            lwsl_info("rx :%d  .%s.\n", (int)len, (char *)in);

            if(pkt_send_format == LWS_WRITE_TEXT){
                strcat(buf_receve,(char *)in);
                if((int)len < 512)
                {

                    printf("one time recvive all :\n%s\n",buf_receve);
                }

                json = cJSON_Parse(buf_receve);
                if (!json) {
                    printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                }


                out=cJSON_Print(json);

                printf("%s\n",out);

                //验证是否是正常的包
                item=cJSON_GetObjectItem(json,"RC"); 
                if(item->valueint == 1){

                    item=cJSON_GetObjectItem(json,"CID"); 

                    if(item->valueint != cid_send + 1){
                        printf("recv a packet ,but wrong CID ,send CID is  %d,rece CID is %d",cid_send,item->valueint);
                        exit(24);
                    }
                }else
                {
                    printf("rece a packet ,but wrong RC :%d",item->valueint);
                    exit(32);
                }

                //从包中获取 数据 WatchEID WatchSID WatchFamiGID


                bzero(WatchSID,sizeof(WatchSID));
                bzero(WatchEID,sizeof(WatchEID));
                bzero(WatchFamiGID,sizeof(WatchFamiGID));
                bzero(GMT,sizeof(GMT));


                item=cJSON_GetObjectItem(json,"SID");

                memcpy(WatchSID,item->valuestring,32);

                printf("WatchSID : %s\n",WatchSID);


                item=cJSON_GetObjectItem(json,"PL");

                item2=cJSON_GetObjectItem(item,"EID");

                memcpy(WatchEID,item2->valuestring,32);

                printf("WatchEID : %s\n",WatchEID);



                item2=cJSON_GetObjectItem(item,"GMT");

                memcpy(GMT,item2->valuestring,17);

                printf("GMT : %s\n",GMT);


                item2=cJSON_GetObjectItem(item,"GID");
                array_count = cJSON_GetArraySize(item2);
                item3=cJSON_GetArrayItem(item2,0);
                //		printf("%s\n",cJSON_Print(item3));

                memcpy(WatchFamiGID,item3->valuestring,32);
                printf("WatchFamiGID : %s\n",WatchFamiGID);

                packet_come_flag = 1;



                //pthread_rwlock_wrlock(&(shms->lock));
                //fill in the buff_to_recv
                // 2. 设置 可读 标志
                //shms->shm_state = READABLE;

                //pthread_rwlock_unlock(&(shms->lock));




                printf("\nthe  packet above  is ack pakcet\n");
            }else if(pkt_send_format == LWS_WRITE_BINARY){
                strcat(buf_receve,(char *)in);
                if((int)len < 512)
                {
                    printf("one time recvive all :\n%s\n",buf_receve);
                }

                if(buf_receve[0] == 0xff && buf_receve[1] == 0xff){
                    printf("recv BINARY pkt , but wrong ack\n");
                }else{
                    printf("recv BINARY pkt , right ack\n");
                }

                packet_come_flag = 1;
                printf("\nthe  packet above  is ack pakcet\n");
            }else{
                printf("wrong \n");
                exit(0);
            }




            break;

            /* because we are protocols[0] ... */

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n",__FILE__,__func__,__LINE__);
            if (wsi == wsi_dumb)
            {
                which = "dumb";
                wsi_dumb = NULL;
            }
            if (wsi == wsi_mirror)
            {
                which = "mirror";
                wsi_mirror = NULL;
            }

            for (n = 0; n < ARRAY_SIZE(wsi_multi); n++)
                if (wsi == wsi_multi[n])
                {
                    sprintf(which_wsi, "multi %d", n);
                    which = which_wsi;
                    wsi_multi[n] = NULL;
                }

            lwsl_err("CLIENT_CONNECTION_ERROR: %s: %s\n", which,
                    in ? (char *)in : "(null)");
            break;

        case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED://--------------------------------------------第一次回调的状态,第二次回调的状态
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED\n",__FILE__,__func__,__LINE__);
            if ((strcmp((const char *)in, "deflate-stream") == 0) && deny_deflate)
            {
                lwsl_notice("denied deflate-stream extension\n");
                return 1;
            }
            if ((strcmp((const char *)in, "x-webkit-deflate-frame") == 0))
                return 1;
            if ((strcmp((const char *)in, "deflate-frame") == 0))
                return 1;
            break;

        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP\n",__FILE__,__func__,__LINE__);
            lwsl_notice("lws_http_client_http_response %d\n",
                    lws_http_client_http_response(wsi));
            break;

            /* chunked content */
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ\n",__FILE__,__func__,__LINE__);
            lwsl_notice("LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: %ld\n",
                    (long)len);
            show_http_content((char *)in, len);
            break;

            /* unchunked content */
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_RECEIVE_CLIENT_HTTP\n",__FILE__,__func__,__LINE__);
            {
                char buffer[1024 + LWS_PRE];
                char *px = buffer + LWS_PRE;
                int lenx = sizeof(buffer) - LWS_PRE;

                /*
                 * Often you need to flow control this by something
                 * else being writable.  In that case call the api
                 * to get a callback when writable here, and do the
                 * pending client read in the writeable callback of
                 * the output.
                 *
                 * In the case of chunked content, this will call back
                 * LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ once per
                 * chunk or partial chunk in the buffer, and report
                 * zero length back here.
                 */
                if (lws_http_client_read(wsi, &px, &lenx) < 0)
                    return -1;
            }
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:

            //pthread_cond_signal(&cond_write);
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_WRITEABLE\n",__FILE__,__func__,__LINE__);
            lwsl_info("Client wsi %p writable\n", wsi);

            printf("state LWS_CALLBACK_CLIENT_WRITEABLE\n");
            //	while(1);
            break;

        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:  //---------------------------------------第三次回调的状态
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER\n",__FILE__,__func__,__LINE__);
            if (test_post)
            {
                unsigned char **p = (unsigned char **)in, *end = (*p) + len;

                if (lws_add_http_header_by_token(wsi,
                            WSI_TOKEN_HTTP_CONTENT_LENGTH,
                            (unsigned char *)"29", 2, p, end))
                    return -1;
                if (lws_add_http_header_by_token(wsi,
                            WSI_TOKEN_HTTP_CONTENT_TYPE,
                            (unsigned char *)"application/x-www-form-urlencoded", 33, p, end))
                    return -1;

                /* inform lws we have http body to send */
                lws_client_http_body_pending(wsi, 1);
                //		lws_callback_on_writable(wsi);
            }
            break;

        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_CLIENT_HTTP_WRITEABLE\n",__FILE__,__func__,__LINE__);
            strcpy(buf + LWS_PRE, "text=hello&send=Send+the+form");
            n = lws_write(wsi, (unsigned char *)&buf[LWS_PRE], strlen(&buf[LWS_PRE]), LWS_WRITE_HTTP);
            if (n < 0)
                return -1;
            /* we only had one thing to send, so inform lws we are done
             * if we had more to send, call lws_callback_on_writable(wsi);
             * and just return 0 from callback.  On having sent the last
             * part, call the below api instead.*/
            lws_client_http_body_pending(wsi, 0);
            break;

        case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_COMPLETED_CLIENT_HTTP\n",__FILE__,__func__,__LINE__);
            wsi_dumb = NULL;
            force_exit = 1;
            break;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param) && !defined(LWS_WITH_MBEDTLS)
        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
            printf("sws : %s,%s,line = %d LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS\n",__FILE__,__func__,__LINE__);
            if (crl_path[0])
            {
                /* Enable CRL checking of the server certificate */
                X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
                X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
                SSL_CTX_set1_param((SSL_CTX*)user, param);
                X509_STORE *store = SSL_CTX_get_cert_store((SSL_CTX*)user);
                X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
                int n = X509_load_cert_crl_file(lookup, crl_path, X509_FILETYPE_PEM);
                X509_VERIFY_PARAM_free(param);
                if (n != 1)
                {
                    char errbuf[256];
                    n = ERR_get_error();
                    lwsl_err("LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: SSL error: %s (%d)\n", ERR_error_string(n, errbuf), n);
                    return 1;
                }
            }
            break;
#endif

        default:
            break;
    }

    return 0;
}


static const struct lws_protocols protocols[] =
{
    {
        "dumb-increment-protocol",
        callback_dumb_increment,
        0,
        512,
    },
#if 0
    {
        "lws-mirror-protocol",
        callback_lws_mirror,
        0,
        128,
    }, {
        "lws-test-raw-client",
        callback_test_raw_client,
        0,
        128
    },
#endif
    { NULL, NULL, 0, 0 } /* end */
};

static const struct lws_extension exts[] =
{
    {
        "permessage-deflate",
        lws_extension_callback_pm_deflate,
        "permessage-deflate; client_no_context_takeover"
    },
    {
        "deflate-frame",
        lws_extension_callback_pm_deflate,
        "deflate_frame"
    },
    { NULL, NULL, NULL /* terminator */ }
};




static int shm_init(void){

    key_t key;

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

    bzero(shms,sizeof(struct shm)); //清 0 共享内存

    //下面为初始化 共享内存中的数据
    init_status(&(shms->read_write_state));
    shms->unwriteable_times_send=0;
    //shms->unwriteable_times_recv=0;
    sem_init((sem_t *)&(shms->sem), 0, 1); 
    //shms->shm_state = NUMBER_OF_MEMBERS; //这个状态不用来进行逻辑判断

    return 0;
}



static struct option options[] =
{
    { "help",	no_argument,		NULL, 'h' },
    { "debug",      required_argument,      NULL, 'd' },
    { "port",	required_argument,	NULL, 'p' },
    { "ssl",	no_argument,		NULL, 's' },
    { "strict-ssl",	no_argument,		NULL, 'S' },
    { "version",	required_argument,	NULL, 'v' },
    { "undeflated",	no_argument,		NULL, 'u' },
    { "multi-test",	no_argument,		NULL, 'm' },
    { "nomirror",	no_argument,		NULL, 'n' },
    { "justmirror",	no_argument,		NULL, 'j' },
    { "longlived",	no_argument,		NULL, 'l' },
    { "post",	no_argument,		NULL, 'o' },
    { "pingpong-secs", required_argument,	NULL, 'P' },
    { "ssl-cert",  required_argument,	NULL, 'C' },
    { "ssl-key",  required_argument,	NULL, 'K' },
    { "ssl-ca",  required_argument,		NULL, 'A' },
#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
    { "ssl-crl",  required_argument,		NULL, 'R' },
#endif
    { NULL, 0, 0, 0 }
};

static int ratelimit_connects(unsigned int *last, unsigned int secs)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (tv.tv_sec - (*last) < secs)
    {
        return 0;
    }
    *last = tv.tv_sec;

    return 1;
}


void usage(void){
    fprintf(stderr, "Usage: libwebsockets-test-client "
            "<server address> [--port=<p>] "
            "[--ssl] [-k] [-v <ver>] "
            "[-d <log bitfield>] [-l]\n");
    exit(0);
}




/**************************************************************************************
 * 线程区
 **************************************************************************************/

/*
 *功能 : 按照顺序 
 1. 读取共享内存 中的 东西
 * 		2. 插入链表中
 * 		3. 发送 1.R
 *
 * */


static void * thread_insert(void *arg){

    ack_state_t ack_state = INIT_STATE;

    list_xxx_t *tmp_xxx_node;
    int fifo_fd = -1;
    int i;
    char buf[32];
    bzero(buf,sizeof(buf));

    while(1){
        usleep(1000*1000); //需要被其他机制 替代  // 一秒发送一次,这里待解决.时间长了可能导致对共享内存 区 的数据 汲取不及时,时间快了会导致长期占用信号量.会造成普通进程对信号量汲取的困难
        sem_wait((sem_t *)&(shms->sem));

        if(!is_writeable_send(shms->read_write_state)){ //普通进程 已经写入了

            //1.拷贝共享内存区数据到暂时节点,并修改暂时节点的deadline
            tmp_xxx_node = (list_xxx_t *)malloc(sizeof(list_xxx_t));
            memcpy(&(tmp_xxx_node->data),&(shms->buff_to_send),\
                    sizeof(shms->buff_to_send));
            tmp_xxx_node->data.msg_info.dead_line= SERVER_DEADLINE;

            //2.处理同步信息
            enable_writeable_send(&(shms->read_write_state));
            sem_post((sem_t *)&(shms->sem));


            //3.解析暂时节点中的数据
            printf(YELLOW "recv msg form process : %s,pid : %d,fifo_path : %s\n" NONE,\
                    processtype(tmp_xxx_node->data.msg_info.process_type),\
                    tmp_xxx_node->data.msg_info.pid,\
                    tmp_xxx_node->data.msg_info.fifo_path);

            //4.插入链表 插入 从后插,目前这么做 ,之后根据 1 来 进行选择怎么插入,可能有优先级,链表读写的时候要不要同步,这是个问题//TODO
            printf(YELLOW "Create a Temporary node ,insert Temporary node into Chain list to be sent ... ");
            list_add_tail(&(tmp_xxx_node->list),&list_xxx_head.list);
            printf("done\n" NONE);


            i = 0;
            list_for_each_entry(tmp_xxx_node,&list_xxx_head.list,list)
                i++;
            printf(GREEN "%d nodes in the Chain list to be sent\n" NONE,i);
#if 0

            // 正常情况下,没必要发,如果出错了要发. 那么这个是RA
            // 什么情况下发,这是个问题
            // 1.打开
            fifo_fd = open((char *)(tmp_xxx_node->data.msg_info.fifo_path),O_RDWR);
            if(0 > fifo_fd){
                perror("open");
            }
            bzero(buf,sizeof(buf));
            buf[0]=R1;
            //printf("%d\n",strlen(tmp_xxx_node->data.node.key[R1]));
            snprintf(buf+1,sizeof(buf)-1,"%d",tmp_xxx_node->data.node.key[R1]); //告知某次的R1 , 某次用key_1R 来表示 ,R1 用 buf 的第一个字节表示
            buf[sizeof(buf)-1] = ack_state;

            // 2. 写

            printf(RED "send ack %d.R:%s",buf[0],buf+1);
            write(fifo_fd,buf,sizeof(buf));
            kill(tmp_xxx_node->data.msg_info.pid, SIGUSR1);
            printf("   ...send ack %d.R to %d DONE\n" NONE,buf[0],tmp_xxx_node->data.msg_info.pid);
            // 3.关闭
            close(fifo_fd);
#endif

        }else //没写入
            sem_post((sem_t *)&(shms->sem));
    }
    return NULL;
}

/*
 *功能 : 按照顺序 
 1. 读取链表节点的信息
 2. 组包
 * 		3. 发送消息
 * 		4. 发送 2.R
 * 		5. 删除对应的链表节点
 *
 * */

static void * thread_del_list(void *tool_in){

    int fifo_fd = -1;
    char buf[32];
    struct pthread_routine_tool *tool = (struct pthread_routine_tool*)tool_in;
    unsigned char *audio_pkt_p = NULL;
    unsigned char * tmp_p = NULL;
    char * audio_buf = NULL;
    int ret = 0;
    char *enc = NULL;
    char *out = NULL;
    char * send_json_p = NULL;

    cJSON *root,*fmt_pl ,*fmt_gp;

    char  tmpConKey[64] = {0};

    FILE *fp = NULL;

    int filesize = 0;

    list_xxx_t *tmp_xxx_node;
    struct list_head *pos,*n;
    int ack_state = INIT_STATE;

    int i = 0;

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    // 1. 等待  web_socket 连接建立
    while(!connection_flag)  // 连接建立完成,会达到 一个状态 LWS_CALLBACK_CLIENT_ESTABLISHED ,在这个状态中 , 置位 connection_flag 为 1
        usleep(1000*20);  

    // 2. 发送log_in 包

    generate_login_cjson(&cjson_to_send); //生成 json包
    pkt_send_format = LWS_WRITE_TEXT; //发送的格式
    cid_send = 10211; //设置 cid_send
    //pkt_send_format  cid_send 会在 这次 对应的 收到的 包中进行 校验 
    websocket_write_back(tool->wsi,cjson_to_send,-1,pkt_send_format);

    //2. 等待 log_in 包的 回应
    while(!packet_come_flag)  //packet_come_flag 会在 收到 正确的包 时 置位 1
        usleep(1000*20);  
    packet_come_flag = 0;
    printf("verify packet comes\n");


    while(1){ //这里 循环发送包

        usleep(1000*200); //在 正常工作 时,这个需要 处理一下, 用别的机制代替 //TODO

        //print_count_linkedlist(list_h_pointer); //打印 链表中有多少节点
        //
        i = 0;
        list_for_each_entry(tmp_xxx_node,&list_xxx_head2.list,list)
            i++;
        printf(GREEN "%d nodes in the A chain list has been sent\n" NONE,i);

        //if(list_h_pointer->next == NULL) //如果为空链表,头结点中没有数据
        //	continue;
        if (list_empty(&list_xxx_head.list))
            continue;




        printf(GREEN "send the first node\n");


        // 2. 发送 2.R


        list_for_each_safe(pos,n,&list_xxx_head.list){
            tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据


            //TODO 发送数据
            // 1. 组包  ,引用的为 链表头结点 后面的包

            // list_h_pointer->next->data.context  


            // 2. 发包,发头结点后面的包
            //websocket_write_back

            // 1.打开
            fifo_fd = open(tmp_xxx_node->data.msg_info.fifo_path,O_RDWR);
            if(0 > fifo_fd){
                perror("open");
            }

            bzero(buf,sizeof(buf));
            buf[0] = RECV_SEND;
            snprintf(buf+1,sizeof(buf)-1,"%d",tmp_xxx_node->data.msg_info.count); 
            ack_state = SUCCESS_ACK;
            buf[sizeof(buf)-1] = ack_state;
            printf(YELLOW"send ack after send to pid :%d,name:%s,count: %d,",\
                    tmp_xxx_node->data.msg_info.pid,\
                    processtype(tmp_xxx_node->data.msg_info.process_type),\
                    tmp_xxx_node->data.msg_info.count);


            // 2. 写
            write(fifo_fd,buf,sizeof(buf));
            kill(tmp_xxx_node->data.msg_info.pid,SIGUSR1);
            printf("   ...send ack DONE\n" NONE);

            // 3. 删 掉 发送过的节点
            //	del_front_linkedlist(list_h_pointer,NULL);
            list_del(pos); // 这肯定是第一个节点
            alarm(1);
            list_add_tail(&(tmp_xxx_node->list),&list_xxx_head2.list);

            // 3.关闭
            close(fifo_fd);
        }

        printf(GREEN "send buff_to_send form linklist to ws_server\n" NONE);
        puts("\n");
    }
    return NULL;
}


/*
 *功能 : 按照顺序 
 * 		1. 读取参数,设置变量
 *		2. 依次建立 tcp  ssl web_socket 连接
 * 		3. 接收消息
 * 		4. 发送 3.R 或者 4.R
 * */

int main(int argc, const char *argv[])
{
    bzero(buf_receve,sizeof(buf_receve));
    int n = 0, m, ret = 0, port = 7681, use_ssl = 0, ietf_version = -1;
    unsigned int rl_dumb = 0, do_ws = 1, pp_secs = 0, do_multi = 0;
#if LWS_DEFINE
    unsigned rl_mirror = 0;
#endif
    struct lws_context_creation_info info;//这个是系统设置
    struct lws_client_connect_info i;//这个是个性化设置
    struct lws_context *context;
    const char *prot, *p;
    char path[300];
    char cert_path[1024] = "";
    char key_path[1024] = "";
    char ca_path[1024] = "";
    int count = 0; //此函数 中有一个循环,这个用来 计数循环的次数


    struct pthread_routine_tool tool; //线程创建时,传的参数

    memset(&info, 0, sizeof info);

    lwsl_notice("libwebsockets test client - license LGPL2.1+SLE\n");
    lwsl_notice("(C) Copyright 2010-2016 Andy Green <andy@warmcat.com>\n");


    if (argc < 2)
        usage();

    while (n >= 0)
    {
        n = getopt_long(argc, (char ** const)argv, "Sjnuv:hsp:d:lC:K:A:P:mo", options, NULL);
        if (n < 0)
            continue;
        switch (n)
        {
            case 'd':
                lws_set_log_level(atoi(optarg), NULL);
                break;
            case 's': /* lax SSL, allow selfsigned, skip checking hostname */   //-----
                use_ssl = LCCSCF_USE_SSL |
                    LCCSCF_ALLOW_SELFSIGNED |
                    LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
                break;
            case 'S': /* Strict SSL, no selfsigned, check server hostname */
                use_ssl = LCCSCF_USE_SSL;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'P':
                pp_secs = atoi(optarg);
                lwsl_notice("Setting pingpong interval to %d\n", pp_secs);
                break;
            case 'j':
                justmirror = 1;
                break;
            case 'l':
                longlived = 1;
                break;
            case 'v':
                ietf_version = atoi(optarg);//13
                break;
            case 'u':
                deny_deflate = 1;
                break;
            case 'm':
                do_multi = 1;
                break;
            case 'o':
                test_post = 1;
                break;
            case 'n':
                flag_no_mirror_traffic = 1;
                lwsl_notice("Disabled sending mirror data (for pingpong testing)\n");
                break;
            case 'C':
                strncpy(cert_path, optarg, sizeof(cert_path) - 1);
                cert_path[sizeof(cert_path) - 1] = '\0';
                break;
            case 'K':
                strncpy(key_path, optarg, sizeof(key_path) - 1);
                key_path[sizeof(key_path) - 1] = '\0';
                break;
            case 'A':
                strncpy(ca_path, optarg, sizeof(ca_path) - 1);
                ca_path[sizeof(ca_path) - 1] = '\0';
                break;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
            case 'R':
                strncpy(crl_path, optarg, sizeof(crl_path) - 1);
                crl_path[sizeof(crl_path) - 1] = '\0';
                break;
#endif
            case 'h':
                usage();
        }
    }

    if (optind >= argc)
        usage();

    ret = shm_init();
    if(ret)
        printf(RED "shm_init failed\n" NONE);

    //list_h_pointer = create_linkedlist();

    INIT_LIST_HEAD(&list_xxx_head.list);  
    INIT_LIST_HEAD(&list_xxx_head2.list);  


    signal(SIGINT, sig_handler);
    signal(SIGALRM, sig_handler);

    memset(&i, 0, sizeof(i));

    i.port = port;
    if (lws_parse_uri((char *)argv[optind], &prot, &i.address, &i.port, &p))//---url
        usage();

    /* add back the leading / on path */
    path[0] = '/';
    strncpy(path + 1, p, sizeof(path) - 2);
    path[sizeof(path) - 1] = '\0';
    printf("sws : %s,%s,line = %d,path = %s,prot = %s\n",__FILE__,__func__,__LINE__,path,prot);//好像是个管道 //  /svc/pipe   wss
    i.path = path;//------------

    if (!strcmp(prot, "http") || !strcmp(prot, "ws"))//---ssl
    {
        printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        use_ssl = 0;
    }
    if (!strcmp(prot, "https") || !strcmp(prot, "wss"))//---  这里了
        if (!use_ssl)
        {
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            use_ssl = LCCSCF_USE_SSL;
        }
    /*
     * create the websockets context.  This tracks open connections and
     * knows how to route any traffic and which protocol version to use,
     * and if each connection is client or server side.
     *
     * For this client-only demo, we tell it to not listen on any port.
     */

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.ws_ping_pong_interval = pp_secs;
    info.extensions = exts;

#if defined(LWS_OPENSSL_SUPPORT)
    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif

    if (use_ssl)  //----ssl
    {
        /*
         * If the server wants us to present a valid SSL client certificate
         * then we can set it up here.
         */

        if (cert_path[0])//---
        {
            info.client_ssl_cert_filepath = cert_path;//---
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        }
        if (key_path[0])
        {
            info.client_ssl_private_key_filepath = key_path;
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        }
        /*
         * A CA cert and CRL can be used to validate the cert send by the server
         */
        if (ca_path[0])
        {
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            info.client_ssl_ca_filepath = ca_path;
        }

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
        else if (crl_path[0])
            lwsl_notice("WARNING, providing a CRL requires a CA cert!\n");
#endif
    }

    if (use_ssl & LCCSCF_USE_SSL)
        lwsl_notice(" Using SSL\n");
    else
        lwsl_notice(" SSL disabled\n");
    if (use_ssl & LCCSCF_ALLOW_SELFSIGNED)
        lwsl_notice(" Selfsigned certs allowed\n");
    else
        lwsl_notice(" Cert must validate correctly (use -s to allow selfsigned)\n");
    if (use_ssl & LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK)
        lwsl_notice(" Skipping peer cert hostname check\n");
    else
        lwsl_notice(" Requiring peer cert hostname matches\n");

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    context = lws_create_context(&info);  //这里面进去了 callback_dumb_increment
    if (context == NULL)
    {
        fprintf(stderr, "Creating libwebsocket context failed\n");
        return 1;
    }
    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    i.context = context;
    i.ssl_connection = use_ssl;//----
    i.host = i.address;
    i.origin = i.address;
    i.ietf_version_or_minus_one = ietf_version;//-------

    if (!strcmp(prot, "http") || !strcmp(prot, "https"))
    {
        lwsl_notice("using %s mode (non-ws)\n", prot);
        if (test_post)
        {
            i.method = "POST";
            lwsl_notice("POST mode\n");
        }
        else
            i.method = "GET";
        printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        do_ws = 0;
    }
    else if (!strcmp(prot, "raw"))
    {
        i.method = "RAW";
        i.protocol = "lws-test-raw-client";
        lwsl_notice("using RAW mode connection\n");
        printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        do_ws = 0;
    }
    else
        lwsl_notice("using %s mode (ws)\n", prot);

    /*
     * sit there servicing the websocket context to handle incoming
     * packets, and drawing random circles on the mirror protocol websocket
     *
     * nothing happens until the client websocket connection is
     * asynchronously established... calling lws_client_connect() only
     * instantiates the connection logically, lws_service() progresses it
     * asynchronously.
     */

    //


    //i填充已经完毕


    //lws_client_connect_via_info
    if (do_multi)  //0
    {
        printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
        for (n = 0; n < ARRAY_SIZE(wsi_multi); n++)
        {
            if (!wsi_multi[n] && ratelimit_connects(&rl_multi[n], 2u))
            {
                lwsl_notice("dumb %d: connecting\n", n);
                i.protocol = protocols[PROTOCOL_DUMB_INCREMENT].name;
                i.pwsi = &wsi_multi[n];
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
                lws_client_connect_via_info(&i);
            }
        }
    }
    else
    {
        printf("sws : %s,%s,line = %d,do_ws = %d\n",__FILE__,__func__,__LINE__,do_ws);//1

        if (do_ws)  // 1
        {
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);//1
            if (!justmirror && !wsi_dumb && ratelimit_connects(&rl_dumb, 2u))   // return 0//-----------------------  //目前就这一个工作
            {
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);//1
                lwsl_notice("dumb: connecting\n");
                i.protocol = protocols[PROTOCOL_DUMB_INCREMENT].name;
                i.pwsi = &wsi_dumb;
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

                lws_client_connect_via_info(&i);//  再循环中只执行一次

                tool.wsi = wsi_dumb;
                tool.context = context;

            }

#if LWS_DEFINE
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);//1
            if (!wsi_mirror && ratelimit_connects(&rl_mirror, 2u))  //-----------------------------------------------
            {
                lwsl_notice("mirror: connecting\n");
                i.protocol = protocols[PROTOCOL_LWS_MIRROR].name;
                i.pwsi = &wsi_mirror;
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
                wsi_mirror = lws_client_connect_via_info(&i);////////再循环中只循环一次
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

            }
#endif
        }
        else
        {
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            if (!wsi_dumb && ratelimit_connects(&rl_dumb, 2u))  //----------------------------------
            {
                lwsl_notice("http: connecting\n");
                i.pwsi = &wsi_dumb;
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
                lws_client_connect_via_info(&i);
            }
        }
    }

    if(0 != pthread_create(&pthid_insert,NULL,thread_insert,NULL)){
        perror("pthid1");
        return -1;
    }

    if(0 != pthread_create(&pthid_del_linklist,NULL,thread_del_list,&tool)){
        perror("pthid1");
        return -1;
    }

    m = 0;
    while (!force_exit)  //会在ctrl + c的时候 force_exit 变为 1 ,条件不满足
    {
        count ++;

        printf("sws : %s,%s,line = %d-------------------------------------------%d-----lws_service begin\n",__FILE__,__func__,__LINE__,count);//1
        lws_service(context, 500);//在循环中执行n次
        printf("sws : %s,%s,line = %d-------------------------------------------%d-----lws_service end\n",__FILE__,__func__,__LINE__,count);//1

        if (do_multi)
        {
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            m++;
            if (m == 10)
            {
                m = 0;
                lwsl_notice("doing lws_callback_on_writable_all_protocol\n");
                lws_callback_on_writable_all_protocol(context, &protocols[PROTOCOL_DUMB_INCREMENT]);
                printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            }
        }
        sleep(2);
    }
    lwsl_err("Exiting\n");
    lws_context_destroy(context);//这个里面调用了一次  	PROTOCOL_DUMB_INCREMENT               //context 里面 记录了这些东西

    return 0;
}
