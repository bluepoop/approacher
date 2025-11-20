#!/bin/bash

# Semantic Approacher编译脚本
echo "编译Semantic Approacher程序（语义增强版）..."

# 设置路径
THINGS_DIR="/home/laplace/things"
INCLUDE_DIR="$THINGS_DIR/include"
LIB_DIR="$THINGS_DIR/lib"

echo "使用things目录: $THINGS_DIR"

# 编译
g++ -std=c++17 \
    -I"$INCLUDE_DIR" \
    -L"$LIB_DIR" \
    -o semantic_approacher \
    semantic_approacher.cpp \
    "$THINGS_DIR/ConceptDatabase.cpp" \
    "$THINGS_DIR/concepts.obx.cpp" \
    -lobjectbox -pthread

if [ $? -eq 0 ]; then
    echo "✅ Semantic Approacher 编译成功！"
    echo ""
    echo "运行程序："
    echo "export LD_LIBRARY_PATH=\"$LIB_DIR:\$LD_LIBRARY_PATH\""
    echo "./semantic_approacher"
    echo ""
    echo "或者直接运行："
    echo "LD_LIBRARY_PATH=\"$LIB_DIR\" ./semantic_approacher"
    echo ""
    echo "程序功能："
    echo "- 🧠 语义预处理（框架已就绪，逻辑待填入）"
    echo "- 🔗 调用原Approacher进行计算"
    echo "- 📝 语义后处理（框架已就绪，逻辑待填入）"
    echo "- 🔄 'direct'命令可切换直接模式"
    echo ""
    echo "现在可以在preprocessInput()和postprocessOutput()函数中"
    echo "添加你的语义分析逻辑了！"
else
    echo "❌ 编译失败！"
    exit 1
fi