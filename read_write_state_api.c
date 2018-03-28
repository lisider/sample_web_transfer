/*************************************************************************
    > File Name: read_write_state_api.c
    > Author: Sues
    > Mail: sumory.kaka@foxmail.com 
    > Created Time: Wed 28 Mar 2018 09:38:41 AM CST
 ************************************************************************/

int is_writeable_send(char value){
         return value&0x01;
}

int is_writeable_recv(char value){
         return (value&0x02) >> 1;
}

void enable_writeable_send(char * value){
        *value = *value | 0x01;
}

void enable_writeable_recv(char *value){
        *value = *value | 0x02;
}

void disable_writeable_send(char *value){
        *value = *value & 0xFE;
}

void disable_writeable_recv(char *value){
        *value = *value & 0xFD;
}
void init_status(char *value){
        *value = 3;
}
