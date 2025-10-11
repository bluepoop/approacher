# Approacher 概念相似度分析器

## 项目概述

Approacher 是一个用于分析生活概念相似程度的程序。它通过预建的概念数据库，计算两个特征列表之间的相似度。

## 核心数据结构

### 1. Feature 结构体

用于表示单个特征，是系统的基本数据单元。

```cpp
struct Feature {
    string key;      // 键名，如"color"，空字符串表示模糊匹配
    string value;    // 值，如"red"

    Feature(const string& k = "", const string& v = "")
        : key(k), value(v) {}
};
```

**用途说明：**
- **概念库中的特征**: 都是完整键值对，如 `Feature("color", "red")`
- **用户输入特征**:
  - 有键名：`Feature("name", "apple")` → 精确匹配
  - 无键名：`Feature("", "red")` → 模糊匹配

**使用示例：**
```cpp
// 概念库：[name:apple,color:red,position:home]
Feature("name", "apple")
Feature("color", "red")
Feature("position", "home")

// 用户输入：["red", "name:apple"]
Feature("", "red")        // 模糊匹配，在所有value中找相似的
Feature("name", "apple")  // 精确匹配，必须key和value都对应
```

### 2. Concept 结构体

用于表示一个完整的概念，包含ID和多个特征。

```cpp
struct Concept {
    int id;                    // 概念的唯一标识符
    vector<Feature> features;  // 这个概念包含的所有特征

    Concept(int concept_id = 0) : id(concept_id) {}
};
```

**ID的作用：**
1. **匹配结果标识**: 区分哪个概念参与了匹配
2. **重合分析**: 找出两个输入共同匹配的概念
3. **调试验证**: 追踪哪个概念贡献了相似度
4. **扩展功能**: 支持概念权重、使用频率统计等

**使用示例：**
```cpp
// example.txt 中的 "1.[name:apple,color:red,position:home]"
Concept concept;
concept.id = 1;
concept.features = {
    Feature("name", "apple"),
    Feature("color", "red"),
    Feature("position", "home")
};
```

### 3. MatchResult 结构体

记录一个输入特征列表与一个概念的匹配结果。

```cpp
struct MatchResult {
    int concept_id;              // 匹配的概念ID
    int match_count;             // 匹配到的特征数量
    vector<int> matched_indices; // 匹配的特征在概念中的位置索引

    MatchResult(int id = 0, int count = 0)
        : concept_id(id), match_count(count) {}
};
```

**使用示例：**
```cpp
// 用户输入 ["red", "apple"] 与概念1匹配
// 概念1: [name:apple, color:red, position:home]
//        索引:  0        1          2

// 匹配过程：
// "red" (模糊) → 匹配 "color:red" (索引1)
// "apple" (模糊) → 匹配 "name:apple" (索引0)

MatchResult result;
result.concept_id = 1;           // 匹配的是概念1
result.match_count = 2;          // 匹配了2个特征
result.matched_indices = {0, 1}; // 匹配了索引0和1的特征
```

**重要性：**
- 为相似度计算提供基础数据
- 支持pij参数的计算（需要知道匹配数量）
- 避免重复计算，提高效率

### 4. TrainingSample 结构体

用于参数学习系统，存储用户提供的训练样本。

```cpp
struct TrainingSample {
    vector<Feature> features_A;      // 第一个特征列表
    vector<Feature> features_B;      // 第二个特征列表
    double expected_similarity;      // 用户期望的相似度值
    double confidence;              // 用户对这个判断的信心度

    TrainingSample(double similarity = 0.0, double conf = 1.0)
        : expected_similarity(similarity), confidence(conf) {}
};
```

**使用场景：**
当算法计算的相似度与用户预期不符时，收集训练样本优化参数。

**使用示例：**
```cpp
// 用户认为 ["red"] 和 ["apple"] 的相似度应该是 0.7
TrainingSample sample1;
sample1.features_A = {Feature("", "red")};
sample1.features_B = {Feature("", "apple")};
sample1.expected_similarity = 0.7;
sample1.confidence = 0.9;  // 90%确信这个判断

// 用户认为 ["red", "apple"] 和 ["red"] 的相似度应该是 0.4
TrainingSample sample2;
sample2.features_A = {Feature("", "red"), Feature("", "apple")};
sample2.features_B = {Feature("", "red")};
sample2.expected_similarity = 0.4;
sample2.confidence = 0.8;  // 80%确信
```

**参数学习流程：**
1. **收集样本**: 用户提供期望的相似度结果
2. **计算差异**: 当前算法结果 vs 期望结果
3. **优化参数**: 调整 p11, p12, p21 等参数，减少差异
4. **考虑信心度**: 信心度高的样本在优化中权重更大

## 匹配策略

### 精确匹配 vs 模糊匹配

1. **精确匹配**: 用户输入有键名，如 `"name:apple"`
   - 必须找到概念中 key="name" 且 value="apple" 的特征
   - 完全对应才算匹配

2. **模糊匹配**: 用户输入无键名，如 `"red"`
   - 在概念的所有特征值中寻找相似的
   - 支持部分匹配和递归匹配

### 数据流向

```
概念库文件 → loadConceptDatabase() → vector<Concept>
用户输入 → parseFeatureList() → vector<Feature>
两者匹配 → findMatchingConcepts() → vector<MatchResult>
重合分析 → analyzeOverlap() → 相似度计算
```

## 已实现函数详解

### 1. matchConceptExact 函数

**功能**: 计算输入特征列表与单个概念的精确匹配结果

```cpp
MatchResult ConceptDatabase::matchConceptExact(
    const vector<Feature>& input_features,
    const unique_ptr<Concept>& concept
);
```

**参数**:
- `input_features`: 用户输入的特征列表
- `concept`: 要匹配的概念对象

**返回值**: `MatchResult` - 包含概念ID、匹配数量、匹配索引

**匹配逻辑**:
- **模糊匹配**（key为空）: 在概念的所有feature_values中查找相同值
- **精确匹配**（key不为空）: 查找key和value都相同的特征对

**示例**:
```cpp
// 输入: ["red", "name:apple"]
// 概念1: [name:apple, color:red, position:home]
// 结果: MatchResult{concept_id=1, match_count=2, matched_indices=[0,1]}
//       - "red" 匹配 color:red (输入索引0)
//       - "name:apple" 匹配 name:apple (输入索引1)
```

### 2. findMatchingConcepts 函数

**功能**: 根据特征列表查找数据库中所有匹配的概念

```cpp
vector<MatchResult> ConceptDatabase::findMatchingConcepts(
    const vector<Feature>& input_features
);
```

**参数**:
- `input_features`: 用户输入的特征列表

**返回值**: `vector<MatchResult>` - 所有匹配结果的列表（只包含match_count > 0的结果）

**工作流程**:
1. 获取数据库中所有概念
2. 对每个概念调用 `matchConceptExact`
3. 过滤掉无匹配的结果
4. 返回有效匹配列表

**示例**:
```cpp
// 输入: ["red", "name:apple"]
// 返回:
// [
//   MatchResult{concept_id=1, match_count=2, matched_indices=[0,1]},
//   MatchResult{concept_id=2, match_count=2, matched_indices=[0,1]},
//   MatchResult{concept_id=3, match_count=1, matched_indices=[1]},
//   MatchResult{concept_id=4, match_count=1, matched_indices=[0]}
// ]
```

### 3. parseFeatureList 函数

**功能**: 将用户输入的字符串列表解析为Feature对象

```cpp
vector<Feature> parseFeatureList(const vector<string>& input_list);
```

**参数**:
- `input_list`: 用户输入的字符串列表

**返回值**: `vector<Feature>` - 解析后的特征列表

**解析规则**:
- **包含冒号**: `"name:apple"` → `Feature("name", "apple")` (精确匹配)
- **不含冒号**: `"red"` → `Feature("", "red")` (模糊匹配)

**示例**:
```cpp
// 输入: ["red", "name:apple", "green"]
// 输出:
// [
//   Feature("", "red"),
//   Feature("name", "apple"),
//   Feature("", "green")
// ]
```

## 当前实现状态

- ✅ 基础数据结构定义完成
- ✅ ObjectBox数据库集成完成
- ✅ 概念数据加载和存储完成
- ✅ 基础查询功能实现（按值查询、按键值对查询）
- ✅ 用户输入解析功能完成
- ✅ **Stage 2 第一部分**: 概念匹配功能完成
  - ✅ `matchConceptExact` - 单个概念匹配
  - ✅ `findMatchingConcepts` - 批量概念匹配
  - ✅ 匹配功能测试和验证完成
- ✅ **Stage 2 已完成**:
  - ✅ `analyzeOverlap` - 分析两个匹配结果的重合（基于重合度等级）
  - ✅ `calculatePartialSimilarity` - 计算分相似度
  - ✅ `calculateMainSimilarity` - 计算主相似度（几何平均数）
  - ✅ `calculateMatchLevel` - 计算重合度等级（1-5）

## 概念库格式

当前使用的测试文件 `example.txt` 格式：
```
1.[name:apple,color:red,position:home]
2.[name:apple,color:red,position:shop]
3.[name:apple,color:green,position:shop]
4.[name:book,color:red,content:travel]
```

## 算法更新记录

### 2025-10-06: 重合度百分比算法重构

**重要变更**：相似度计算算法从"匹配特征数"改为"重合度百分比等级"

#### 核心改进

1. **重合度等级计算**
   - 重合度 = 匹配特征数 / 总特征数 × 100%
   - 向上取整到20%档次：20%, 40%, 60%, 80%, 100%
   - 对应等级：1, 2, 3, 4, 5

2. **参数表扩展**
   - 从 p11-p33 扩展到 p11-p55
   - 支持所有重合度等级组合
   - 更精细的相似度权重控制

3. **主相似度优化**
   - 改为几何平均数：√(A分相似度 × B分相似度)
   - 避免单方向低相似度严重拖累总体
   - 结果更平衡和直观

4. **交互式界面**
   - 支持逗号分隔的多特征输入
   - 实时计算并显示分相似度和主相似度
   - 格式：[A]->[B], [A]<-[B], [A]<->[B]

#### 新算法流程

1. **特征解析**：解析逗号分隔的用户输入
2. **概念匹配**：在数据库中查找匹配的概念
3. **重合度计算**：计算每个重合概念的A、B重合度等级
4. **分相似度**：Σ(概念数 × p_ij) / 总匹配概念数
5. **主相似度**：√(A分相似度 × B分相似度)

#### 技术实现

- 新增 `calculateMatchLevel()` 函数
- 重写 `analyzeOverlap()` 支持重合度等级
- 更新 `calculatePartialSimilarity()` 使用新参数
- 修改 `calculateMainSimilarity()` 应用开根号
- 扩展参数表到25个p参数（p11-p55）

此更新使相似度计算更加合理，能够更好地反映概念间的真实相似程度。

### 2025-10-11: 第三阶段模糊匹配和参数学习系统

**重要扩展**：实现了高级模糊匹配和参数学习功能，大幅提升系统智能化水平

#### 核心新功能

1. **字符串模糊匹配**
   - 实现 Levenshtein 编辑距离算法
   - 支持字符串相似度计算（0-1评分）
   - 可配置相似度阈值（默认0.6）

2. **递归匹配机制**
   - 支持多层递归查找相似概念
   - 可配置递归深度（默认2层）
   - 匹配强度随递归深度衰减

3. **参数学习优化**
   - 基于用户反馈的训练样本收集
   - 数值梯度下降参数优化算法
   - 支持信心度加权的误差计算
   - 参数范围约束（0.1-5.0）

4. **参数持久化**
   - 参数文件保存/加载功能
   - 注释格式的可读配置文件
   - 参数版本管理支持

5. **交互式界面增强**
   - 'fuzzy' 命令切换模糊匹配模式
   - 'params' 命令进入参数学习
   - 'save'/'load' 命令管理参数
   - 详细的匹配信息显示

#### 技术架构

**新增函数列表**：
```cpp
// 模糊匹配核心
int calculateStringDistance(str1, str2)           // 编辑距离
double calculateStringSimilarity(str1, str2)      // 相似度计算
vector<pair<string,double>> findSimilarValues()   // 模糊查找
MatchResult matchConceptFuzzy()                   // 模糊概念匹配
vector<MatchResult> recursiveMatch()              // 递归匹配

// 参数学习系统
void addTrainingSample(sample)                    // 添加训练样本
void optimizeParameters()                         // 参数优化
double evaluateParameters(params)                 // 参数评估
bool saveParameters(filename)                     // 保存参数
bool loadParameters(filename)                     // 加载参数

// 增强的匹配函数
vector<MatchResult> findMatchingConcepts()        // 支持模糊匹配重载
```

#### 使用示例

**模糊匹配模式**：
```bash
输入对象A: fuzzy          # 切换到模糊匹配
模糊匹配模式: 开启
输入对象A: red            # 能匹配 "reed", "read" 等相似词
输入对象B: book           # 能匹配 "books", "booking" 等
匹配模式: 模糊匹配
匹配概念数 - A: 12, B: 4, 重合: 4
```

**参数学习模式**：
```bash
输入对象A: params         # 进入参数学习
输入训练样本数量: 2
--- 训练样本 1 ---
输入对象A: red,apple
输入对象B: green,apple
期望相似度 (0-1): 0.8
信心度 (0-1): 0.9
开始参数优化...
```

#### 技术特性

- **智能匹配**：从纯精确匹配升级为智能模糊匹配
- **自适应学习**：根据用户反馈自动优化参数
- **多层递归**：支持概念间的深度关联发现
- **配置管理**：灵活的参数保存和版本控制
- **用户友好**：直观的交互命令和详细结果显示

第三阶段的实现使Approacher从基础相似度计算工具发展为具备学习能力的智能概念分析系统。

## 技术讨论与改进空间

### 🤔 当前实现的争议点

#### 1. **递归匹配复杂度问题**
```cpp
vector<MatchResult> recursiveMatch(const vector<Feature>& input_features, int max_depth = 2, double fuzzy_threshold = 0.6)
```

**潜在问题**：
- 递归算法可能导致指数级复杂度爆炸 O(n² × m²)
- 数据库增大时性能会急剧下降
- 缺少递归调用计数限制

**改进方向**：添加缓存机制、迭代实现、调用次数限制

#### 2. **参数优化算法可靠性**
```cpp
void optimizeParameters(int max_iterations = 100, double learning_rate = 0.01)
```

**潜在问题**：
- 数值梯度计算精度有限（epsilon = 0.001）
- 固定学习率可能不适合所有场景
- 容易陷入局部最优解
- 缺少动量和自适应机制

**改进方向**：实现Adam/AdaGrad算法、自适应学习率、梯度检查

#### 3. **模糊匹配阈值合理性**
```cpp
double fuzzy_threshold = 0.6;  // 默认阈值
```

**潜在问题**：
- 0.6阈值缺乏理论依据
- 对不同长度字符串不够灵活
- 用户无法运行时调整
- 缺少领域相关的阈值标准

**改进方向**：动态阈值调整、用户配置接口、字符串长度自适应

#### 4. **内存管理和持久化**
```cpp
vector<TrainingSample> training_samples;  // 仅内存存储
```

**潜在问题**：
- 训练样本程序重启后丢失
- 大量样本可能导致内存溢出
- 缺少训练历史管理

**改进方向**：文件持久化、数据库存储、增量学习

#### 5. **递归强度衰减公式**
```cpp
recursive_result.match_count = max(1, recursive_result.match_count / 2);
```

**潜在问题**：
- 简单除以2过于粗糙
- 未考虑递归深度影响
- 最小值强制为1可能不合理

**改进方向**：指数衰减函数、深度加权、配置化衰减参数

#### 6. **字符串相似度计算局限性**
```cpp
return 1.0 - (double)edit_distance / max_length;
```

**潜在问题**：
- 仅基于编辑距离，忽略语义相似性
- 对同义词无法识别（"苹果" vs "apple"）
- 缺少中文特殊处理
- 未考虑词汇相关性

**改进方向**：引入词向量、语义嵌入、多语言支持、领域词典

#### 7. **用户界面复杂性**
```cpp
else if (line_a == "params") {
    // 复杂的参数学习流程
}
```

**潜在问题**：
- 参数学习对普通用户过于复杂
- 缺少操作指导和帮助信息
- 无参数优化效果可视化
- 错误处理不够友好

**改进方向**：用户指导向导、效果可视化、简化交互流程

### 💡 未来发展路线图

#### Phase 4A: 性能与稳定性优化
- [ ] 实现递归匹配缓存机制
- [ ] 添加调用深度和次数限制
- [ ] 训练样本文件持久化
- [ ] 参数范围检查和验证
- [ ] 内存使用优化

#### Phase 4B: 算法智能化升级
- [ ] 实现Adam/L-BFGS优化算法
- [ ] 动态阈值调整机制
- [ ] 指数衰减强度函数
- [ ] 自适应学习率调整
- [ ] 多目标参数优化

#### Phase 4C: 语义理解增强
- [ ] 词向量/word embeddings集成
- [ ] 多语言支持（中英文等）
- [ ] 领域特定词典
- [ ] 语义相似度计算
- [ ] 概念关系图谱

#### Phase 4D: 用户体验提升
- [ ] 图形化参数学习界面
- [ ] 优化效果可视化
- [ ] 智能推荐和提示
- [ ] 批量处理和导入导出
- [ ] 性能监控仪表板

### 📊 技术债务管理

**高优先级**（影响核心功能）：
- 递归匹配性能优化
- 训练样本持久化
- 参数优化算法改进

**中优先级**（影响用户体验）：
- 动态阈值调整
- 用户界面简化
- 错误处理改进

**低优先级**（功能增强）：
- 语义相似度计算
- 多语言支持
- 可视化界面

这些争议点和改进建议为Approacher的持续发展提供了明确的技术路线图，同时也为开源贡献者提供了参与方向。

## 🗂️ 项目结构和部署

### 目录结构

```
~/approacher/                    # 主项目目录
├── approacher.cpp              # 主程序文件
├── compile_with_things.sh      # 编译脚本
├── test_*.sh                   # 测试脚本
└── README.md                   # 项目文档

~/things/                       # 共享数据库目录
├── ConceptDatabase.hpp         # 数据库类头文件
├── ConceptDatabase.cpp         # 数据库类实现
├── concepts.obx.hpp/.cpp       # ObjectBox生成文件
├── objectbox-model.h           # ObjectBox模型定义
├── example.txt                 # 示例概念数据
├── parameters.txt              # 参数配置文件（自动生成）
├── concepts-db/                # ObjectBox数据库文件
│   ├── data.mdb
│   └── lock.mdb
├── include/                    # ObjectBox头文件
│   └── objectbox.hpp
└── lib/                        # ObjectBox动态库
    └── libobjectbox.so
```

### 🚀 编译和运行

#### 方法1：使用编译脚本（推荐）
```bash
# 编译程序
./compile_with_things.sh

# 运行程序
LD_LIBRARY_PATH="/home/laplace/things/lib" ./approacher
```

#### 方法2：手动编译
```bash
# 编译命令
g++ -std=c++17 \
    -I/home/laplace/things/include \
    -L/home/laplace/things/lib \
    -o approacher \
    approacher.cpp \
    /home/laplace/things/ConceptDatabase.cpp \
    /home/laplace/things/concepts.obx.cpp \
    -lobjectbox -pthread

# 设置库路径并运行
export LD_LIBRARY_PATH="/home/laplace/things/lib:$LD_LIBRARY_PATH"
./approacher
```

### 🔗 数据库共享机制

**优势**：
- ✅ **全局共享**：`~/things` 目录下的数据库可被系统任意程序访问
- ✅ **数据持久化**：概念数据和参数配置在程序重启后保留
- ✅ **多程序协作**：不同项目可以使用相同的概念数据库
- ✅ **统一管理**：所有概念分析工具共享一套参数和数据

**访问方式**：
```cpp
// 任何程序都可以这样访问数据库
ConceptDatabase db;
db.initialize("/home/laplace/things/concepts-db");
db.loadFromFile("/home/laplace/things/example.txt");
```

### 📂 文件说明

| 文件 | 作用 | 位置 |
|------|------|------|
| **ConceptDatabase.hpp/cpp** | 数据库核心功能 | `~/things/` |
| **concepts.obx.*** | ObjectBox自动生成 | `~/things/` |
| **concepts-db/** | 数据库存储目录 | `~/things/` |
| **example.txt** | 示例概念数据 | `~/things/` |
| **parameters.txt** | 训练参数文件 | `~/things/`（自动生成）|
| **include/lib/** | ObjectBox依赖 | `~/things/` |

### 🔧 配置管理

**参数文件位置**：`~/things/parameters.txt`
**数据库位置**：`~/things/concepts-db/`
**示例数据**：`~/things/example.txt`

使用 `save` 和 `load` 命令可以方便地管理参数配置，所有设置都保存在 `~/things` 目录中，实现了真正的全局共享。

每行格式：`ID.[key1:value1,key2:value2,...]`