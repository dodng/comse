#!/bin/bash

cp ../core/comse ./

mkdir -p cppjieba
cp -r ../core/cppjieba/dict/ ./cppjieba

nohup ./comse ./conf/comse.conf 1>out 2>err &
