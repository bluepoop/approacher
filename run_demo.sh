#!/bin/bash
# 演示脚本

echo "Approacher Project 演示"
echo "======================="

# 设置库路径
export LD_LIBRARY_PATH="approacher_lib/lib:$LD_LIBRARY_PATH"

# 运行主调用程序
./main_caller
