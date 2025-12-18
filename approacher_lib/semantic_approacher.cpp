#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <fstream>

#include "things/ConceptDatabase.hpp"

using namespace std;

// 函数前向声明
vector<string> parseCommaInput(const string& input);
void runSemanticApproacher();
bool isEqualsKeyValuePair(const string& input);
string extractKeyFromEqualsKeyValue(const string& input);
string convertEqualsToColonKeyValue(const string& input);
double handleEqualsKeyValueSpecialCases(const string& input_a, const string& input_b);
void preprocessEqualsKeyValuePairs(const string& input_a, const string& input_b, string& processed_a, string& processed_b);
string applyBackupPOSRules(const string& word, bool quiet_mode = false);

// 全局数据库实例
unique_ptr<ConceptDatabase> g_semantic_database;

// 三分数结构体 - 存储主相似度和两个方向的分相似度
struct ThreeScoreResult {
    double main_similarity;     // 语义增强后的主相似度
    double partial_a_to_b;      // A→B方向分相似度
    double partial_b_to_a;      // B→A方向分相似度

    ThreeScoreResult(double main = -2.0, double a_to_b = -2.0, double b_to_a = -2.0)
        : main_similarity(main), partial_a_to_b(a_to_b), partial_b_to_a(b_to_a) {}
};

// 全局语义分析结果存储
struct SemanticAnalysisResult {
    string input_a;
    string input_b;
    double containment_strength_a_to_b = 0.0;  // A包含B的强度
    double containment_strength_b_to_a = 0.0;  // B包含A的强度
    bool has_semantic_enhancement = false;
} g_semantic_result;

// 特征贡献度信息结构体
struct FeatureContribution {
    string feature;           // 特征名
    double contribution_percent;  // 贡献百分比

    FeatureContribution(const string& feat = "", double contrib = 0.0)
        : feature(feat), contribution_percent(contrib) {}
};

// 完整信息结果结构体
struct FullInfoResult {
    double forward_similarity;      // 正向相似度 (A→B)
    double reverse_similarity;      // 反向相似度 (B→A)
    double main_similarity;         // 主相似度 (总相似度)
    vector<FeatureContribution> feature_contributions;  // 每个特征的贡献度

    // 错误检查
    bool has_error() const {
        return main_similarity == -2.0;
    }

    bool no_match() const {
        return main_similarity == -1.0;
    }

    FullInfoResult(double forward = -2.0, double reverse = -2.0, double main = -2.0)
        : forward_similarity(forward), reverse_similarity(reverse), main_similarity(main) {}
};

// =============================================================================
// 语义包含匹配功能 (复合词形容词+名词包含关系检测)
// =============================================================================

/**
 * Query part-of-speech information from database (with backup rules)
 * Get POS by querying word_class field in concept database, fallback to backup rules if failed
 * @param word The word to identify
 * @param quiet_mode Whether to suppress debug output
 * @return "adj" for adjective, "noun" for noun, "unknown" for unknown
 */
string identifyPartOfSpeech(const string& word, bool quiet_mode = false) {
    if (!g_semantic_database) {
        return applyBackupPOSRules(word, quiet_mode);
    }

    try {
        // Get all concepts
        auto all_concepts = g_semantic_database->getAllConcepts();

        // Search for matching concepts
        for (auto& concept : all_concepts) {
            for (size_t i = 0; i < concept->feature_keys.size(); i++) {
                // Find concept with name field matching current word
                if (concept->feature_keys[i] == "name" && concept->feature_values[i] == word) {
                    // Search for word_class field in the same concept
                    for (size_t j = 0; j < concept->feature_keys.size(); j++) {
                        if (concept->feature_keys[j] == "word_class") {
                            string word_class = concept->feature_values[j];

                            // Convert POS tags
                            if (word_class == "adjective") {
                                if (!quiet_mode) {
                                    cout << "[POS Query] \"" << word << "\" → adjective (database)" << endl;
                                }
                                return "adj";
                            } else if (word_class == "noun") {
                                if (!quiet_mode) {
                                    cout << "[POS Query] \"" << word << "\" → noun (database)" << endl;
                                }
                                return "noun";
                            }
                        }
                    }
                }
            }
        }

        // Not found in database, use backup rules
        return applyBackupPOSRules(word, quiet_mode);

    } catch (const exception& e) {
        if (!quiet_mode) {
            cerr << "[POS Query] Query failed: " << e.what() << endl;
        }
        return applyBackupPOSRules(word, quiet_mode);
    }
}

/**
 * Backup part-of-speech identification rules (used when database query fails)
 * @param word The word to identify
 * @param quiet_mode Whether to suppress debug output
 * @return "adj" for adjective, "noun" for noun, "unknown" for unknown
 */
string applyBackupPOSRules(const string& word, bool quiet_mode) {
    // English adjective common patterns
    vector<string> adj_suffixes = {"ful", "less", "ish", "ous", "ive", "able", "ible", "al", "ic", "ed", "ing"};
    for (const string& suffix : adj_suffixes) {
        if (word.length() > suffix.length() &&
            word.substr(word.length() - suffix.length()) == suffix) {
            if (!quiet_mode) {
                cout << "[POS Query] \"" << word << "\" → adjective (backup rule: suffix '" << suffix << "')" << endl;
            }
            return "adj";
        }
    }

    // English noun common patterns
    vector<string> noun_suffixes = {"ness", "tion", "sion", "ment", "ity", "ty", "er", "or", "ist", "ian", "ship", "hood"};
    for (const string& suffix : noun_suffixes) {
        if (word.length() > suffix.length() &&
            word.substr(word.length() - suffix.length()) == suffix) {
            if (!quiet_mode) {
                cout << "[POS Query] \"" << word << "\" → noun (backup rule: suffix '" << suffix << "')" << endl;
            }
            return "noun";
        }
    }

    // Predefined vocabulary
    vector<string> known_nouns = {"girl", "student", "teacher", "car", "vehicle", "apple", "book", "computer", "phone", "house", "person", "child", "woman", "man"};
    vector<string> known_adjs = {"beautiful", "gentle", "smart", "diligent", "fast", "red", "new", "excellent", "pretty", "cute", "good", "bad", "big", "small", "happy", "sad"};

    for (const string& noun : known_nouns) {
        if (word == noun) {
            if (!quiet_mode) {
                cout << "[POS Query] \"" << word << "\" → noun (backup rule: predefined)" << endl;
            }
            return "noun";
        }
    }

    for (const string& adj : known_adjs) {
        if (word == adj) {
            if (!quiet_mode) {
                cout << "[POS Query] \"" << word << "\" → adjective (backup rule: predefined)" << endl;
            }
            return "adj";
        }
    }

    if (!quiet_mode) {
        cout << "[POS Query] \"" << word << "\" → unknown (no rules can identify)" << endl;
    }
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
 * Parse comma-separated sequence into noun segments list
 * Rule: adjectives before each noun up to the previous noun belong to this noun
 * @param sequence comma-separated sequence (e.g.: "beautiful,gentle,girl,smart,diligent,student")
 * @param quiet_mode Whether to suppress debug output
 * @return noun segments list
 */
vector<NounSegment> parseSequenceToNounSegments(const string& sequence, bool quiet_mode = false) {
    vector<NounSegment> segments;
    vector<string> words = parseCommaInput(sequence);

    vector<string> current_adjectives;

    for (const string& word : words) {
        string pos = identifyPartOfSpeech(word, quiet_mode);

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
double detectSemanticContainment(const string& sequence1, const string& sequence2, bool quiet_mode = false) {
    if (!quiet_mode) {
        cout << "[Semantic Analysis] Detecting containment: \"" << sequence1 << "\" vs \"" << sequence2 << "\"" << endl;
    }

    // Parse both sequences into noun segments
    vector<NounSegment> segments1 = parseSequenceToNounSegments(sequence1, quiet_mode);
    vector<NounSegment> segments2 = parseSequenceToNounSegments(sequence2, quiet_mode);

    if (!quiet_mode) {
        cout << "[Semantic Analysis] Sequence 1 parse result:" << endl;
        for (size_t i = 0; i < segments1.size(); i++) {
            cout << "  Noun: " << segments1[i].noun << ", Adjectives: [";
            for (size_t j = 0; j < segments1[i].adjectives.size(); j++) {
                cout << segments1[i].adjectives[j];
                if (j < segments1[i].adjectives.size() - 1) cout << ", ";
            }
            cout << "]" << endl;
        }

        cout << "[Semantic Analysis] Sequence 2 parse result:" << endl;
        for (size_t i = 0; i < segments2.size(); i++) {
            cout << "  Noun: " << segments2[i].noun << ", Adjectives: [";
            for (size_t j = 0; j < segments2[i].adjectives.size(); j++) {
                cout << segments2[i].adjectives[j];
                if (j < segments2[i].adjectives.size() - 1) cout << ", ";
            }
            cout << "]" << endl;
        }
    }

    // Check containment relationship
    double containment_strength = checkSequenceContainment(segments1, segments2);

    if (!quiet_mode) {
        if (containment_strength > 0.0) {
            cout << "[Semantic Analysis] Containment detected! Strength: " << containment_strength << endl;
        } else {
            cout << "[Semantic Analysis] No containment detected" << endl;
        }
    }

    return containment_strength;
}

/**
 * 分析两个输入序列之间的语义包含关系
 * @param input_a 输入A（逗号分隔的序列）
 * @param input_b 输入B（逗号分隔的序列）
 * @return 无返回值，结果保存在g_semantic_result中
 */
void analyzeSemanticRelationship(const string& input_a, const string& input_b, bool quiet_mode = false) {
    if (!quiet_mode) {
        cout << "\n=== Semantic Relationship Analysis ===" << endl;
    }

    // Clear previous results
    g_semantic_result = SemanticAnalysisResult();
    g_semantic_result.input_a = input_a;
    g_semantic_result.input_b = input_b;

    if (!quiet_mode) {
        cout << "[Semantic Analysis] Analyzing sequence containment..." << endl;
        cout << "[Semantic Analysis] Sequence A: \"" << input_a << "\"" << endl;
        cout << "[Semantic Analysis] Sequence B: \"" << input_b << "\"" << endl;
    }

    // Directly check containment relationship between two complete sequences
    double containment_a_to_b = detectSemanticContainment(input_a, input_b, quiet_mode);
    double containment_b_to_a = detectSemanticContainment(input_b, input_a, quiet_mode);

    // Save results
    g_semantic_result.containment_strength_a_to_b = containment_a_to_b;
    g_semantic_result.containment_strength_b_to_a = containment_b_to_a;

    if (containment_a_to_b > 0.0 || containment_b_to_a > 0.0) {
        g_semantic_result.has_semantic_enhancement = true;
        if (!quiet_mode) {
            cout << "\n[Semantic Analysis Result] Semantic containment found:" << endl;
            if (containment_a_to_b > 0.0) {
                cout << "  A contains B strength: " << containment_a_to_b << endl;
            }
            if (containment_b_to_a > 0.0) {
                cout << "  B contains A strength: " << containment_b_to_a << endl;
            }
        }
    } else {
        if (!quiet_mode) {
            cout << "\n[Semantic Analysis Result] No semantic containment found" << endl;
        }
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
string preprocessInput(const string& input, bool quiet_mode = false) {
    // TODO: Implement semantic preprocessing logic here
    // Examples:
    // 1. Analyze compound words: good_content → good + content
    // 2. Detect containment relationships
    // 3. Expand synonyms
    // 4. Semantic enhancement

    if (!quiet_mode) {
        cout << "[Semantic Preprocessing] Input: " << input << endl;
    }

    // Currently return original input directly, logic can be added later
    string processed = input;

    if (!quiet_mode) {
        cout << "[Semantic Preprocessing] Processed: " << processed << endl;
    }
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
string postprocessOutput(const string& approacher_output, const string& original_input_a, const string& original_input_b, bool quiet_mode = false) {
    if (!quiet_mode) {
        cout << "[Semantic Postprocessing] Analyzing Approacher output and applying semantic enhancement..." << endl;
    }

    string enhanced_output = approacher_output;

    // If semantic containment relationship is detected, need to enhance similarity
    if (g_semantic_result.has_semantic_enhancement) {
        if (!quiet_mode) {
            cout << "[Semantic Postprocessing] Semantic containment detected, applying similarity enhancement" << endl;
        }

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

                        if (!quiet_mode) {
                            cout << "[Semantic Postprocessing] Similarity enhancement: " << original_score << " → "
                                 << enhanced_score << " (enhancement factor: " << enhancement_factor << ")" << endl;
                        }
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

    if (!quiet_mode) {
        // Add semantic analysis report
        enhanced_output += "\n=== Semantic Analysis Report ===\n";

        if (g_semantic_result.has_semantic_enhancement) {
            enhanced_output += "[Semantic Containment] Detection successful\n";
            if (g_semantic_result.containment_strength_a_to_b > 0.0) {
                enhanced_output += "  A contains B (strength: " + to_string(g_semantic_result.containment_strength_a_to_b) + ")\n";
            }
            if (g_semantic_result.containment_strength_b_to_a > 0.0) {
                enhanced_output += "  B contains A (strength: " + to_string(g_semantic_result.containment_strength_b_to_a) + ")\n";
            }
            enhanced_output += "  Similarity enhanced based on containment relationship\n";
        } else {
            enhanced_output += "[Semantic Containment] Not detected\n";
            enhanced_output += "  Using original Approacher similarity results\n";
        }
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
    string command = "cd . && ";
    command += "export LD_LIBRARY_PATH='./things/lib:$LD_LIBRARY_PATH' && ";
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

    // Special case 1: Two identical equals key-value pairs
    if (a_is_equals_kv && b_is_equals_kv && input_a == input_b) {
        cout << "[Equals Key-Value] Detected identical key-value pairs, returning fixed similarity 100" << endl;
        return 100.0;
    }

    // Special case 2: equals key-value pair vs corresponding key
    if (a_is_equals_kv && !b_is_equals_kv) {
        string key_a = extractKeyFromEqualsKeyValue(input_a);
        if (!key_a.empty() && key_a == input_b) {
            cout << "[Equals Key-Value] Detected key-value pair vs corresponding key match: \"" << input_a << "\" vs \"" << input_b << "\", returning fixed similarity 100" << endl;
            return 100.0;
        }
    }

    // Special case 3: key vs corresponding equals key-value pair
    if (!a_is_equals_kv && b_is_equals_kv) {
        string key_b = extractKeyFromEqualsKeyValue(input_b);
        if (!key_b.empty() && input_a == key_b) {
            cout << "[Equals Key-Value] Detected key vs corresponding key-value pair match: \"" << input_a << "\" vs \"" << input_b << "\", returning fixed similarity 100" << endl;
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
        cout << "[Equals Key-Value] Conversion: \"" << input_a << "\" → \"" << processed_a << "\"" << endl;
    }
    if (processed_b != input_b) {
        cout << "[Equals Key-Value] Conversion: \"" << input_b << "\" → \"" << processed_b << "\"" << endl;
    }
}

// 非交互模式处理函数
double runNonInteractiveMode(const string& input_a, const string& input_b, bool quiet_mode) {
    // 保存原始输入
    string original_a = input_a;
    string original_b = input_b;

    // Check special cases of equals key-value pairs
    if (!quiet_mode) {
        cout << "\n=== Equals Key-Value Check Phase ===" << endl;
    }
    double special_case_result = handleEqualsKeyValueSpecialCases(input_a, input_b);

    if (special_case_result >= 0) {
        // Is special case, return result directly
        if (!quiet_mode) {
            cout << "\nFinal result: Equals key-value special match" << endl;
            cout << "Similarity: " << special_case_result << endl;
        } else {
            cout << special_case_result << endl;
        }
        return special_case_result;
    }

    // 不是特殊情况，进行等号键值对预处理
    string equals_processed_a, equals_processed_b;
    preprocessEqualsKeyValuePairs(input_a, input_b, equals_processed_a, equals_processed_b);

    string line_a = equals_processed_a;
    string line_b = equals_processed_b;

    // Perform semantic relationship analysis
    if (!quiet_mode) {
        cout << "\n=== Semantic Analysis Phase ===" << endl;
    }
    analyzeSemanticRelationship(line_a, line_b, quiet_mode);

    // Preprocess input
    string processed_a = preprocessInput(line_a, quiet_mode);
    string processed_b = preprocessInput(line_b, quiet_mode);

    // Call approacher program
    if (!quiet_mode) {
        cout << "\n=== Calling Approacher Calculation ===" << endl;
    }
    string approacher_result = callApproacher(processed_a, processed_b);

    // Postprocess output
    if (!quiet_mode) {
        cout << "\n=== Semantic Postprocessing Phase ===" << endl;
    }
    string final_output = postprocessOutput(approacher_result, original_a, original_b, quiet_mode);

    // Parse three scores from original approacher output
    ThreeScoreResult basic_scores;

    stringstream basic_ss(approacher_result);
    string basic_line;
    while (getline(basic_ss, basic_line)) {
        if (basic_line.find("->") != string::npos && basic_line.find(" : ") != string::npos) {
            // A→B direction: [A]->[B] : score
            size_t colon_pos = basic_line.find(" : ");
            string score_str = basic_line.substr(colon_pos + 3);
            try {
                basic_scores.partial_a_to_b = stod(score_str);
            } catch (...) {}
        } else if (basic_line.find("<-") != string::npos && basic_line.find("<->") == string::npos && basic_line.find(" : ") != string::npos) {
            // B→A direction: [A]<-[B] : score
            size_t colon_pos = basic_line.find(" : ");
            string score_str = basic_line.substr(colon_pos + 3);
            try {
                basic_scores.partial_b_to_a = stod(score_str);
            } catch (...) {}
        } else if (basic_line.find("<->") != string::npos && basic_line.find(" : ") != string::npos) {
            // Main similarity: [A]<->[B] : score
            size_t colon_pos = basic_line.find(" : ");
            string score_str = basic_line.substr(colon_pos + 3);
            try {
                basic_scores.main_similarity = stod(score_str);
            } catch (...) {}
        }
    }

    // Parse enhanced main similarity from final output
    ThreeScoreResult enhanced_scores = basic_scores;  // Copy basic scores first

    stringstream ss(final_output);
    string line;
    while (getline(ss, line)) {
        if (line.find("<->") != string::npos && line.find(" : ") != string::npos) {
            // Enhanced main similarity: [A]<->[B] : score
            size_t colon_pos = line.find(" : ");
            string score_str = line.substr(colon_pos + 3);
            try {
                enhanced_scores.main_similarity = stod(score_str);
                break;
            } catch (...) {}
        }
    }

    // Handle error cases
    if (basic_scores.main_similarity <= 0.0 && basic_scores.partial_a_to_b <= 0.0 && basic_scores.partial_b_to_a <= 0.0) {
        // No valid scores found
        if (basic_scores.main_similarity == 0.0) {
            basic_scores.main_similarity = -1.0;  // No matching concept
            enhanced_scores.main_similarity = -1.0;
        } else {
            basic_scores.main_similarity = -2.0;  // Program error
            enhanced_scores.main_similarity = -2.0;
        }
        basic_scores.partial_a_to_b = basic_scores.main_similarity;
        basic_scores.partial_b_to_a = basic_scores.main_similarity;
        enhanced_scores.partial_a_to_b = basic_scores.partial_a_to_b;
        enhanced_scores.partial_b_to_a = basic_scores.partial_b_to_a;
    }

    if (quiet_mode) {
        // Output format: main_similarity,partial_a_to_b,partial_b_to_a
        cout << enhanced_scores.main_similarity << ","
             << enhanced_scores.partial_a_to_b << ","
             << enhanced_scores.partial_b_to_a << endl;
    } else {
        cout << "\nMain similarity: " << enhanced_scores.main_similarity << endl;
        cout << "A→B partial similarity: " << enhanced_scores.partial_a_to_b << endl;
        cout << "B→A partial similarity: " << enhanced_scores.partial_b_to_a << endl;
        if (g_semantic_result.has_semantic_enhancement && enhanced_scores.main_similarity != basic_scores.main_similarity) {
            cout << "Semantic enhancement: " << (enhanced_scores.main_similarity - basic_scores.main_similarity) << endl;
        }
    }

    return enhanced_scores.main_similarity;
}

void printHelp(const string& program_name) {
    cout << "Usage: " << program_name << " [options] [input_A] [input_B]" << endl;
    cout << "\nOptions:" << endl;
    cout << "  -h, --help      Show help information" << endl;
    cout << "  -q, --quiet     Quiet mode, output only similarity values" << endl;
    cout << "  -s, --silent    Same as --quiet" << endl;
    cout << "  -f, --fullinfo  Get full info including feature contributions" << endl;
    cout << "\nExamples:" << endl;
    cout << "  Interactive mode: " << program_name << endl;
    cout << "  Non-interactive:  " << program_name << " \"beautiful,girl\" \"girl\"" << endl;
    cout << "  Quiet mode:       " << program_name << " -q \"key=value\" \"key\"" << endl;
    cout << "  Full info:        " << program_name << " -f \"red,apple\" \"apple\"" << endl;
}

/**
 * 获取完整的特征贡献度信息
 * @param input_a 输入A字符串
 * @param input_b 输入B字符串
 * @param quiet_mode 是否静默模式
 * @return FullInfoResult 包含相似度和特征贡献信息
 */
FullInfoResult callSemanticApproacher_fullInfo(const string& input_a, const string& input_b, bool quiet_mode = false) {
    FullInfoResult result;

    if (!g_semantic_database) {
        if (!quiet_mode) {
            cerr << "Database not initialized!" << endl;
        }
        return result; // 默认值为-2.0，表示错误
    }

    try {
        // 解析输入
        auto parsed_a = parseCommaInput(input_a);
        auto parsed_b = parseCommaInput(input_b);

        if (parsed_a.empty() || parsed_b.empty()) {
            if (!quiet_mode) {
                cerr << "Input cannot be empty" << endl;
            }
            return result; // -2.0 程序错误
        }

        // 预处理特殊格式
        string processed_a = input_a;
        string processed_b = input_b;
        preprocessEqualsKeyValuePairs(input_a, input_b, processed_a, processed_b);

        // 转换为特征列表
        auto features_a = parseFeatureList(parseCommaInput(processed_a));
        auto features_b = parseFeatureList(parseCommaInput(processed_b));

        // 使用ConceptDatabase的完整贡献度计算函数
        auto contribution_info = g_semantic_database->calculateSimilarityWithFeatureContribution(
            features_a, features_b, g_similarity_params);

        // 填充结果
        result.forward_similarity = contribution_info.forward_similarity;
        result.reverse_similarity = contribution_info.reverse_similarity;
        result.main_similarity = contribution_info.main_similarity;

        // 转换特征贡献信息
        for (const auto& contrib : contribution_info.feature_contributions) {
            result.feature_contributions.push_back(FeatureContribution(contrib.first, contrib.second));
        }

        // 应用语义增强
        if (contribution_info.main_similarity > 0.0) {
            // 检测语义包含关系
            double containment_a_to_b = 0.0;
            double containment_b_to_a = 0.0;
            bool has_semantic = false;

            // 检测A→B方向的语义包含
            vector<string> compound_a = parseCommaInput(processed_a);
            vector<string> compound_b = parseCommaInput(processed_b);

            for (const auto& item_a : compound_a) {
                for (const auto& item_b : compound_b) {
                    double containment = detectSemanticContainment(item_a, item_b, quiet_mode);
                    if (containment > 0.0) {
                        containment_a_to_b += containment;
                        has_semantic = true;
                    }
                }
            }

            // 检测B→A方向的语义包含
            for (const auto& item_b : compound_b) {
                for (const auto& item_a : compound_a) {
                    double containment = detectSemanticContainment(item_b, item_a, quiet_mode);
                    if (containment > 0.0) {
                        containment_b_to_a += containment;
                        has_semantic = true;
                    }
                }
            }

            // 应用语义增强
            if (has_semantic) {
                double semantic_boost_a_to_b = containment_a_to_b * 0.3;
                double semantic_boost_b_to_a = containment_b_to_a * 0.3;

                result.forward_similarity = min(1.0, result.forward_similarity + semantic_boost_a_to_b);
                result.reverse_similarity = min(1.0, result.reverse_similarity + semantic_boost_b_to_a);
                result.main_similarity = sqrt(result.forward_similarity * result.reverse_similarity);

                if (!quiet_mode) {
                    cout << "[Semantic Enhancement Applied]" << endl;
                }
            }
        }

        // 处理无匹配情况
        if (result.main_similarity == 0.0) {
            result.forward_similarity = -1.0;
            result.reverse_similarity = -1.0;
            result.main_similarity = -1.0;
        }

    } catch (const exception& e) {
        if (!quiet_mode) {
            cerr << "Error during calculation: " << e.what() << endl;
        }
        // 保持默认的-2.0错误值
    }

    return result;
}

int main(int argc, char* argv[])
{
    bool quiet_mode = false;
    bool fullinfo_mode = false;
    string input_a, input_b;

    // 解析命令行参数
    vector<string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    // 检查帮助选项
    for (const auto& arg : args) {
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        }
    }

    // 解析其他选项和参数
    int arg_index = 0;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "-q" || args[i] == "--quiet" || args[i] == "-s" || args[i] == "--silent") {
            quiet_mode = true;
        } else if (args[i] == "-f" || args[i] == "--fullinfo") {
            fullinfo_mode = true;
        } else {
            // 非选项参数
            if (arg_index == 0) {
                input_a = args[i];
                arg_index++;
            } else if (arg_index == 1) {
                input_b = args[i];
                arg_index++;
            }
        }
    }

    // 初始化数据库连接
    g_semantic_database = make_unique<ConceptDatabase>();
    if (!g_semantic_database->initialize("things/concepts-db")) {
        if (!quiet_mode) {
            cerr << "Warning: Unable to connect to semantic analysis database, some features may be limited" << endl;
        }
    } else {
        if (!quiet_mode) {
            cout << "Semantic Approacher - Enhanced Concept Similarity Analyzer" << endl;
            cout << "Based on Approacher, with added semantic analysis layer and equals key-value special handling" << endl;
            cout << "Semantic analysis database connection successful" << endl;
            g_semantic_database->printStatistics(quiet_mode);
        }
    }

    // 根据参数决定运行模式
    if (arg_index == 2) {
        // 非交互模式
        if (fullinfo_mode) {
            // 完整信息模式
            FullInfoResult result = callSemanticApproacher_fullInfo(input_a, input_b, quiet_mode);

            if (quiet_mode) {
                // 静默模式：输出结构化数据
                cout << result.forward_similarity << "," << result.reverse_similarity << "," << result.main_similarity;
                for (const auto& contrib : result.feature_contributions) {
                    cout << "," << contrib.feature << ":" << contrib.contribution_percent;
                }
                cout << endl;
            } else {
                // 详细模式：输出可读信息
                cout << "\n=== 完整相似度分析结果 ===" << endl;
                cout << "正向相似度 (A→B): " << result.forward_similarity << endl;
                cout << "反向相似度 (B→A): " << result.reverse_similarity << endl;
                cout << "主相似度 (总体): " << result.main_similarity << endl;

                cout << "\n=== 特征贡献度分析 ===" << endl;
                for (const auto& contrib : result.feature_contributions) {
                    cout << "特征 \"" << contrib.feature << "\": " << contrib.contribution_percent << "%" << endl;
                }
            }

            // 根据结果确定退出码
            if (result.has_error()) {
                return 1;  // 程序错误
            } else {
                return 0;  // 正常
            }
        } else {
            // 标准模式
            double score = runNonInteractiveMode(input_a, input_b, quiet_mode);
            // 区分退出码：-2为程序错误，其他为正常
            return (score == -2.0) ? 1 : 0;
        }
    } else if (arg_index == 0) {
        // Interactive mode
        if (!quiet_mode) {
            cout << "\nEntering interactive mode..." << endl;
        }
        runSemanticApproacher();
    } else {
        cerr << "Error: Incorrect number of arguments. Use -h for help." << endl;
        return 1;
    }

    if (!quiet_mode) {
        cout << "Semantic Approacher program ended." << endl;
    }
    return 0;
}