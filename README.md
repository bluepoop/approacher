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

## 当前实现状态

- ✅ 基础数据结构定义完成
- ⏳ 正在实现 loadConceptDatabase 临时测试函数
- ⏸️ 待实现 parseFeatureList 用户输入解析函数
- ⏸️ 待测试数据加载和解析功能

## 概念库格式

当前使用的测试文件 `example.txt` 格式：
```
1.[name:apple,color:red,position:home]
2.[name:apple,color:red,position:shop]
3.[name:apple,color:green,position:shop]
4.[name:book,color:red,content:travel]
```

每行格式：`ID.[key1:value1,key2:value2,...]`