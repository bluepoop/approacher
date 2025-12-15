# Approacher Library 部署说明

## 项目结构

```
approacher_lib/
├── approacher.cpp          # 基础相似度分析器源码
├── semantic_approacher.cpp # 语义增强分析器源码
├── ConceptDatabase.cpp     # 概念数据库实现
├── ConceptDatabase.hpp     # 概念数据库头文件
├── build.sh               # 编译脚本
├── things/                # 依赖和数据目录
│   ├── concepts-db/       # ObjectBox数据库
│   ├── ConceptDatabase.*  # 数据库实现文件
│   └── example.txt        # 示例数据
├── include/               # 头文件目录
└── lib/                   # 库文件目录
```

## 部署步骤

### 1. 编译程序

```bash
cd approacher_lib
chmod +x build.sh
./build.sh
```

### 2. 设置环境变量

```bash
export LD_LIBRARY_PATH="./lib:$LD_LIBRARY_PATH"
```

或将其添加到 ~/.bashrc 中永久生效。

## 使用方法

### 命令行参数模式（推荐用于程序调用）

#### approacher 基础分析器

```bash
# 帮助信息
./approacher -h

# 非交互模式（返回相似度）
./approacher "red,apple" "green,apple"

# 静默模式（只输出数值）
./approacher -q "red,apple" "green,apple"

# 模糊匹配模式
./approacher -f "red,apple" "green,apple"
```

#### semantic_approacher 语义增强分析器

```bash
# 帮助信息
./semantic_approacher -h

# 非交互模式
./semantic_approacher "美丽,温柔,女孩" "女孩"

# 静默模式（只输出数值，适合程序调用）
./semantic_approacher -q "key=value" "key"
```

### 交互式CLI模式

```bash
# 基础分析器
./approacher

# 语义增强分析器
./semantic_approacher
```

## C++程序调用示例

### 方法1：使用system()或popen()调用

```cpp
#include <iostream>
#include <cstdio>
#include <string>

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
```

### 方法2：使用exec()族函数

```cpp
#include <unistd.h>
#include <sys/wait.h>

double callSemanticApproacher(const std::string& input_a, const std::string& input_b) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        chdir("approacher_lib");
        setenv("LD_LIBRARY_PATH", "./lib:$LD_LIBRARY_PATH", 1);

        execl("./semantic_approacher", "semantic_approacher",
              "-q", input_a.c_str(), input_b.c_str(), nullptr);
        exit(1);
    }

    // 父进程
    close(pipefd[1]);
    char buffer[128];
    read(pipefd[0], buffer, sizeof(buffer));
    wait(nullptr);

    return std::stod(buffer);
}
```

## 功能列表

### approacher 基础分析器

- 概念相似度计算
- 精确/模糊匹配模式
- 键值对处理
- 参数学习（交互式模式）

### semantic_approacher 语义增强分析器

- 所有approacher功能
- 语义包含关系检测
- 复合词分析
- 形容词-名词结构分析
- 等号键值对特殊处理

## 返回值说明

- 命令行模式：成功返回0，失败返回1
- 相似度值：0.0 到 100.0 之间的浮点数
- 静默模式：仅输出纯数值，无额外信息

## 故障排查

1. **找不到库文件**
   ```bash
   export LD_LIBRARY_PATH="./lib:$LD_LIBRARY_PATH"
   ```

2. **数据库初始化失败**
   - 检查 things/concepts-db 目录是否存在
   - 检查 things/example.txt 文件是否存在

3. **编译失败**
   - 确保安装了 g++ 编译器（C++17支持）
   - 检查 ObjectBox 库是否在 lib 目录中

## 性能优化建议

1. 使用静默模式（-q）减少输出开销
2. 批量调用时保持程序运行，避免重复初始化
3. 对于大量查询，考虑使用共享内存或套接字通信