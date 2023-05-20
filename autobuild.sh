#!/bin/bash

set -e

# 如果没有 build 目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 删除 build 目录中的内容
rm -rf `pwd`/build/*

# 在 build 目录中构建项目
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo；so 库拷贝到 /usr/lib    PATH
if [ ! -d /usr/include/mymuduo ]; then 
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

# 刷新动态库缓存
ldconfig