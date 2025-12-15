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
