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

每行格式：`ID.[key1:value1,key2:value2,...]`