#define OBX_CPP_FILE
#include "ConceptDatabase.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;

bool ConceptDatabase::initialize(const string& dbPath) {
    try {
        obx::Options options(create_obx_model());
        options.directory(dbPath);
        store = make_unique<obx::Store>(options);
        conceptBox = make_unique<obx::Box<Concept>>(*store);
        return true;
    } catch (const exception& e) {
        cerr << "数据库初始化失败: " << e.what() << endl;
        return false;
    }
}

bool ConceptDatabase::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "错误：无法打开文件 " << filename << endl;
        return false;
    }

    string line;
    int line_number = 0;
    int loaded_count = 0;

    while (getline(file, line)) {
        line_number++;
        if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // 解析格式：ID.[key:value,key:value,...]
        size_t dot_pos = line.find('.');
        if (dot_pos == string::npos) {
            cerr << "警告：第" << line_number << "行格式错误，跳过：" << line << endl;
            continue;
        }

        // 提取ID
        int concept_id;
        try {
            concept_id = stoi(line.substr(0, dot_pos));
        } catch (const invalid_argument& e) {
            cerr << "警告：第" << line_number << "行ID格式错误，跳过：" << line << endl;
            continue;
        }

        // 查找方括号
        size_t bracket_start = line.find('[', dot_pos);
        size_t bracket_end = line.find(']', bracket_start);
        if (bracket_start == string::npos || bracket_end == string::npos) {
            cerr << "警告：第" << line_number << "行缺少方括号，跳过：" << line << endl;
            continue;
        }

        // 提取特征字符串
        string features_str = line.substr(bracket_start + 1, bracket_end - bracket_start - 1);

        // 创建概念对象
        Concept concept;
        concept.id = 0;  // ObjectBox要求新对象使用ID 0，它会自动分配唯一ID

        // 解析特征列表
        if (!features_str.empty()) {
            stringstream ss(features_str);
            string feature_str;

            while (getline(ss, feature_str, ',')) {
                // 去除前后空格
                size_t start = feature_str.find_first_not_of(" \t");
                size_t end = feature_str.find_last_not_of(" \t");

                if (start == string::npos) continue;
                feature_str = feature_str.substr(start, end - start + 1);

                // 解析键值对
                size_t colon_pos = feature_str.find(':');
                if (colon_pos == string::npos) {
                    cerr << "警告：第" << line_number << "行特征格式错误，跳过：" << feature_str << endl;
                    continue;
                }

                string key = feature_str.substr(0, colon_pos);
                string value = feature_str.substr(colon_pos + 1);

                // 去除key和value的空格
                if (!key.empty()) {
                    size_t key_start = key.find_first_not_of(" \t");
                    size_t key_end = key.find_last_not_of(" \t");
                    if (key_start != string::npos) {
                        key = key.substr(key_start, key_end - key_start + 1);
                    }
                }

                if (!value.empty()) {
                    size_t val_start = value.find_first_not_of(" \t");
                    size_t val_end = value.find_last_not_of(" \t");
                    if (val_start != string::npos) {
                        value = value.substr(val_start, val_end - val_start + 1);
                    }
                }

                if (!key.empty() && !value.empty()) {
                    concept.feature_keys.push_back(key);
                    concept.feature_values.push_back(value);
                }
            }
        }

        // 保存到ObjectBox
        if (!concept.feature_keys.empty()) {
            try {
                conceptBox->put(concept);
                loaded_count++;
            } catch (const exception& e) {
                cerr << "警告：保存概念" << concept_id << "失败：" << e.what() << endl;
            }
        }
    }

    file.close();
    cout << "成功从 " << filename << " 加载了 " << loaded_count << " 个概念到数据库" << endl;
    return loaded_count > 0;
}

unique_ptr<Concept> ConceptDatabase::findById(obx_id id) {
    try {
        return conceptBox->get(id);
    } catch (const exception& e) {
        cerr << "查找概念失败: " << e.what() << endl;
        return nullptr;
    }
}

vector<unique_ptr<Concept>> ConceptDatabase::findByValue(const string& value) {
    vector<unique_ptr<Concept>> results;
    try {
        // 获取所有概念并手动筛选（简单实现）
        auto all_concepts = conceptBox->getAll();
        for (auto& concept : all_concepts) {
            for (const auto& feature_value : concept->feature_values) {
                if (feature_value == value) {
                    results.push_back(make_unique<Concept>(*concept));
                    break;
                }
            }
        }
    } catch (const exception& e) {
        cerr << "按值查找失败: " << e.what() << endl;
    }
    return results;
}

vector<unique_ptr<Concept>> ConceptDatabase::findByKeyValue(const string& key, const string& value) {
    vector<unique_ptr<Concept>> results;
    try {
        auto all_concepts = conceptBox->getAll();
        for (auto& concept : all_concepts) {
            for (size_t i = 0; i < concept->feature_keys.size(); i++) {
                if (concept->feature_keys[i] == key && concept->feature_values[i] == value) {
                    results.push_back(make_unique<Concept>(*concept));
                    break;
                }
            }
        }
    } catch (const exception& e) {
        cerr << "按键值对查找失败: " << e.what() << endl;
    }
    return results;
}

vector<unique_ptr<Concept>> ConceptDatabase::getAllConcepts() {
    vector<unique_ptr<Concept>> results;
    try {
        results = conceptBox->getAll();
    } catch (const exception& e) {
        cerr << "获取所有概念失败: " << e.what() << endl;
    }
    return results;
}

void ConceptDatabase::printStatistics() {
    try {
        auto count = conceptBox->count();
        cout << "数据库统计：" << endl;
        cout << "  概念总数: " << count << endl;
    } catch (const exception& e) {
        cerr << "获取统计信息失败: " << e.what() << endl;
    }
}

// 全局pij参数配置
unordered_map<string, double> g_similarity_params = {
    // 等级1 (20%重合度)
    {"p11", 1.0},   // 双方都是20%重合度
    {"p12", 0.9},   // A:20%, B:40%
    {"p13", 0.8},   // A:20%, B:60%
    {"p14", 0.7},   // A:20%, B:80%
    {"p15", 0.6},   // A:20%, B:100%

    // 等级2 (40%重合度)
    {"p21", 0.9},   // A:40%, B:20%
    {"p22", 1.2},   // A:40%, B:40%
    {"p23", 1.1},   // A:40%, B:60%
    {"p24", 1.0},   // A:40%, B:80%
    {"p25", 0.9},   // A:40%, B:100%

    // 等级3 (60%重合度)
    {"p31", 0.8},   // A:60%, B:20%
    {"p32", 1.1},   // A:60%, B:40%
    {"p33", 1.5},   // A:60%, B:60%
    {"p34", 1.4},   // A:60%, B:80%
    {"p35", 1.3},   // A:60%, B:100%

    // 等级4 (80%重合度)
    {"p41", 0.7},   // A:80%, B:20%
    {"p42", 1.0},   // A:80%, B:40%
    {"p43", 1.4},   // A:80%, B:60%
    {"p44", 1.8},   // A:80%, B:80%
    {"p45", 1.7},   // A:80%, B:100%

    // 等级5 (100%重合度)
    {"p51", 0.6},   // A:100%, B:20%
    {"p52", 0.9},   // A:100%, B:40%
    {"p53", 1.3},   // A:100%, B:60%
    {"p54", 1.7},   // A:100%, B:80%
    {"p55", 2.0}    // A:100%, B:100%
};

// Stage 2: 概念匹配和相似度计算功能

MatchResult ConceptDatabase::matchConceptExact(const vector<Feature>& input_features, const unique_ptr<Concept>& concept) {
    MatchResult result;
    result.concept_id = concept->id;
    result.match_count = 0;

    // 遍历每个输入特征
    for (size_t i = 0; i < input_features.size(); i++) {
        const Feature& input_feature = input_features[i];
        bool matched = false;

        if (input_feature.key.empty()) {
            // 模糊匹配：只比较值
            for (const auto& concept_value : concept->feature_values) {
                if (input_feature.value == concept_value) {
                    matched = true;
                    break;
                }
            }
        } else {
            // 精确匹配：需要key和value都匹配
            for (size_t j = 0; j < concept->feature_keys.size(); j++) {
                if (concept->feature_keys[j] == input_feature.key &&
                    concept->feature_values[j] == input_feature.value) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched) {
            result.match_count++;
            result.matched_indices.push_back(i);
        }
    }

    return result;
}

vector<MatchResult> ConceptDatabase::findMatchingConcepts(const vector<Feature>& input_features) {
    vector<MatchResult> results;

    try {
        // 获取所有概念
        auto all_concepts = conceptBox->getAll();

        for (auto& concept : all_concepts) {
            MatchResult match_result = matchConceptExact(input_features, concept);

            // 只保留有匹配的结果
            if (match_result.match_count > 0) {
                results.push_back(match_result);
            }
        }
    } catch (const exception& e) {
        cerr << "查找匹配概念失败: " << e.what() << endl;
    }

    return results;
}

// 工具函数：解析用户输入特征列表
vector<Feature> parseFeatureList(const vector<string>& input_list) {
    vector<Feature> features;

    for (const string& input : input_list) {
        size_t colon_pos = input.find(':');
        if (colon_pos != string::npos) {
            // 有冒号，是键值对，精确匹配
            string key = input.substr(0, colon_pos);
            string value = input.substr(colon_pos + 1);
            features.emplace_back(key, value);
        } else {
            // 无冒号，是纯值，模糊匹配
            features.emplace_back("", input);
        }
    }

    return features;
}

map<pair<int,int>, int> ConceptDatabase::analyzeOverlap(const vector<MatchResult>& matches_A, const vector<MatchResult>& matches_B, int total_features_A, int total_features_B, int& total_matches) {
    map<pair<int,int>, int> overlap_map;
    total_matches = 0;

    // 构建matches_B的概念ID到匹配数的映射，便于快速查找
    unordered_map<obx_id, int> matches_B_map;
    for (const auto& match_B : matches_B) {
        matches_B_map[match_B.concept_id] = match_B.match_count;
    }

    // 遍历matches_A，查找重合的概念
    for (const auto& match_A : matches_A) {
        auto it = matches_B_map.find(match_A.concept_id);
        if (it != matches_B_map.end()) {
            // 找到重合概念
            int match_count_A = match_A.match_count;
            int match_count_B = it->second;

            // 计算重合度等级
            int level_A = calculateMatchLevel(match_count_A, total_features_A);
            int level_B = calculateMatchLevel(match_count_B, total_features_B);

            pair<int,int> level_pair(level_A, level_B);
            overlap_map[level_pair]++;
            total_matches++;
        }
    }

    return overlap_map;
}

double ConceptDatabase::calculatePartialSimilarity(const map<pair<int,int>, int>& overlap_map, int divisor, const unordered_map<string, double>& params) {
    if (divisor == 0) {
        return 0.0;
    }

    double weighted_sum = 0.0;

    for (const auto& entry : overlap_map) {
        int level_A = entry.first.first;
        int level_B = entry.first.second;
        int concept_count = entry.second;

        // 构建参数键名，如"p25"（等级2和等级5）
        string param_key = "p" + to_string(level_A) + to_string(level_B);

        // 查找参数值，如果没有找到则使用默认值1.0
        double param_value = 1.0;
        auto param_it = params.find(param_key);
        if (param_it != params.end()) {
            param_value = param_it->second;
        }

        weighted_sum += concept_count * param_value;
    }

    return weighted_sum / divisor;
}

int ConceptDatabase::calculateMatchLevel(int matched_features, int total_features) {
    if (total_features == 0 || matched_features <= 0) {
        return 1; // 默认最低等级
    }

    // 计算重合度百分比
    double match_percentage = (double)matched_features / total_features * 100.0;

    // 向上取整到最近的20%档次，返回对应等级1-5
    if (match_percentage <= 20.0) return 1;    // 0%-20% → 等级1
    else if (match_percentage <= 40.0) return 2;    // 20%-40% → 等级2
    else if (match_percentage <= 60.0) return 3;    // 40%-60% → 等级3
    else if (match_percentage <= 80.0) return 4;    // 60%-80% → 等级4
    else return 5;    // 80%-100% → 等级5
}

double ConceptDatabase::calculateMainSimilarity(const vector<Feature>& features_A, const vector<Feature>& features_B, const unordered_map<string, double>& params) {
    // 1. 获取两个特征列表的匹配结果
    auto matches_A = findMatchingConcepts(features_A);
    auto matches_B = findMatchingConcepts(features_B);

    // 获取总特征数
    int total_features_A = features_A.size();
    int total_features_B = features_B.size();

    // 2. 分析重合（基于重合度等级）
    int total_matches = 0;
    auto overlap_map = analyzeOverlap(matches_A, matches_B, total_features_A, total_features_B, total_matches);

    // 如果没有重合，相似度为0
    if (total_matches == 0) {
        return 0.0;
    }

    // 获取A和B各自的匹配概念数
    int matches_A_count = matches_A.size();
    int matches_B_count = matches_B.size();

    // 3. 计算A的分相似度（除以A的匹配概念数）
    double partial_similarity_A = calculatePartialSimilarity(overlap_map, matches_A_count, params);

    // 4. 计算B的分相似度（需要交换i,j的视角，除以B的匹配概念数）
    map<pair<int,int>, int> overlap_map_B;
    for (const auto& entry : overlap_map) {
        pair<int,int> swapped_pair(entry.first.second, entry.first.first);  // 交换i,j
        overlap_map_B[swapped_pair] = entry.second;
    }
    double partial_similarity_B = calculatePartialSimilarity(overlap_map_B, matches_B_count, params);

    // 5. 计算主相似度：两个分相似度乘积的平方根（几何平均数）
    return sqrt(partial_similarity_A * partial_similarity_B);
}