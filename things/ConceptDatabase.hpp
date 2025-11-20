#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <algorithm>
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
    vector<TrainingSample> training_samples;  // 训练样本存储

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

    // 根据特征列表查找匹配的概念（精确匹配）
    vector<MatchResult> findMatchingConcepts(const vector<Feature>& input_features);

    // 根据特征列表查找匹配的概念（支持模糊匹配）
    vector<MatchResult> findMatchingConcepts(const vector<Feature>& input_features, bool use_fuzzy_matching, double fuzzy_threshold = 0.6, int max_recursive_depth = 2);

    // 计算单个概念的匹配结果
    MatchResult matchConceptExact(const vector<Feature>& input_features, const unique_ptr<Concept>& concept);

    // 复合词匹配辅助函数：生成所有保持顺序的子序列组合
    vector<vector<int>> generateSubsequenceIndices(int n);

    // 复合词匹配辅助函数：检查复合词匹配
    int checkCompoundWordMatches(const vector<Feature>& input_features, const unique_ptr<Concept>& concept, vector<int>& matched_indices);

    // 分析两个匹配结果的重合情况（基于重合度等级）
    map<pair<int,int>, int> analyzeOverlap(const vector<MatchResult>& matches_A, const vector<MatchResult>& matches_B, int total_features_A, int total_features_B, int& total_matches);

    // 计算重合度等级（1-5，对应20%-100%）
    int calculateMatchLevel(int matched_features, int total_features);

    // 计算分相似度
    double calculatePartialSimilarity(const map<pair<int,int>, int>& overlap_map, int divisor, const unordered_map<string, double>& params);

    // 计算主相似度
    double calculateMainSimilarity(const vector<Feature>& features_A, const vector<Feature>& features_B, const unordered_map<string, double>& params);

    // Stage 3: 模糊匹配和参数学习功能

    // 计算字符串编辑距离（Levenshtein距离）
    int calculateStringDistance(const string& str1, const string& str2);

    // 计算字符串相似度（0-1之间）
    double calculateStringSimilarity(const string& str1, const string& str2);

    // 模糊查找相似值
    vector<pair<string, double>> findSimilarValues(const string& query_value, double min_similarity = 0.6);

    // 支持模糊匹配的概念匹配
    MatchResult matchConceptFuzzy(const vector<Feature>& input_features, const unique_ptr<Concept>& concept, double fuzzy_threshold = 0.6);

    // 递归匹配功能（支持深度限制）
    vector<MatchResult> recursiveMatch(const vector<Feature>& input_features, int max_depth = 2, double fuzzy_threshold = 0.6);

    // 训练样本管理
    void addTrainingSample(const TrainingSample& sample);
    vector<TrainingSample> getTrainingSamples();
    void clearTrainingSamples();

    // 参数优化
    void optimizeParameters(int max_iterations = 100, double learning_rate = 0.01);
    double evaluateParameters(const unordered_map<string, double>& params);

    // 参数持久化
    bool saveParameters(const string& filename = "parameters.txt");
    bool loadParameters(const string& filename = "parameters.txt");
};

// 全局pij参数配置
extern unordered_map<string, double> g_similarity_params;

// 工具函数：解析用户输入特征列表
vector<Feature> parseFeatureList(const vector<string>& input_list);