# Approacher 概念相似度分析器 - 详细实现计划

本文档记录了Approacher项目的完整实现计划，包括5个开发阶段和20个核心函数的详细规格。

## 项目概述

**目标**: 实现一个分辨生活概念相似程度的程序
**核心算法**: 基于概念特征库的加权相似度计算
**技术栈**: C++ + 内存数据结构
**特色功能**: 递归模糊匹配 + 参数自学习

---

## 阶段1: 基础数据结构与解析

### 1.1 数据结构定义
```cpp
struct Feature {
    string key;      // 键名，如"color"，空字符串表示纯值
    string value;    // 值，如"red"
    bool is_exact;   // true=键值对匹配，false=模糊值匹配
};

struct Concept {
    int id;
    vector<Feature> features;
};

struct MatchResult {
    int concept_id;
    int match_count;
    vector<int> matched_indices;  // 匹配的特征索引
};

struct TrainingSample {
    vector<Feature> features_A;
    vector<Feature> features_B;
    double expected_similarity;
    double confidence;  // 用户对此样本的信心度
};
```

### 1.2 概念库加载函数
**函数名**: `loadConceptDatabase`
**输入**: `string filename` (如"example.txt")
**输出**: `vector<Concept> concepts`
**功能**: 从文件加载概念库，解析每行的特征
**解析规则**:
- 格式: `[name:apple,color:red,position:home]`
- 提取键值对，生成Feature结构

### 1.3 特征解析函数
**函数名**: `parseFeatureList`
**输入**: `vector<string> input_list` (如["red", "name:apple"])
**输出**: `vector<Feature> parsed_features`
**功能**: 将输入字符串列表解析为Feature结构
**解析规则**:
- "red" → Feature{key="", value="red", is_exact=false}
- "name:apple" → Feature{key="name", value="apple", is_exact=true}

### 1.4 字符串分词函数
**函数名**: `tokenizeValue`
**输入**: `string value` (如"red hat")
**输出**: `vector<string> tokens` (如["red", "hat"])
**功能**: 按空格分词，用于模糊匹配
**注意**: 只处理空格分隔，不处理其他标点符号

---

## 阶段2: 精确匹配算法核心

### 2.1 单个概念匹配函数
**函数名**: `matchConceptExact`
**输入**: `vector<Feature> input_features, Concept concept`
**输出**: `MatchResult result`
**功能**: 计算输入特征与单个概念的精确匹配结果
**匹配规则**:
- is_exact=true: 必须key和value都匹配
- is_exact=false: 只要value匹配即可，忽略key

### 2.2 概念库批量匹配函数
**函数名**: `findMatchingConcepts`
**输入**: `vector<Feature> input_features, vector<Concept> concepts`
**输出**: `vector<MatchResult> matches`
**功能**: 找出所有与输入特征匹配的概念及匹配度
**返回**: 只返回match_count > 0的结果

### 2.3 重合分析函数
**函数名**: `analyzeOverlap`
**输入**: `vector<MatchResult> matches_A, vector<MatchResult> matches_B`
**输出**: `map<pair<int,int>, int> overlap_map`
**功能**: 分析两个匹配结果的重合情况，按匹配数量分类统计
**输出格式**: `{<(match_count_A, match_count_B)>: count}`
**示例**: `{<(2,1)>: 2, <(1,1)>: 1}` 表示有2个概念A匹配2个B匹配1个，1个概念双方各匹配1个

### 2.4 分相似度计算函数
**函数名**: `calculatePartialSimilarity`
**输入**: `map<pair<int,int>, int> overlap_map, int total_matches, unordered_map<string, double> params`
**输出**: `double partial_similarity`
**功能**: 根据重合分析和参数计算分相似度
**公式**: `sum(count * params["p" + to_string(i) + to_string(j)]) / total_matches`

### 2.5 主相似度计算函数
**函数名**: `calculateMainSimilarity`
**输入**: `vector<Feature> features_A, vector<Feature> features_B, vector<Concept> concepts, unordered_map<string, double> params`
**输出**: `double main_similarity`
**功能**: 计算两个特征列表的主相似度（n=0精确匹配）
**公式**: `partial_similarity_A * partial_similarity_B`

---

## 阶段3: 递归模糊匹配

### 3.1 值相似度计算函数
**函数名**: `calculateValueSimilarity`
**输入**: `string value1, string value2, int remaining_levels, unordered_map<string, double> params`
**输出**: `double similarity`
**功能**: 递归计算两个值的相似度（如"red" vs "red hat"）
**递归逻辑**:
- remaining_levels=0: 只判断完全相等
- remaining_levels>0: 分词后递归计算子词相似度

### 3.2 模糊匹配判断函数
**函数名**: `isFuzzyMatch`
**输入**: `Feature input_feature, Feature concept_feature, double threshold, int remaining_levels, unordered_map<string, double> params`
**输出**: `bool is_match`
**功能**: 判断两个特征是否在模糊匹配下相似
**判断标准**: `calculateValueSimilarity() >= threshold`

### 3.3 扩展相似度计算函数
**函数名**: `calculateSimilarityWithFuzzy`
**输入**: `vector<Feature> features_A, vector<Feature> features_B, vector<Concept> concepts, unordered_map<string, double> params, int n_levels, double fuzzy_threshold`
**输出**: `double similarity`
**功能**: 带递归模糊匹配的完整相似度计算
**逻辑**: 先尝试精确匹配，如果结果不理想则启用模糊匹配

---

## 阶段4: 参数学习系统

### 4.1 参数梯度计算函数
**函数名**: `calculateParameterGradients`
**输入**: `vector<TrainingSample> samples, unordered_map<string, double> current_params`
**输出**: `unordered_map<string, double> gradients`
**功能**: 计算损失函数对各参数的梯度
**损失函数**: `sum((predicted - expected)^2 * confidence)`

### 4.2 参数优化函数
**函数名**: `optimizeParameters`
**输入**: `vector<TrainingSample> training_data, unordered_map<string, double> initial_params, double learning_rate, int max_iterations`
**输出**: `unordered_map<string, double> optimized_params`
**功能**: 使用梯度下降优化pij参数
**优化算法**: 标准梯度下降 + 动量项

### 4.3 参数验证函数
**函数名**: `validateParameters`
**输入**: `unordered_map<string, double> params, vector<TrainingSample> test_data`
**输出**: `double validation_score`
**功能**: 在测试集上验证参数的性能
**评分标准**: 1 - 平均相对误差

### 4.4 交互式学习函数
**函数名**: `interactiveLearning`
**输入**: `vector<pair<vector<string>, vector<string>>> test_cases`
**输出**: `unordered_map<string, double> learned_params`
**功能**: 与用户交互，收集训练样本并学习参数

---

## 阶段5: 集成与测试

### 5.1 主接口函数
**函数名**: `calculateSimilarity` (主接口)
**输入**: `vector<string> list1, vector<string> list2, string concept_db_file, string params_file, int n_levels=0, double fuzzy_threshold=0.5`
**输出**: `double similarity`
**功能**: 完整的相似度计算接口
**流程**: 加载数据 → 解析特征 → 计算相似度 → 返回结果

### 5.2 参数管理函数
**函数名**: `loadParameters` / `saveParameters`
**输入**: `string filename` / `(unordered_map<string, double> params, string filename)`
**输出**: `unordered_map<string, double>` / `bool success`
**功能**: 参数的持久化存储和加载
**格式**: JSON或简单的key=value格式

### 5.3 单元测试函数
**函数名**: `runUnitTests`
**输入**: 无
**输出**: `bool all_passed`
**功能**: 运行所有单元测试，验证各模块功能
**测试用例**: 包含边界情况、错误输入、性能测试

### 5.4 性能基准测试函数
**函数名**: `benchmarkPerformance`
**输入**: `vector<pair<vector<string>, vector<string>>> test_cases, int iterations`
**输出**: `double average_time_ms`
**功能**: 测试算法性能，识别瓶颈
**输出**: 平均执行时间和内存使用情况

---

## 实现顺序建议

1. **第一优先级**: 阶段1 + 阶段2 (基础数据结构和精确匹配)
   - 这是整个系统的核心，必须先确保正确性
   - 可以用example.txt进行初步测试

2. **第二优先级**: 阶段5.1-5.2 (主接口和参数管理)
   - 提供完整的使用接口
   - 便于后续功能的集成测试

3. **第三优先级**: 阶段3 (递归模糊匹配)
   - 在精确匹配稳定后再添加复杂功能
   - 需要仔细调试递归逻辑

4. **第四优先级**: 阶段4 (参数学习)
   - 最复杂的部分，需要数值计算
   - 可能需要引入外部数学库

5. **最后**: 阶段5.3-5.4 (测试和性能优化)
   - 在所有功能完成后进行全面测试

---

## 开发注意事项

1. **错误处理**: 每个函数都要有完善的错误处理和输入验证
2. **内存管理**: 注意大型概念库的内存使用优化
3. **性能优化**: 关键路径上的算法优化，如匹配算法的时间复杂度
4. **可扩展性**: 预留接口便于后续功能扩展
5. **调试友好**: 充分的日志输出和中间结果可视化

## 测试策略

1. **单元测试**: 每个函数独立测试
2. **集成测试**: 端到端的相似度计算测试
3. **性能测试**: 大规模数据下的性能表现
4. **用户接受度测试**: 与用户预期结果的对比