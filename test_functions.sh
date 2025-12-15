#!/bin/bash
# 功能测试脚本

echo "========================================"
echo "Approacher 功能测试脚本"
echo "========================================"

# 设置库路径
export LD_LIBRARY_PATH="approacher_lib/lib:$LD_LIBRARY_PATH"

echo ""
echo "1. 测试基础approacher（直接调用）"
echo "--------------------------------"

echo "测试1: red,apple vs green,apple"
cd approacher_lib
./approacher -q "red,apple" "green,apple"
echo "退出码: $?"

echo ""
echo "测试2: content vs apple"
./approacher -q "content" "apple"
echo "退出码: $?"

cd ..

echo ""
echo "2. 测试语义增强approacher"
echo "-------------------------"

echo "测试1: key=value vs key (应该返回100)"
cd approacher_lib
./semantic_approacher -q "key=value" "key"
echo "退出码: $?"

echo ""
echo "测试2: 空对空 (应该返回100)"
./semantic_approacher -q "" ""
echo "退出码: $?"

cd ..

echo ""
echo "3. 测试帮助信息"
echo "---------------"

echo "基础approacher帮助:"
cd approacher_lib
./approacher -h

echo ""
echo "语义增强approacher帮助:"
./semantic_approacher -h

cd ..

echo ""
echo "4. 测试快捷脚本"
echo "---------------"

echo "测试快捷脚本调用:"
./call_approacher.sh "content" "apple" -q
echo "退出码: $?"

./call_approacher.sh "key=value" "key" -s -q
echo "退出码: $?"

echo ""
echo "========================================"
echo "功能测试完成"
echo "========================================"