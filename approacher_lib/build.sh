#!/bin/bash
# approacher_lib 编译脚本
# 用于编译 approacher 和 semantic_approacher

echo "======================================="
echo "Approacher Library 编译脚本"
echo "======================================="

# 设置编译器和编译选项
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall"
INCLUDES="-I. -Ithings -Iinclude"
LIBS="-Llib -lobjectbox -lpthread"

# 检查必要的目录和文件
if [ ! -d "things/concepts-db" ]; then
    echo "警告: 数据库目录 things/concepts-db 不存在，将自动创建"
    mkdir -p things/concepts-db
fi

# 编译 ObjectBox模型文件
echo ""
echo "编译 concepts.obx.cpp..."
$CXX $CXXFLAGS $INCLUDES -c things/concepts.obx.cpp -o concepts.obx.o
if [ $? -ne 0 ]; then
    echo "错误: concepts.obx.cpp 编译失败"
    exit 1
fi

# 编译 ConceptDatabase
echo ""
echo "编译 ConceptDatabase.cpp..."
$CXX $CXXFLAGS $INCLUDES -c ConceptDatabase.cpp -o ConceptDatabase.o
if [ $? -ne 0 ]; then
    echo "错误: ConceptDatabase 编译失败"
    exit 1
fi

# 编译 approacher
echo ""
echo "编译 approacher..."
$CXX $CXXFLAGS $INCLUDES approacher.cpp ConceptDatabase.o concepts.obx.o $LIBS -o approacher
if [ $? -ne 0 ]; then
    echo "错误: approacher 编译失败"
    exit 1
fi
chmod +x approacher
echo "✓ approacher 编译成功"

# 编译 semantic_approacher
echo ""
echo "编译 semantic_approacher..."
$CXX $CXXFLAGS $INCLUDES semantic_approacher.cpp ConceptDatabase.o concepts.obx.o $LIBS -o semantic_approacher
if [ $? -ne 0 ]; then
    echo "错误: semantic_approacher 编译失败"
    exit 1
fi
chmod +x semantic_approacher
echo "✓ semantic_approacher 编译成功"

# 清理中间文件
rm -f *.o

echo ""
echo "======================================="
echo "编译完成！"
echo ""
echo "可执行文件:"
echo "  - approacher: 基础相似度分析器"
echo "  - semantic_approacher: 语义增强分析器"
echo ""
echo "使用示例:"
echo "  交互式模式: ./approacher"
echo "  命令行模式: ./approacher \"red,apple\" \"green,apple\""
echo "  静默模式:   ./semantic_approacher -q \"美丽,女孩\" \"女孩\""
echo "======================================="