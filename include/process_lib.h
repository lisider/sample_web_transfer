#ifndef __PROCESS_LIB__
#define __PROCESS_LIB__

typedef void (*call_back_fun_t)(void);


enum ERROR_send_pkt{
	R1_NOT_RECEVE = 1,
	R2_NOT_RECEVE,
	R3_NOT_RECEVE,
	R4_NOT_RECEVE,
	R1_READ_ERROR = 5,
	R2_READ_ERROR,
	R3_READ_ERROR,
	R4_READ_ERROR,
	R1_WRONG_ACK  = 8,
	R2_WRONG_ACK ,
	R3_WRONG_ACK ,
	R4_WRONG_ACK 
};



extern void call_back_register(call_back_fun_t *p,int count);
extern void creat_fifo(const char *path);
extern int pkt_service(msg_send_t * send_pkt_p,int size,int count);
extern int pkt_send(msg_send_t * send_pkt_p,int size,int count);
extern void lib_exit_sig(int arg);
#endif
