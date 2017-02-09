CC = g++
#FLAGS = --shared -g -Wall -fPIC
FLAGS = -g -Wall 
LIBS_INSTALL_DIR = ../../libs
INC = -I/home/dodng/lab/comse/core/cppjieba/include -I/home/dodng/lab/comse/core/cppjieba/deps
#EXE = 3rd_ip_entry.so
EXE = test_here

CPPSOURCES=$(wildcard *.cpp)
CPPOBJS=$(patsubst %.cpp, %.o,$(CPPSOURCES))

SOURCES = $(CSOURCES) 
OBJS = $(CPPOBJS)

#LIBS = -L$(LIBS_INSTALL_DIR) -lpolicy_manager

all: $(EXE)

%.o:%.cpp
	$(CC) -c $(FLAGS) $(INC) -o $@ $<

$(EXE): $(OBJS)
	$(CC) $(FLAGS) $(INC) -o $(EXE) $(OBJS) $(LIBS)
clean:
	rm -rf $(OBJS) $(EXE) core.*
