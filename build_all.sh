#!/bin/bash
# 完整项目编译脚本

echo "=========================================="
echo "Approacher Project 完整编译脚本"
echo "=========================================="

# 1. 编译 approacher_lib
echo ""
echo "1. 编译 approacher_lib..."
echo "----------------------------"

cd approacher_lib
if [ ! -f "build.sh" ]; then
    echo "错误: approacher_lib/build.sh 不存在"
    exit 1
fi

chmod +x build.sh
./build.sh

if [ $? -ne 0 ]; then
    echo "错误: approacher_lib 编译失败"
    exit 1
fi

cd ..

# 2. 编译主调用程序
echo ""
echo "2. 编译主调用程序..."
echo "----------------------------"

if [ ! -f "main_caller.cpp" ]; then
    echo "错误: main_caller.cpp 不存在"
    exit 1
fi

g++ -std=c++17 -O2 -Wall main_caller.cpp -o main_caller

if [ $? -ne 0 ]; then
    echo "错误: main_caller 编译失败"
    exit 1
fi

chmod +x main_caller
echo "✓ main_caller 编译成功"

# 3. 创建快捷脚本
echo ""
echo "3. 创建快捷脚本..."
echo "----------------------------"

# 创建运行脚本
cat > run_demo.sh << 'EOF'
#!/bin/bash
# 演示脚本

echo "Approacher Project 演示"
echo "======================="

# 设置库路径
export LD_LIBRARY_PATH="approacher_lib/lib:$LD_LIBRARY_PATH"

# 运行主调用程序
./main_caller
EOF

chmod +x run_demo.sh
echo "✓ run_demo.sh 创建成功"

# 创建直接调用脚本
cat > call_approacher.sh << 'EOF'
#!/bin/bash
# 直接调用approacher的快捷脚本

if [ $# -lt 2 ]; then
    echo "用法: $0 <输入A> <输入B> [选项]"
    echo "选项:"
    echo "  -s, --semantic  使用语义增强分析器"
    echo "  -q, --quiet     静默模式"
    echo "  -f, --fuzzy     模糊匹配（仅基础分析器）"
    echo ""
    echo "示例:"
    echo "  $0 \"red,apple\" \"green,apple\""
    echo "  $0 \"美丽,女孩\" \"女孩\" -s"
    echo "  $0 \"key=value\" \"key\" -s -q"
    exit 1
fi

INPUT_A="$1"
INPUT_B="$2"
shift 2

USE_SEMANTIC=false
PROGRAM_ARGS=""

# 解析选项
for arg in "$@"; do
    case $arg in
        -s|--semantic)
            USE_SEMANTIC=true
            ;;
        -q|--quiet)
            PROGRAM_ARGS="$PROGRAM_ARGS -q"
            ;;
        -f|--fuzzy)
            if [ "$USE_SEMANTIC" = false ]; then
                PROGRAM_ARGS="$PROGRAM_ARGS -f"
            fi
            ;;
    esac
done

# 设置库路径并运行
cd approacher_lib
export LD_LIBRARY_PATH="./lib:$LD_LIBRARY_PATH"

if [ "$USE_SEMANTIC" = true ]; then
    ./semantic_approacher $PROGRAM_ARGS "$INPUT_A" "$INPUT_B"
else
    ./approacher $PROGRAM_ARGS "$INPUT_A" "$INPUT_B"
fi
EOF

chmod +x call_approacher.sh
echo "✓ call_approacher.sh 创建成功"

# 4. 检查文件结构
echo ""
echo "4. 检查项目结构..."
echo "----------------------------"

echo "项目文件:"
ls -la main_caller run_demo.sh call_approacher.sh 2>/dev/null || echo "某些文件不存在"

echo ""
echo "approacher_lib 文件:"
ls -la approacher_lib/{approacher,semantic_approacher} 2>/dev/null || echo "某些可执行文件不存在"

echo ""
echo "=========================================="
echo "编译完成！"
echo ""
echo "可用命令:"
echo "  ./main_caller          - 运行C++主调用程序演示"
echo "  ./run_demo.sh          - 运行完整演示"
echo "  ./call_approacher.sh   - 快捷调用脚本"
echo ""
echo "快捷调用示例:"
echo "  ./call_approacher.sh \"red,apple\" \"green,apple\""
echo "  ./call_approacher.sh \"美丽,女孩\" \"女孩\" -s -q"
echo "  ./call_approacher.sh \"key=value\" \"key\" --semantic"
echo "=========================================="