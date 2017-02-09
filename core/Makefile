CC = g++
#FLAGS = --shared -g -Wall -fPIC
#FLAGS = -g -Wall 
FLAGS = -g
#LIBS_INSTALL_DIR = ../../libs
INC = -I./libevent-1.4.14-stable -I./jsoncpp/include -I./cppjieba/include -I./cppjieba/deps
#EXE = 3rd_ip_entry.so
EXE = comse

CPPSOURCES=$(wildcard *.cpp)
CPPOBJS=$(patsubst %.cpp, %.o,$(CPPSOURCES))

SOURCES = $(CSOURCES) 
OBJS = $(CPPOBJS)

LIBS = ./libevent-1.4.14-stable/.libs/libevent.a ./libjsoncpp.a -lpthread

all: $(EXE)

%.o:%.c
	$(CC) -c $(FLAGS) $(INC) -o $@ $<
%.o:%.cpp
	$(CC) -c $(FLAGS) $(INC) -o $@ $<

$(EXE): $(OBJS)
	$(CC) $(FLAGS) $(INC) -o $(EXE) $(OBJS) $(LIBS)
clean:
	rm -rf $(OBJS) $(EXE) core.*
