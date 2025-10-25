#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <fstream>

#include "/home/laplace/things/ConceptDatabase.hpp"

using namespace std;

// 函数前向声明
vector<string> parseCommaInput(const string& input);
void runSemanticApproacher();
bool isEqualsKeyValuePair(const string& input);
string extractKeyFromEqualsKeyValue(const string& input);
string convertEqualsToColonKeyValue(const string& input);
double handleEqualsKeyValueSpecialCases(const string& input_a, const string& input_b);
void preprocessEqualsKeyValuePairs(const string& input_a, const string& input_b, string& processed_a, string& processed_b);
string applyBackupPOSRules(const string& word);

// 全局数据库实例
unique_ptr<ConceptDatabase> g_semantic_database;

// 全局语义分析结果存储
struct SemanticAnalysisResult {
    string input_a;
    string input_b;
    double containment_strength_a_to_b = 0.0;  // A包含B的强度
    double containment_strength_b_to_a = 0.0;  // B包含A的强度
    bool has_semantic_enhancement = false;
} g_semantic_result;

// =============================================================================
// 语义包含匹配功能 (复合词形容词+名词包含关系检测)
// =============================================================================

/**
 * 从数据库查询词性信息（带备用规则）
 * 通过查询概念数据库中的word_class字段获取词性，如果失败则使用备用规则
 * @param word 待识别的单词
 * @return "adj" 形容词, "noun" 名词, "unknown" 未知
 */
string identifyPartOfSpeech(const string& word) {
    if (!g_semantic_database) {
        return applyBackupPOSRules(word);
    }

    try {
        // 获取所有概念
        auto all_concepts = g_semantic_database->getAllConcepts();

        // 查找匹配的概念
        for (auto& concept : all_concepts) {
            for (size_t i = 0; i < concept->feature_keys.size(); i++) {
                // 查找name字段匹配当前单词的概念
                if (concept->feature_keys[i] == "name" && concept->feature_values[i] == word) {
                    // 在同一个概念中查找word_class字段
                    for (size_t j = 0; j < concept->feature_keys.size(); j++) {
                        if (concept->feature_keys[j] == "word_class") {
                            string word_class = concept->feature_values[j];

                            // 转换词性标记
                            if (word_class == "adjective") {
                                cout << "[词性查询] \"" << word << "\" → 形容词 (数据库)" << endl;
                                return "adj";
                            } else if (word_class == "noun") {
                                cout << "[词性查询] \"" << word << "\" → 名词 (数据库)" << endl;
                                return "noun";
                            }
                        }
                    }
                }
            }
        }

        // 数据库中没有找到，使用备用规则
        return applyBackupPOSRules(word);

    } catch (const exception& e) {
        cerr << "[词性查询] 查询失败: " << e.what() << endl;
        return applyBackupPOSRules(word);
    }
}

/**
 * 备用词性识别规则（当数据库查询失败时使用）
 * @param word 待识别的单词
 * @return "adj" 形容词, "noun" 名词, "unknown" 未知
 */
string applyBackupPOSRules(const string& word) {
    // 中文形容词常见特征
    if (word.find("的") != string::npos && word.length() > 3) {
        cout << "[词性查询] \"" << word << "\" → 形容词 (备用规则: 含'的')" << endl;
        return "adj";
    }

    // 中文名词常见特征
    vector<string> noun_suffixes = {"人", "者", "生", "师", "员", "家", "手", "工"};
    for (const string& suffix : noun_suffixes) {
        if (word.length() >= suffix.length() &&
            word.substr(word.length() - suffix.length()) == suffix) {
            cout << "[词性查询] \"" << word << "\" → 名词 (备用规则: 后缀'" << suffix << "')" << endl;
            return "noun";
        }
    }

    // 预定义词汇
    vector<string> known_nouns = {"女孩", "学生", "老师", "汽车", "轿车", "苹果", "书", "电脑", "手机"};
    vector<string> known_adjs = {"美丽", "温柔", "聪明", "勤奋", "快速", "红色", "新的", "优秀", "漂亮", "可爱"};

    for (const string& noun : known_nouns) {
        if (word == noun) {
            cout << "[词性查询] \"" << word << "\" → 名词 (备用规则: 预定义)" << endl;
            return "noun";
        }
    }

    for (const string& adj : known_adjs) {
        if (word == adj) {
            cout << "[词性查询] \"" << word << "\" → 形容词 (备用规则: 预定义)" << endl;
            return "adj";
        }
    }

    cout << "[词性查询] \"" << word << "\" → 未知 (所有规则都无法识别)" << endl;
    return "unknown";
}

/**
 * 复合词分割函数
 * 将下划线分隔的复合词分割成单词列表
 * @param compound_word 复合词，如 "sweet_red_apple"
 * @return 单词列表，如 ["sweet", "red", "apple"]
 */
vector<string> splitCompoundWord(const string& compound_word) {
    vector<string> words;
    stringstream ss(compound_word);
    string word;

    while (getline(ss, word, '_')) {
        // 去除空格
        word.erase(remove_if(word.begin(), word.end(), ::isspace), word.end());
        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

/**
 * 名词段结构体 - 表示一个名词及其前面的形容词
 */
struct NounSegment {
    string noun;                    // 名词
    vector<string> adjectives;      // 属于该名词的形容词列表
};

/**
 * 解析逗号分隔序列为名词段列表
 * 规则：每个名词前面到上一个名词为止的形容词都属于这个名词
 * @param sequence 逗号分隔的序列 (如: "美丽,温柔,女孩,聪明,勤奋,学生")
 * @return 名词段列表
 */
vector<NounSegment> parseSequenceToNounSegments(const string& sequence) {
    vector<NounSegment> segments;
    vector<string> words = parseCommaInput(sequence);

    vector<string> current_adjectives;

    for (const string& word : words) {
        string pos = identifyPartOfSpeech(word);

        if (pos == "noun") {
            // 遇到名词，创建新的名词段
            NounSegment segment;
            segment.noun = word;
            segment.adjectives = current_adjectives;  // 前面累积的形容词都属于这个名词
            segments.push_back(segment);

            // 清空形容词列表，为下一个名词准备
            current_adjectives.clear();
        } else if (pos == "adj") {
            // 遇到形容词，加入当前形容词列表
            current_adjectives.push_back(word);
        } else {
            // 未知词性，当作形容词处理
            current_adjectives.push_back(word);
        }
    }

    return segments;
}

/**
 * 检查形容词列表的包含关系（子集关系，忽略顺序）
 * @param container_adjs 容器的形容词列表
 * @param contained_adjs 被包含的形容词列表
 * @return true 如果contained_adjs是container_adjs的子集
 */
bool checkAdjectiveSubset(const vector<string>& container_adjs, const vector<string>& contained_adjs) {
    // 将形容词转换为小写，但不排序（保持原始逻辑）
    vector<string> container_lower;
    vector<string> contained_lower;

    for (const string& adj : container_adjs) {
        string adj_lower = adj;
        transform(adj_lower.begin(), adj_lower.end(), adj_lower.begin(), ::tolower);
        container_lower.push_back(adj_lower);
    }

    for (const string& adj : contained_adjs) {
        string adj_lower = adj;
        transform(adj_lower.begin(), adj_lower.end(), adj_lower.begin(), ::tolower);
        contained_lower.push_back(adj_lower);
    }

    // 检查contained_lower中的每个形容词是否都在container_lower中
    for (const string& contained_adj : contained_lower) {
        bool found = false;
        for (const string& container_adj : container_lower) {
            if (container_adj == contained_adj) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

/**
 * 检查两个名词段列表的包含关系
 * @param container_segments 容器的名词段列表
 * @param contained_segments 被包含的名词段列表
 * @return 包含关系强度 (0.0: 无关系, 4.0: 强包含关系)
 */
double checkSequenceContainment(const vector<NounSegment>& container_segments,
                               const vector<NounSegment>& contained_segments) {
    // 规则1: 名词数量必须相同
    if (container_segments.size() != contained_segments.size()) {
        return 0.0;
    }

    // 规则2: 名词序列必须完全相同且顺序相同
    for (size_t i = 0; i < container_segments.size(); i++) {
        string noun1_lower = container_segments[i].noun;
        string noun2_lower = contained_segments[i].noun;
        transform(noun1_lower.begin(), noun1_lower.end(), noun1_lower.begin(), ::tolower);
        transform(noun2_lower.begin(), noun2_lower.end(), noun2_lower.begin(), ::tolower);

        if (noun1_lower != noun2_lower) {
            return 0.0;
        }
    }

    // 规则3: 对于每个名词，被包含方的形容词必须是包含方形容词的子集
    for (size_t i = 0; i < container_segments.size(); i++) {
        if (!checkAdjectiveSubset(container_segments[i].adjectives,
                                 contained_segments[i].adjectives)) {
            return 0.0;
        }
    }

    // 计算包含强度
    double containment_strength = 4.0;  // 基础包含强度

    // 计算形容词删除程度，删除越多强度越低
    int total_container_adjs = 0;
    int total_contained_adjs = 0;

    for (size_t i = 0; i < container_segments.size(); i++) {
        total_container_adjs += container_segments[i].adjectives.size();
        total_contained_adjs += contained_segments[i].adjectives.size();
    }

    if (total_container_adjs > 0) {
        double retention_ratio = (double)total_contained_adjs / total_container_adjs;
        if (retention_ratio < 0.3) {
            containment_strength = 3.5;  // 删除过多，降低强度
        }
    }

    return containment_strength;
}

/**
 * 语义包含关系检测主函数
 * @param sequence1 第一个逗号分隔序列 (潜在的容器)
 * @param sequence2 第二个逗号分隔序列 (潜在的被包含者)
 * @return 包含关系强度 (0.0: 无关系, 4.0: 强包含关系)
 */
double detectSemanticContainment(const string& sequence1, const string& sequence2) {
    cout << "[语义分析] 检测包含关系: \"" << sequence1 << "\" vs \"" << sequence2 << "\"" << endl;

    // 解析两个序列为名词段
    vector<NounSegment> segments1 = parseSequenceToNounSegments(sequence1);
    vector<NounSegment> segments2 = parseSequenceToNounSegments(sequence2);

    cout << "[语义分析] 序列1解析结果:" << endl;
    for (size_t i = 0; i < segments1.size(); i++) {
        cout << "  名词: " << segments1[i].noun << ", 形容词: [";
        for (size_t j = 0; j < segments1[i].adjectives.size(); j++) {
            cout << segments1[i].adjectives[j];
            if (j < segments1[i].adjectives.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }

    cout << "[语义分析] 序列2解析结果:" << endl;
    for (size_t i = 0; i < segments2.size(); i++) {
        cout << "  名词: " << segments2[i].noun << ", 形容词: [";
        for (size_t j = 0; j < segments2[i].adjectives.size(); j++) {
            cout << segments2[i].adjectives[j];
            if (j < segments2[i].adjectives.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }

    // 检查包含关系
    double containment_strength = checkSequenceContainment(segments1, segments2);

    if (containment_strength > 0.0) {
        cout << "[语义分析] 检测到包含关系！强度: " << containment_strength << endl;
    } else {
        cout << "[语义分析] 未检测到包含关系" << endl;
    }

    return containment_strength;
}

/**
 * 分析两个输入序列之间的语义包含关系
 * @param input_a 输入A（逗号分隔的序列）
 * @param input_b 输入B（逗号分隔的序列）
 * @return 无返回值，结果保存在g_semantic_result中
 */
void analyzeSemanticRelationship(const string& input_a, const string& input_b) {
    cout << "\n=== 语义关系分析 ===" << endl;

    // 清空之前的结果
    g_semantic_result = SemanticAnalysisResult();
    g_semantic_result.input_a = input_a;
    g_semantic_result.input_b = input_b;

    cout << "[语义分析] 分析序列包含关系..." << endl;
    cout << "[语义分析] 序列A: \"" << input_a << "\"" << endl;
    cout << "[语义分析] 序列B: \"" << input_b << "\"" << endl;

    // 直接检查两个完整序列的包含关系
    double containment_a_to_b = detectSemanticContainment(input_a, input_b);
    double containment_b_to_a = detectSemanticContainment(input_b, input_a);

    // 保存结果
    g_semantic_result.containment_strength_a_to_b = containment_a_to_b;
    g_semantic_result.containment_strength_b_to_a = containment_b_to_a;

    if (containment_a_to_b > 0.0 || containment_b_to_a > 0.0) {
        g_semantic_result.has_semantic_enhancement = true;
        cout << "\n[语义分析结果] 发现语义包含关系：" << endl;
        if (containment_a_to_b > 0.0) {
            cout << "  A包含B的强度: " << containment_a_to_b << endl;
        }
        if (containment_b_to_a > 0.0) {
            cout << "  B包含A的强度: " << containment_b_to_a << endl;
        }
    } else {
        cout << "\n[语义分析结果] 未发现语义包含关系" << endl;
    }
}

// =============================================================================
// 语义分析和预处理函数 (框架，逻辑待实现)
// =============================================================================

/**
 * 输入预处理函数
 * 在发送给approacher之前对输入进行语义分析和处理
 * @param input 原始输入字符串
 * @return 处理后的输入字符串
 */
string preprocessInput(const string& input) {
    // TODO: 在这里实现语义预处理逻辑
    // 例如：
    // 1. 分析复合词：good_content → good + content
    // 2. 检测包含关系
    // 3. 扩展同义词
    // 4. 语义增强

    cout << "[语义预处理] 输入: " << input << endl;

    // 当前直接返回原输入，后续可在此添加逻辑
    string processed = input;

    cout << "[语义预处理] 处理后: " << processed << endl;
    return processed;
}

/**
 * 输出后处理函数
 * 对approacher的输出进行后处理和增强
 * @param approacher_output approacher程序的原始输出
 * @param original_input_a 用户输入的原始对象A
 * @param original_input_b 用户输入的原始对象B
 * @return 增强后的输出
 */
string postprocessOutput(const string& approacher_output, const string& original_input_a, const string& original_input_b) {
    cout << "[语义后处理] 分析Approacher输出并应用语义增强..." << endl;

    string enhanced_output = approacher_output;

    // 如果检测到语义包含关系，需要增强相似度
    if (g_semantic_result.has_semantic_enhancement) {
        cout << "[语义后处理] 检测到语义包含关系，应用相似度增强" << endl;

        // 解析approacher输出中的相似度百分比
        stringstream ss(approacher_output);
        string line;
        string enhanced_lines;

        while (getline(ss, line)) {
            // 查找包含相似度的行（格式如 "[apple,red]->[apple] : 1.217"）
            size_t colon_pos = line.find(" : ");
            if (colon_pos != string::npos) {
                // 提取冒号后的数字
                string score_part = line.substr(colon_pos + 3);

                // 去除空格并提取数字
                size_t score_start = score_part.find_first_not_of(" \t");
                if (score_start != string::npos) {
                    size_t score_end = score_part.find_first_of(" \t\n", score_start);
                    if (score_end == string::npos) score_end = score_part.length();

                    string score_str = score_part.substr(score_start, score_end - score_start);

                    try {
                        double original_score = stod(score_str);

                        // 应用语义增强
                        double enhancement_factor = max(g_semantic_result.containment_strength_a_to_b,
                                                       g_semantic_result.containment_strength_b_to_a);
                        double enhanced_score = original_score * enhancement_factor;

                        // 替换原分数
                        string enhanced_line = line.substr(0, colon_pos + 3) + to_string(enhanced_score);

                        enhanced_lines += enhanced_line + "\n";

                        cout << "[语义后处理] 相似度增强: " << original_score << " → "
                             << enhanced_score << " (增强系数: " << enhancement_factor << ")" << endl;
                    } catch (const exception& e) {
                        // 如果解析失败，保持原行
                        enhanced_lines += line + "\n";
                    }
                } else {
                    enhanced_lines += line + "\n";
                }
            } else {
                enhanced_lines += line + "\n";
            }
        }

        enhanced_output = enhanced_lines;
    }

    // 添加语义分析报告
    enhanced_output += "\n=== 语义分析报告 ===\n";

    if (g_semantic_result.has_semantic_enhancement) {
        enhanced_output += "[语义包含关系] 检测成功\n";
        if (g_semantic_result.containment_strength_a_to_b > 0.0) {
            enhanced_output += "  A包含B (强度: " + to_string(g_semantic_result.containment_strength_a_to_b) + ")\n";
        }
        if (g_semantic_result.containment_strength_b_to_a > 0.0) {
            enhanced_output += "  B包含A (强度: " + to_string(g_semantic_result.containment_strength_b_to_a) + ")\n";
        }
        enhanced_output += "  相似度已按包含关系进行增强\n";
    } else {
        enhanced_output += "[语义包含关系] 未检测到\n";
        enhanced_output += "  使用原始Approacher相似度结果\n";
    }

    return enhanced_output;
}

/**
 * 调用approacher程序并获取输出
 * @param input_a 处理后的输入A
 * @param input_b 处理后的输入B
 * @return approacher的输出结果
 */
string callApproacher(const string& input_a, const string& input_b) {
    // 创建临时输入文件
    string temp_input_file = "/tmp/approacher_input.txt";
    ofstream temp_file(temp_input_file);
    temp_file << input_a << "\n" << input_b << "\nquit\n";
    temp_file.close();

    // 调用approacher程序
    string command = "cd /home/laplace/approacher && ";
    command += "export LD_LIBRARY_PATH='/home/laplace/things/lib:$LD_LIBRARY_PATH' && ";
    command += "./approacher < " + temp_input_file + " 2>&1";

    // 执行命令并获取输出
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "错误：无法调用approacher程序";
    }

    string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    // 清理临时文件
    remove(temp_input_file.c_str());

    return result;
}

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

/**
 * 运行语义增强分析器的主要功能
 * 处理用户交互、语义分析和相似度计算
 */
void runSemanticApproacher() {
    // 交互式计算相似度
    cout << "\n=== 语义增强相似度计算 ===" << endl;
    cout << "输入格式: 第一行输入对象A (逗号分隔特征), 第二行输入对象B" << endl;
    cout << "例如: good_content,red" << endl;
    cout << "      content,apple" << endl;
    cout << "特殊命令:" << endl;
    cout << "  'direct' - 直接调用原approacher（跳过语义处理）" << endl;
    cout << "  'quit' 或 'exit' - 退出程序" << endl;

    bool direct_mode = false;
    string line_a, line_b;

    while (true) {
        cout << "\n输入对象A: ";
        if (!getline(cin, line_a)) break;

        // 检查特殊命令
        if (line_a == "quit" || line_a == "exit") {
            break;
        } else if (line_a == "direct") {
            direct_mode = !direct_mode;
            cout << "直接模式: " << (direct_mode ? "开启（跳过语义处理）" : "关闭（使用语义处理）") << endl;
            continue;
        }

        cout << "输入对象B: ";
        if (!getline(cin, line_b)) break;

        // 检查退出命令
        if (line_b == "quit" || line_b == "exit") break;

        if (line_a.empty() || line_b.empty()) {
            cout << "输入不能为空，请重新输入。" << endl;
            continue;
        }

        // 保存原始输入
        string original_a = line_a;
        string original_b = line_b;

        // 首先检查等号键值对的特殊情况
        cout << "\n=== 等号键值对检查阶段 ===" << endl;
        double special_case_result = handleEqualsKeyValueSpecialCases(line_a, line_b);

        if (special_case_result >= 0) {
            // 是特殊情况，直接输出结果
            cout << "\n" << string(50, '=') << endl;
            cout << "最终结果: 等号键值对特殊匹配" << endl;
            cout << "相似度: " << special_case_result << endl;
            cout << "说明: 等号键值对特殊处理 - 固定相似度匹配" << endl;
            cout << string(50, '=') << endl;
            continue;
        }

        // 不是特殊情况，进行等号键值对预处理（转换为冒号格式）
        string equals_processed_a, equals_processed_b;
        preprocessEqualsKeyValuePairs(line_a, line_b, equals_processed_a, equals_processed_b);

        // 更新输入为预处理后的结果
        line_a = equals_processed_a;
        line_b = equals_processed_b;

        // 根据模式选择处理方式
        string processed_a, processed_b;

        if (direct_mode) {
            // 直接模式：跳过语义处理
            processed_a = line_a;
            processed_b = line_b;
            cout << "\n[直接模式] 跳过语义预处理" << endl;
        } else {
            // 语义模式：进行语义关系分析
            cout << "\n=== 语义分析阶段 ===" << endl;
            analyzeSemanticRelationship(line_a, line_b);

            // 预处理输入（当前直接返回原输入，语义增强在后处理阶段应用）
            processed_a = preprocessInput(line_a);
            processed_b = preprocessInput(line_b);
        }

        // 调用approacher程序
        cout << "\n=== 调用Approacher计算 ===" << endl;
        string approacher_result = callApproacher(processed_a, processed_b);

        // 后处理输出
        string final_output;
        if (direct_mode) {
            final_output = approacher_result;
        } else {
            cout << "\n=== 语义后处理阶段 ===" << endl;
            final_output = postprocessOutput(approacher_result, original_a, original_b);
        }

        // 显示最终结果
        cout << "\n" << string(50, '=') << endl;
        cout << "最终结果:" << endl;
        cout << final_output << endl;
        cout << string(50, '=') << endl;
    }
}

// =============================================================================
// 等号键值对特殊处理功能
// =============================================================================

/**
 * 检测字符串是否为等号键值对格式
 * @param input 输入字符串
 * @return true 如果是等号键值对格式 (key=value)
 */
bool isEqualsKeyValuePair(const string& input) {
    size_t equals_pos = input.find('=');
    return (equals_pos != string::npos && equals_pos > 0 && equals_pos < input.length() - 1);
}

/**
 * 从等号键值对中提取键名
 * @param input 等号键值对字符串 (key=value)
 * @return 键名部分，如果不是有效格式则返回空字符串
 */
string extractKeyFromEqualsKeyValue(const string& input) {
    if (!isEqualsKeyValuePair(input)) {
        return "";
    }

    size_t equals_pos = input.find('=');
    string key = input.substr(0, equals_pos);

    // 去除前后空格
    size_t start = key.find_first_not_of(" \t");
    if (start == string::npos) return "";

    size_t end = key.find_last_not_of(" \t");
    return key.substr(start, end - start + 1);
}

/**
 * 将等号键值对转换为冒号键值对格式
 * @param input 等号键值对字符串 (key=value)
 * @return 冒号键值对字符串 (key:value)，如果不是有效格式则返回原字符串
 */
string convertEqualsToColonKeyValue(const string& input) {
    if (!isEqualsKeyValuePair(input)) {
        return input;
    }

    size_t equals_pos = input.find('=');
    string key = input.substr(0, equals_pos);
    string value = input.substr(equals_pos + 1);

    // 去除键和值的前后空格
    size_t key_start = key.find_first_not_of(" \t");
    if (key_start != string::npos) {
        size_t key_end = key.find_last_not_of(" \t");
        key = key.substr(key_start, key_end - key_start + 1);
    }

    size_t value_start = value.find_first_not_of(" \t");
    if (value_start != string::npos) {
        size_t value_end = value.find_last_not_of(" \t");
        value = value.substr(value_start, value_end - value_start + 1);
    }

    return key + ":" + value;
}

/**
 * 处理等号键值对的特殊情况
 * @param input_a 输入A
 * @param input_b 输入B
 * @return 如果是特殊情况返回固定相似度100，否则返回-1表示需要正常处理
 */
double handleEqualsKeyValueSpecialCases(const string& input_a, const string& input_b) {
    bool a_is_equals_kv = isEqualsKeyValuePair(input_a);
    bool b_is_equals_kv = isEqualsKeyValuePair(input_b);

    // 特殊情况1: 两个完全相同的等号键值对
    if (a_is_equals_kv && b_is_equals_kv && input_a == input_b) {
        cout << "[等号键值对] 检测到完全相同的键值对，返回固定相似度100" << endl;
        return 100.0;
    }

    // 特殊情况2: 等号键值对 vs 对应的键
    if (a_is_equals_kv && !b_is_equals_kv) {
        string key_a = extractKeyFromEqualsKeyValue(input_a);
        if (!key_a.empty() && key_a == input_b) {
            cout << "[等号键值对] 检测到键值对与对应键的匹配: \"" << input_a << "\" vs \"" << input_b << "\"，返回固定相似度100" << endl;
            return 100.0;
        }
    }

    // 特殊情况3: 键 vs 对应的等号键值对
    if (!a_is_equals_kv && b_is_equals_kv) {
        string key_b = extractKeyFromEqualsKeyValue(input_b);
        if (!key_b.empty() && input_a == key_b) {
            cout << "[等号键值对] 检测到键与对应键值对的匹配: \"" << input_a << "\" vs \"" << input_b << "\"，返回固定相似度100" << endl;
            return 100.0;
        }
    }

    // 不是特殊情况，需要正常处理
    return -1.0;
}

/**
 * 对输入进行等号键值对预处理
 * 将等号键值对转换为冒号键值对，以便后续使用原有逻辑处理
 * @param input_a 输入A
 * @param input_b 输入B
 * @param processed_a 处理后的输入A
 * @param processed_b 处理后的输入B
 */
void preprocessEqualsKeyValuePairs(const string& input_a, const string& input_b,
                                  string& processed_a, string& processed_b) {
    processed_a = convertEqualsToColonKeyValue(input_a);
    processed_b = convertEqualsToColonKeyValue(input_b);

    if (processed_a != input_a) {
        cout << "[等号键值对] 转换: \"" << input_a << "\" → \"" << processed_a << "\"" << endl;
    }
    if (processed_b != input_b) {
        cout << "[等号键值对] 转换: \"" << input_b << "\" → \"" << processed_b << "\"" << endl;
    }
}

int main()
{
    cout << "Semantic Approacher 语义增强概念相似度分析器" << endl;
    cout << "基于Approacher，增加语义分析层和等号键值对特殊处理" << endl;

    // 初始化数据库连接用于语义分析
    g_semantic_database = make_unique<ConceptDatabase>();
    if (!g_semantic_database->initialize("/home/laplace/things/concepts-db")) {
        cerr << "警告：无法连接到语义分析数据库，部分功能可能受限" << endl;
    } else {
        cout << "语义分析数据库连接成功" << endl;
        g_semantic_database->printStatistics();
    }

    // 运行语义分析器（包含等号键值对特殊处理功能）
    // 等号键值对特殊处理：
    // 1. key=value vs key → 固定相似度100
    // 2. key=value vs key=value → 固定相似度100
    // 3. 其他情况：转换为key:value格式后按原有逻辑处理
    runSemanticApproacher();

    cout << "Semantic Approacher 程序结束。" << endl;
    return 0;
}