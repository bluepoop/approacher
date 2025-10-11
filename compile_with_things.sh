#!/bin/bash

# Approacher编译脚本 - 使用~/things数据库
echo "编译Approacher程序（使用~/things数据库）..."

# 设置路径
THINGS_DIR="/home/laplace/things"
INCLUDE_DIR="$THINGS_DIR/include"
LIB_DIR="$THINGS_DIR/lib"

# 编译
g++ -std=c++17 \
    -I"$INCLUDE_DIR" \
    -L"$LIB_DIR" \
    -o approacher \
    approacher.cpp \
    "$THINGS_DIR/ConceptDatabase.cpp" \
    "$THINGS_DIR/concepts.obx.cpp" \
    -lobjectbox -pthread

if [ $? -eq 0 ]; then
    echo "编译成功！"
    echo ""
    echo "运行程序："
    echo "export LD_LIBRARY_PATH=\"$LIB_DIR:\$LD_LIBRARY_PATH\""
    echo "./approacher"
    echo ""
    echo "或者直接运行："
    echo "LD_LIBRARY_PATH=\"$LIB_DIR\" ./approacher"
else
    echo "编译失败！"
    exit 1
fi