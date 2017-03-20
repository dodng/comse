#!/bin/bash

CUR_PATH=`pwd`
#build libevent,just build one time
tar xvzf libevent-1.4.14-stable.tar.gz
cd $CUR_PATH"/libevent-1.4.14-stable/"
./configure
make clean;make

#build comse
cd $CUR_PATH
make clean;make
