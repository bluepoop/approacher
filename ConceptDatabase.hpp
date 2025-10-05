#pragma once

#include <vector>
#include <string>
#include <memory>
#include "objectbox.hpp"
#include "concepts.obx.hpp"
#include "objectbox-model.h"

using namespace std;

// 用户输入特征结构
struct Feature {
    string key;      // 键名，空字符串表示模糊匹配
    string value;    // 值

    Feature(const string& k = "", const string& v = "")
        : key(k), value(v) {}
};

// 匹配结果结构
struct MatchResult {
    obx_id concept_id;
    int match_count;
    vector<int> matched_indices;  // 匹配的特征索引

    MatchResult(obx_id id = 0, int count = 0)
        : concept_id(id), match_count(count) {}
};

// 训练样本结构
struct TrainingSample {
    vector<Feature> features_A;
    vector<Feature> features_B;
    double expected_similarity;
    double confidence;

    TrainingSample(double similarity = 0.0, double conf = 1.0)
        : expected_similarity(similarity), confidence(conf) {}
};

// ObjectBox数据库管理类
class ConceptDatabase {
private:
    unique_ptr<obx::Store> store;
    unique_ptr<obx::Box<Concept>> conceptBox;

public:
    // 初始化数据库
    bool initialize(const string& dbPath = "concepts-db");

    // 从文件加载概念数据库
    bool loadFromFile(const string& filename);

    // 按ID查找概念
    unique_ptr<Concept> findById(obx_id id);

    // 按值模糊查找概念（用于模糊匹配）
    vector<unique_ptr<Concept>> findByValue(const string& value);

    // 按键值对精确查找概念（用于精确匹配）
    vector<unique_ptr<Concept>> findByKeyValue(const string& key, const string& value);

    // 获取所有概念
    vector<unique_ptr<Concept>> getAllConcepts();

    // 获取数据库统计信息
    void printStatistics();

    // Stage 2: 概念匹配和相似度计算功能

    // 根据特征列表查找匹配的概念
    vector<MatchResult> findMatchingConcepts(const vector<Feature>& input_features);

    // 计算单个概念的匹配结果
    MatchResult matchConceptExact(const vector<Feature>& input_features, const unique_ptr<Concept>& concept);
};

// 工具函数：解析用户输入特征列表
vector<Feature> parseFeatureList(const vector<string>& input_list);