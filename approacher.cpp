//START from here
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "ConceptDatabase.hpp"

using namespace std;

// 全局数据库实例
unique_ptr<ConceptDatabase> g_database;

int main()
{
    cout << "Approacher 概念相似度分析器 (ObjectBox版)" << endl;

    // 初始化数据库
    g_database = make_unique<ConceptDatabase>();
    if (!g_database->initialize()) {
        cerr << "数据库初始化失败！" << endl;
        return 1;
    }

    // 加载测试数据
    if (!g_database->loadFromFile("example.txt")) {
        cerr << "加载测试数据失败！" << endl;
        return 1;
    }

    // 显示统计信息
    g_database->printStatistics();

    // 详细数据验证
    cout << "\n=== 详细数据验证 ===" << endl;

    // 首先查看所有数据库中的概念
    cout << "数据库中所有概念的完整信息：" << endl;
    auto all_concepts = g_database->getAllConcepts();
    for (const auto& concept : all_concepts) {
        cout << "  概念ID " << concept->id << " (特征数量: " << concept->feature_keys.size() << ")" << endl;
        cout << "    特征键: [";
        for (size_t i = 0; i < concept->feature_keys.size(); i++) {
            if (i > 0) cout << ", ";
            cout << "\"" << concept->feature_keys[i] << "\"";
        }
        cout << "]" << endl;
        cout << "    特征值: [";
        for (size_t i = 0; i < concept->feature_values.size(); i++) {
            if (i > 0) cout << ", ";
            cout << "\"" << concept->feature_values[i] << "\"";
        }
        cout << "]" << endl;
        cout << "    完整特征: ";
        for (size_t i = 0; i < concept->feature_keys.size(); i++) {
            if (i > 0) cout << ", ";
            cout << concept->feature_keys[i] << ":" << concept->feature_values[i];
        }
        cout << endl << endl;
    }

    // 测试按值查询
    cout << "=== 查询测试 ===" << endl;
    cout << "查找值为 'red' 的概念：" << endl;
    auto red_concepts = g_database->findByValue("red");
    cout << "  找到 " << red_concepts.size() << " 个匹配概念：" << endl;
    for (const auto& concept : red_concepts) {
        cout << "    概念ID " << concept->id << ": ";
        for (size_t i = 0; i < concept->feature_keys.size(); i++) {
            if (i > 0) cout << ", ";
            cout << concept->feature_keys[i] << ":" << concept->feature_values[i];
        }
        cout << endl;
    }

    // 测试按键值对查询
    cout << "\n查找 name:apple 的概念：" << endl;
    auto apple_concepts = g_database->findByKeyValue("name", "apple");
    cout << "  找到 " << apple_concepts.size() << " 个匹配概念：" << endl;
    for (const auto& concept : apple_concepts) {
        cout << "    概念ID " << concept->id << ": ";
        for (size_t i = 0; i < concept->feature_keys.size(); i++) {
            if (i > 0) cout << ", ";
            cout << concept->feature_keys[i] << ":" << concept->feature_values[i];
        }
        cout << endl;
    }

    // 测试用户输入解析
    cout << "\n=== 测试输入解析 ===" << endl;
    vector<string> test_input = {"red", "name:apple"};
    auto parsed_features = parseFeatureList(test_input);
    cout << "输入 [\"red\", \"name:apple\"] 解析结果：" << endl;
    for (const auto& feature : parsed_features) {
        cout << "  key='" << feature.key << "', value='" << feature.value << "'" << endl;
    }

    // 测试新实现的匹配函数
    cout << "\n=== 测试概念匹配函数 ===" << endl;

    // 测试特征列表A: ["red", "name:apple"]
    cout << "测试特征列表A: [\"red\", \"name:apple\"]" << endl;
    auto matches_A = g_database->findMatchingConcepts(parsed_features);
    cout << "找到 " << matches_A.size() << " 个匹配概念：" << endl;
    for (const auto& match : matches_A) {
        cout << "  概念ID " << match.concept_id << ": 匹配数=" << match.match_count
             << ", 匹配索引=[";
        for (size_t i = 0; i < match.matched_indices.size(); i++) {
            if (i > 0) cout << ",";
            cout << match.matched_indices[i];
        }
        cout << "]" << endl;
    }

    // 测试特征列表B: ["red"]
    cout << "\n测试特征列表B: [\"red\"]" << endl;
    vector<string> test_input_B = {"red"};
    auto parsed_features_B = parseFeatureList(test_input_B);
    auto matches_B = g_database->findMatchingConcepts(parsed_features_B);
    cout << "找到 " << matches_B.size() << " 个匹配概念：" << endl;
    for (const auto& match : matches_B) {
        cout << "  概念ID " << match.concept_id << ": 匹配数=" << match.match_count
             << ", 匹配索引=[";
        for (size_t i = 0; i < match.matched_indices.size(); i++) {
            if (i > 0) cout << ",";
            cout << match.matched_indices[i];
        }
        cout << "]" << endl;
    }

    return 0;
}