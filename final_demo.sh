#!/bin/bash
# 最终演示脚本

echo "========================================"
echo "Approacher Project 最终演示"
echo "基于approacher库重构，支持父程序调用"
echo "========================================"

echo ""
echo "项目结构:"
echo "  approacher_lib/     - 子文件夹，包含核心程序"
echo "  │── approacher      - 基础相似度分析器"
echo "  │── semantic_approacher - 语义增强分析器"
echo "  │── build.sh        - 编译脚本"
echo "  │── DEPLOYMENT.md   - 部署说明"
echo "  └── things/         - 数据库和依赖"
echo "  main_caller.cpp     - C++主调用程序"
echo "  build_all.sh        - 完整编译脚本"
echo "  call_approacher.sh  - 快捷调用脚本"

echo ""
echo "========================================"
echo "1. 直接调用演示"
echo "========================================"

echo ""
echo "[基础分析器] 测试 \"red,apple\" vs \"green,apple\""
cd approacher_lib
export LD_LIBRARY_PATH="./lib:$LD_LIBRARY_PATH"
./approacher -q "red,apple" "green,apple"

echo ""
echo "[基础分析器] 测试 \"content\" vs \"apple\""
./approacher -q "content" "apple"

echo ""
echo "[语义增强] 测试等号键值对 \"key=value\" vs \"key\""
./semantic_approacher -q "key=value" "key"

echo ""
echo "[语义增强] 测试相同的键值对 \"name=test\" vs \"name=test\""
./semantic_approacher -q "name=test" "name=test"

cd ..

echo ""
echo "========================================"
echo "2. 快捷脚本调用演示"
echo "========================================"

echo ""
echo "使用快捷脚本调用基础分析器:"
echo "$ ./call_approacher.sh \"red,apple\" \"green,apple\" -q"
./call_approacher.sh "red,apple" "green,apple" -q

echo ""
echo "使用快捷脚本调用语义增强分析器:"
echo "$ ./call_approacher.sh \"key=value\" \"key\" -s -q"
./call_approacher.sh "key=value" "key" -s -q

echo ""
echo "========================================"
echo "3. C++程序调用示例代码"
echo "========================================"

echo ""
echo "以下是C++程序中调用approacher功能的示例代码:"
echo ""
cat << 'EOF'
// 包含头文件
#include <iostream>
#include <string>
#include <cstdio>

// 调用基础approacher的函数
double callApproacher(const std::string& input_a, const std::string& input_b) {
    std::string command = "cd approacher_lib && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./approacher -q \"" + input_a + "\" \"" + input_b + "\"";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1.0;

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    return std::stod(result);
}

// 调用语义增强approacher的函数
double callSemanticApproacher(const std::string& input_a, const std::string& input_b) {
    std::string command = "cd approacher_lib && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./semantic_approacher -q \"" + input_a + "\" \"" + input_b + "\"";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1.0;

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    return std::stod(result);
}

// 使用示例
int main() {
    // 基础相似度计算
    double score1 = callApproacher("red,apple", "green,apple");
    std::cout << "基础相似度: " << score1 << std::endl;

    // 语义增强计算
    double score2 = callSemanticApproacher("key=value", "key");
    std::cout << "语义增强: " << score2 << std::endl;

    return 0;
}
EOF

echo ""
echo "========================================"
echo "4. 部署和使用说明"
echo "========================================"

echo ""
echo "部署步骤:"
echo "1. 编译程序:     ./build_all.sh"
echo "2. 设置库路径:   export LD_LIBRARY_PATH=\"approacher_lib/lib:\$LD_LIBRARY_PATH\""
echo "3. 使用程序:     见下面的调用方式"

echo ""
echo "调用方式:"
echo ""
echo "命令行直接调用:"
echo "  cd approacher_lib && ./approacher \"input1\" \"input2\""
echo "  cd approacher_lib && ./semantic_approacher \"input1\" \"input2\""
echo ""
echo "使用快捷脚本:"
echo "  ./call_approacher.sh \"input1\" \"input2\"          # 基础分析器"
echo "  ./call_approacher.sh \"input1\" \"input2\" -s       # 语义增强"
echo "  ./call_approacher.sh \"input1\" \"input2\" -q       # 静默模式"
echo ""
echo "C++程序调用:"
echo "  参见上述示例代码，使用popen()或system()调用"

echo ""
echo "命令行选项:"
echo "  -h, --help      显示帮助信息"
echo "  -q, --quiet     静默模式，只输出相似度数值"
echo "  -s, --semantic  使用语义增强分析器（仅限快捷脚本）"
echo "  -f, --fuzzy     模糊匹配模式（仅基础分析器）"

echo ""
echo "========================================"
echo "演示完成！"
echo ""
echo "特性总结:"
echo "✓ 子文件夹架构 - 核心程序在approacher_lib/目录"
echo "✓ 保留原CLI   - 支持交互式和命令行参数模式"
echo "✓ C++调用接口 - 提供main_caller.cpp示例"
echo "✓ 快捷脚本    - call_approacher.sh简化调用"
echo "✓ 完整文档    - DEPLOYMENT.md部署说明"
echo "✓ 自动编译    - build_all.sh一键编译"
echo "========================================"