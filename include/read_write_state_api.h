/*************************************************************************
    > File Name: read_write_state_api.h
    > Author: Sues
    > Mail: sumory.kaka@foxmail.com 
    > Created Time: Wed 28 Mar 2018 09:35:35 AM CST
 ************************************************************************/


#ifndef __READ_WRITE_STATE_API__
#define __READ_WRITE_STATE_API__

int is_writeable_send(char value);
int is_writeable_recv(char value);
void enable_writeable_send(char * value);
void enable_writeable_recv(char *value);
void disable_writeable_send(char *value);
void disable_writeable_recv(char *value);
void init_status(char *value);


#endif
