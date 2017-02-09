#!/bin/bash

CUR_PATH=`pwd`
#build libevent
cd $CUR_PATH"/libevent-1.4.14-stable/"
./configure
make clean;make

#build jsoncpp
cd $CUR_PATH"/jsoncpp/"
rm -rf $CUR_PATH"/jsoncpp/libs"
scons platform=linux-gcc
jsoncpp_lib=`find $CUR_PATH"/jsoncpp/libs/" -name "*.a"`

cd $CUR_PATH
rm -f libjsoncpp.a
ln -s $jsoncpp_lib libjsoncpp.a
make clean;make
