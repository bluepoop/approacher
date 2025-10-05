#define OBX_CPP_FILE
#include "ConceptDatabase.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

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