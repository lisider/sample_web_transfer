export CC=gcc
export CPP=g++
export CFLAGS= -g -lpthread


CSRCS = $(wildcard *.c)  
COBJS = $(patsubst %.c, %, $(CSRCS))  
CPPSRCS = $(wildcard *.cpp)  
CPPOBJS += $(patsubst %.cpp, %, $(CPPSRCS))  


CHEADERS = $(wildcard *.h)  

OBJ = $(COBJS)
OBJ += $(CPPOBJS)

all: ws_client process1 process2

%.o:%.c 
	$(CC) -c $^ -o $@  -g

%.o:%.cpp 
	$(CPP) -c  $^ -o $@ -g

ws_client : ws_client.o linklist.o
	$(CC) $^ -o $@ $(CFLAGS)

process1 : process1.o
	$(CC) $^ -o $@ $(CFLAGS)

process2 : process2.o
	$(CC) $^ -o $@ $(CFLAGS)
	


clean:
	rm *.o $(OBJ)  -rf

.PHONY:clean
