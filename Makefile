export CC=gcc
export CPP=g++
export CFLAGS= -g -lpthread -D_DEBUG -lwebsockets -lpthread -lcjson  -lbase64 -lm -ldl


CSRCS = $(wildcard *.c)  
COBJS = $(patsubst %.c, %, $(CSRCS))  
CPPSRCS = $(wildcard *.cpp)  
CPPOBJS += $(patsubst %.cpp, %, $(CPPSRCS))  


CHEADERS = $(wildcard *.h)  

OBJ = $(COBJS)
OBJ += $(CPPOBJS)

all: ws_client process

%.o:%.c 
	$(CC) -c $^ -o $@  -g $(CFLAGS)

%.o:%.cpp 
	$(CPP) -c  $^ -o $@ -g $(CFLAGS)

ws_client : ws_client.o linklist.o
	$(CC) $^ -o $@ $(CFLAGS)

process : process.o
	$(CC) $^ -o $@ $(CFLAGS)



clean:
	rm *.o $(OBJ)  -rf

.PHONY:clean
