#!/bin/bash

echo "清空数据库..."
if [ -f concepts-db/data.mdb ]; then
    rm concepts-db/data.mdb concepts-db/lock.mdb
    echo "数据库已清空"
fi

echo "编译 Approacher..."
g++ -std=c++17 -I./include -L./lib -o approacher approacher.cpp ConceptDatabase.cpp concepts.obx.cpp objectbox-model.h -lobjectbox -pthread

if [ $? -eq 0 ]; then
    echo "编译成功！"
    echo "运行程序："
    export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
    ./approacher
else
    echo "编译失败！"
    exit 1
fi