CC=gcc
CPP=g++
CFLAGS = -g -D_DEBUG -I./include  # 编译的时候用到的
LDFLAGS = -L./lib#链接的时候用到的
LIBS = -lpthread   -lbase64  -lcJSON -lcrypto -lssl -lwebsockets -lpthread  -lm -Wl,-rpath=/usr/local/lib -Wl,-rpath,./lib


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

ws_client : ws_client.o linklist.o read_write_state_api.o
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

process : process.o process_lib.o read_write_state_api.o
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)



clean:
	rm *.o $(OBJ)  core -rf

.PHONY:clean
