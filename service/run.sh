#!/bin/bash

cp ../core/comse ./
nohup ./comse ./conf/comse.conf 1>out 2>err &
