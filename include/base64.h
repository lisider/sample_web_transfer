#ifndef __BASE64__
#define __BASE64__

char * base64_encode(const char* data, int data_len); 
char * base64_decode(const char* data, int data_len); 
char find_pos(char ch); 

#endif
