#!/bin/sh
export CROSS_COMPILE="/home/yajianz/tools/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-"
mkdir -p build && cd build/
cmake ../
make
