//START from here
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <cmath>

#include "/home/laplace/things/ConceptDatabase.hpp"

using namespace std;

// 全局数据库实例
unique_ptr<ConceptDatabase> g_database;

// 解析逗号分隔的输入字符串
vector<string> parseCommaInput(const string& input) {
    vector<string> result;
    stringstream ss(input);
    string item;

    while (getline(ss, item, ',')) {
        // 去除前后空格
        size_t start = item.find_first_not_of(" \t");
        if (start == string::npos) continue; // 空字符串

        size_t end = item.find_last_not_of(" \t");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            result.push_back(item);
        }
    }

    return result;
}

int main()
{
    cout << "Approacher 概念相似度分析器 (ObjectBox版)" << endl;

    // 初始化数据库
    g_database = make_unique<ConceptDatabase>();
    if (!g_database->initialize("/home/laplace/things/concepts-db")) {
        cerr << "数据库初始化失败！" << endl;
        return 1;
    }

    // 加载测试数据
    if (!g_database->loadFromFile("/home/laplace/things/example.txt")) {
        cerr << "加载测试数据失败！" << endl;
        return 1;
    }

    // 交互式计算相似度
    cout << "\n=== 交互式相似度计算 ===" << endl;
    cout << "输入格式: 第一行输入对象A (逗号分隔特征), 第二行输入对象B" << endl;
    cout << "例如: red,apple" << endl;
    cout << "      green,book" << endl;
    cout << "特殊命令:" << endl;
    cout << "  'fuzzy' - 切换模糊匹配模式" << endl;
    cout << "  'params' - 参数学习模式" << endl;
    cout << "  'save' - 保存参数" << endl;
    cout << "  'load' - 加载参数" << endl;
    cout << "  'quit' 或 'exit' - 退出程序" << endl;

    bool use_fuzzy_matching = false;
    double fuzzy_threshold = 0.6;
    int recursive_depth = 2;

    string line_a, line_b;
    while (true) {
        cout << "\n输入对象A: ";
        if (!getline(cin, line_a)) break;

        // 检查特殊命令
        if (line_a == "quit" || line_a == "exit") {
            break;
        } else if (line_a == "fuzzy") {
            use_fuzzy_matching = !use_fuzzy_matching;
            cout << "模糊匹配模式: " << (use_fuzzy_matching ? "开启" : "关闭") << endl;
            if (use_fuzzy_matching) {
                cout << "模糊阈值: " << fuzzy_threshold << ", 递归深度: " << recursive_depth << endl;
            }
            continue;
        } else if (line_a == "params") {
            cout << "进入参数学习模式..." << endl;
            cout << "输入训练样本数量: ";
            string count_str;
            if (getline(cin, count_str)) {
                try {
                    int sample_count = stoi(count_str);
                    for (int i = 0; i < sample_count; i++) {
                        cout << "\n--- 训练样本 " << (i+1) << " ---" << endl;
                        cout << "输入对象A: ";
                        string train_a;
                        if (!getline(cin, train_a)) break;

                        cout << "输入对象B: ";
                        string train_b;
                        if (!getline(cin, train_b)) break;

                        cout << "期望相似度 (0-1): ";
                        string similarity_str;
                        if (!getline(cin, similarity_str)) break;

                        cout << "信心度 (0-1): ";
                        string confidence_str;
                        if (!getline(cin, confidence_str)) break;

                        try {
                            double expected_sim = stod(similarity_str);
                            double confidence = stod(confidence_str);

                            auto features_a = parseFeatureList(parseCommaInput(train_a));
                            auto features_b = parseFeatureList(parseCommaInput(train_b));

                            TrainingSample sample;
                            sample.features_A = features_a;
                            sample.features_B = features_b;
                            sample.expected_similarity = expected_sim;
                            sample.confidence = confidence;

                            g_database->addTrainingSample(sample);
                            cout << "样本已添加" << endl;
                        } catch (const exception& e) {
                            cout << "输入格式错误: " << e.what() << endl;
                        }
                    }

                    cout << "\n开始参数优化..." << endl;
                    g_database->optimizeParameters();
                } catch (const exception& e) {
                    cout << "输入错误: " << e.what() << endl;
                }
            }
            continue;
        } else if (line_a == "save") {
            if (g_database->saveParameters("/home/laplace/things/parameters.txt")) {
                cout << "参数保存成功" << endl;
            }
            continue;
        } else if (line_a == "load") {
            if (g_database->loadParameters("/home/laplace/things/parameters.txt")) {
                cout << "参数加载成功" << endl;
            }
            continue;
        }

        cout << "输入对象B: ";
        if (!getline(cin, line_b)) break;

        // 检查退出命令
        if (line_b == "quit" || line_b == "exit") break;

        // 解析输入
        auto input_a = parseCommaInput(line_a);
        auto input_b = parseCommaInput(line_b);

        if (input_a.empty() || input_b.empty()) {
            cout << "输入不能为空，请重新输入。" << endl;
            continue;
        }

        // 转换为特征列表
        auto features_a = parseFeatureList(input_a);
        auto features_b = parseFeatureList(input_b);

        // 直接使用calculateMainSimilarity函数计算
        double main_similarity = g_database->calculateMainSimilarity(features_a, features_b, g_similarity_params);

        if (main_similarity == 0.0) {
            cout << "无重合概念，相似度为 0" << endl;
            continue;
        }

        // 为了显示分相似度，还是需要手动计算一次
        auto matches_a = g_database->findMatchingConcepts(features_a, use_fuzzy_matching, fuzzy_threshold, recursive_depth);
        auto matches_b = g_database->findMatchingConcepts(features_b, use_fuzzy_matching, fuzzy_threshold, recursive_depth);
        int total_matches = 0;
        auto overlap_map = g_database->analyzeOverlap(matches_a, matches_b, features_a.size(), features_b.size(), total_matches);

        double partial_a_to_b = g_database->calculatePartialSimilarity(overlap_map, matches_a.size(), g_similarity_params);
        map<pair<int,int>, int> overlap_map_b;
        for (const auto& entry : overlap_map) {
            pair<int,int> swapped_pair(entry.first.second, entry.first.first);
            overlap_map_b[swapped_pair] = entry.second;
        }
        double partial_b_to_a = g_database->calculatePartialSimilarity(overlap_map_b, matches_b.size(), g_similarity_params);

        // 构建显示字符串
        string display_a = "[";
        for (size_t i = 0; i < input_a.size(); i++) {
            if (i > 0) display_a += ",";
            display_a += input_a[i];
        }
        display_a += "]";

        string display_b = "[";
        for (size_t i = 0; i < input_b.size(); i++) {
            if (i > 0) display_b += ",";
            display_b += input_b[i];
        }
        display_b += "]";

        // 输出结果
        cout << "\n=== 计算结果 ===" << endl;
        cout << "匹配模式: " << (use_fuzzy_matching ? "模糊匹配" : "精确匹配") << endl;
        if (use_fuzzy_matching) {
            cout << "模糊阈值: " << fuzzy_threshold << ", 递归深度: " << recursive_depth << endl;
        }
        cout << "匹配概念数 - A: " << matches_a.size() << ", B: " << matches_b.size() << ", 重合: " << total_matches << endl;
        cout << display_a << "->" << display_b << " : " << partial_a_to_b << endl;
        cout << display_a << "<-" << display_b << " : " << partial_b_to_a << endl;
        cout << display_a << "<->" << display_b << " : " << main_similarity << endl;
    }

    cout << "程序结束。" << endl;


    return 0;
}