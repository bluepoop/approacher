#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

/**
 * 调用 approacher_lib 中的程序的封装类
 */
/**
 * 三分数结构体 - 存储主相似度和两个方向的分相似度
 */
struct ThreeScores {
    double main_similarity;     // 语义增强后的主相似度
    double partial_a_to_b;      // A→B方向分相似度
    double partial_b_to_a;      // B→A方向分相似度

    ThreeScores(double main = -2.0, double a_to_b = -2.0, double b_to_a = -2.0)
        : main_similarity(main), partial_a_to_b(a_to_b), partial_b_to_a(b_to_a) {}
};

/**
 * 双分数结构体 - 存储基础相似度和语义增强相似度（保持向后兼容）
 */
struct SimilarityScores {
    double basic_score;      // 基础相似度
    double enhanced_score;   // 语义增强相似度

    SimilarityScores(double basic = -2.0, double enhanced = -2.0)
        : basic_score(basic), enhanced_score(enhanced) {}
};

/**
 * 完整分析结果结构体 - 包含所有三个分数
 */
struct CompleteSimilarityResult {
    double basic_score;         // 基础approacher的分数
    double semantic_score;      // 语义增强后的分数
    double enhancement_boost;   // 语义提升幅度 (semantic - basic)

    // 错误状态检查
    bool has_error() const {
        return basic_score == -2.0 || semantic_score == -2.0;
    }

    bool no_match() const {
        return basic_score == -1.0 || semantic_score == -1.0;
    }

    CompleteSimilarityResult(double basic = -2.0, double semantic = -2.0)
        : basic_score(basic), semantic_score(semantic),
          enhancement_boost(semantic - basic) {}
};

/**
 * 特征贡献度信息结构体
 */
struct FeatureContribution {
    string feature;           // 特征名
    double contribution_percent;  // 贡献百分比

    FeatureContribution(const string& feat = "", double contrib = 0.0)
        : feature(feat), contribution_percent(contrib) {}
};

/**
 * 完整信息结果结构体 - 包含相似度和特征贡献度信息
 */
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

/**
 * 解释相似度分数的含义
 */
string explainScore(double score) {
    if (score == -2.0) {
        return "程序错误";
    } else if (score == -1.0) {
        return "无匹配概念";
    } else if (score >= 0.0) {
        return "相似度: " + to_string(score);
    } else {
        return "未知错误";
    }
}

class ApproacherLibCaller {
private:
  string lib_path;

  /**
   * 执行命令并获取输出
   * 返回pair<输出内容, 退出码>
   */
  pair<string, int> executeCommand(const string &command) {
    // 添加 2>/dev/null 来屏蔽 stderr
    string full_command = command + " 2>/dev/null";
    FILE *pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
      throw runtime_error("无法执行命令: " + command);
    }

    string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
    }

    int exit_code = pclose(pipe);
    return make_pair(result, exit_code);
  }

  /**
   * 解析输出中的数值（支持多行输出）
   */
  double parseNumericResult(const string &output) {
    // 尝试从输出中提取最后一个有效数值
    stringstream ss(output);
    string line;
    double last_valid_number = -1.0;

    while (getline(ss, line)) {
      // 尝试解析每一行为数值
      try {
        // 去除前后空白
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos)
          continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        string trimmed = line.substr(start, end - start + 1);

        // 检查是否是双分数格式 (basic_score,enhanced_score)
        size_t comma_pos = trimmed.find(',');
        if (comma_pos != string::npos) {
          // 解析增强分数（逗号后的值）
          string enhanced_str = trimmed.substr(comma_pos + 1);
          double enhanced_value = stod(enhanced_str);
          last_valid_number = enhanced_value;
        } else {
          // 单分数格式
          double value = stod(trimmed);
          last_valid_number = value;
        }
      } catch (...) {
        // 如果不是数值，忽略这一行
        continue;
      }
    }

    // 检查是否找到了有效数值
    // -1和-2是有效返回值，只有在完全没有找到数值时才报错
    if (last_valid_number < -2.0) {
      throw invalid_argument("无法从输出中解析数值");
    }

    return last_valid_number;
  }

  /**
   * 解析逗号分隔的输入字符串
   */
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
   * 解析三分数输出格式 (main_similarity,partial_a_to_b,partial_b_to_a)
   */
  ThreeScores parseThreeScores(const string &output) {
    stringstream ss(output);
    string line;
    ThreeScores scores;

    while (getline(ss, line)) {
      try {
        // 去除前后空白
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos)
          continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        string trimmed = line.substr(start, end - start + 1);

        // 检查是否是三分数格式
        size_t first_comma = trimmed.find(',');
        if (first_comma != string::npos) {
          size_t second_comma = trimmed.find(',', first_comma + 1);
          if (second_comma != string::npos) {
            // 三分数格式: main,a_to_b,b_to_a
            string main_str = trimmed.substr(0, first_comma);
            string a_to_b_str = trimmed.substr(first_comma + 1, second_comma - first_comma - 1);
            string b_to_a_str = trimmed.substr(second_comma + 1);

            scores.main_similarity = stod(main_str);
            scores.partial_a_to_b = stod(a_to_b_str);
            scores.partial_b_to_a = stod(b_to_a_str);
            return scores;
          } else {
            // 双分数格式，当作main和main处理
            string first_str = trimmed.substr(0, first_comma);
            string second_str = trimmed.substr(first_comma + 1);
            double main_score = stod(second_str);  // 使用第二个值作为主分数
            scores.main_similarity = main_score;
            scores.partial_a_to_b = main_score;
            scores.partial_b_to_a = main_score;
            return scores;
          }
        } else {
          // 单分数格式，所有分数相同
          double value = stod(trimmed);
          scores.main_similarity = value;
          scores.partial_a_to_b = value;
          scores.partial_b_to_a = value;
          return scores;
        }
      } catch (...) {
        continue;
      }
    }

    return scores;  // 返回默认值(-2.0, -2.0, -2.0)
  }

  /**
   * 解析双分数输出格式 (basic_score,enhanced_score) - 保持向后兼容
   */
  SimilarityScores parseDualScores(const string &output) {
    stringstream ss(output);
    string line;
    SimilarityScores scores;

    while (getline(ss, line)) {
      try {
        // 去除前后空白
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos)
          continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        string trimmed = line.substr(start, end - start + 1);

        // 检查是否是双分数格式
        size_t comma_pos = trimmed.find(',');
        if (comma_pos != string::npos) {
          // 解析基础分数和增强分数
          string basic_str = trimmed.substr(0, comma_pos);
          string enhanced_str = trimmed.substr(comma_pos + 1);

          scores.basic_score = stod(basic_str);
          scores.enhanced_score = stod(enhanced_str);
          return scores;
        } else {
          // 单分数格式，基础分数和增强分数相同
          double value = stod(trimmed);
          scores.basic_score = value;
          scores.enhanced_score = value;
          return scores;
        }
      } catch (...) {
        continue;
      }
    }

    return scores;  // 返回默认值(-2.0, -2.0)
  }

public:
  /**
   * 构造函数
   * @param lib_dir approacher_lib 目录路径
   */
  ApproacherLibCaller(const string &lib_dir = "approacher_lib")
      : lib_path(lib_dir) {}

  /**
   * 调用基础approacher
   * @param input_a 输入A
   * @param input_b 输入B
   * @param use_fuzzy 是否使用模糊匹配
   * @param quiet_mode 静默模式（只返回数值）
   * @return 相似度值
   */
  double callApproacher(const string &input_a, const string &input_b,
                        bool use_fuzzy = false, bool quiet_mode = true) {
    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./approacher";

    if (quiet_mode)
      command += " -q";
    if (use_fuzzy)
      command += " -f";

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto result = executeCommand(command);
      string output = result.first;
      int exit_code = result.second;

      // 根据退出码判断：0为正常，1为程序错误
      if (exit_code == 1) {
        return -2.0;  // 程序错误
      } else if (exit_code == 0) {
        // 解析输出，可能是 -1（无匹配）或正常相似度值
        return parseNumericResult(output);
      } else {
        // 其他未知退出码
        return -2.0;  // 程序错误
      }
    } catch (const exception &e) {
      cerr << "调用approacher失败: " << e.what() << endl;
      return -2.0;  // 程序错误
    }
  }

  /**
   * 调用语义增强approacher
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return 相似度值
   */
  double callSemanticApproacher(const string &input_a, const string &input_b,
                                bool quiet_mode = true) {
    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./semantic_approacher";

    if (quiet_mode)
      command += " -q";

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto result = executeCommand(command);
      string output = result.first;
      int exit_code = result.second;

      // 根据退出码判断：0为正常，1为程序错误
      if (exit_code == 1) {
        return -2.0;  // 程序错误
      } else if (exit_code == 0) {
        // 解析输出，可能是 -1（无匹配）或正常相似度值
        return parseNumericResult(output);
      } else {
        // 其他未知退出码
        return -2.0;  // 程序错误
      }
    } catch (const exception &e) {
      cerr << "调用semantic_approacher失败: " << e.what() << endl;
      return -2.0;  // 程序错误
    }
  }

  /**
   * 调用语义增强approacher并获取三分数结果
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return ThreeScores结构体，包含主相似度和两个方向的分相似度
   */
  ThreeScores callSemanticApproacherThree(const string &input_a, const string &input_b,
                                         bool quiet_mode = true) {
    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./semantic_approacher";

    if (quiet_mode)
      command += " -q";

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto result = executeCommand(command);
      string output = result.first;
      int exit_code = result.second;

      // 根据退出码判断：0为正常，1为程序错误
      if (exit_code == 1) {
        return ThreeScores(-2.0, -2.0, -2.0);  // 程序错误
      } else if (exit_code == 0) {
        // 解析三分数输出
        return parseThreeScores(output);
      } else {
        // 其他未知退出码
        return ThreeScores(-2.0, -2.0, -2.0);  // 程序错误
      }
    } catch (const exception &e) {
      cerr << "调用semantic_approacher失败: " << e.what() << endl;
      return ThreeScores(-2.0, -2.0, -2.0);  // 程序错误
    }
  }

  /**
   * 调用语义增强approacher并获取双分数结果（保持向后兼容）
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return SimilarityScores结构体，包含基础分数和增强分数
   */
  SimilarityScores callSemanticApproacherDual(const string &input_a, const string &input_b,
                                             bool quiet_mode = true) {
    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./semantic_approacher";

    if (quiet_mode)
      command += " -q";

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto result = executeCommand(command);
      string output = result.first;
      int exit_code = result.second;

      // 根据退出码判断：0为正常，1为程序错误
      if (exit_code == 1) {
        return SimilarityScores(-2.0, -2.0);  // 程序错误
      } else if (exit_code == 0) {
        // 解析双分数输出
        return parseDualScores(output);
      } else {
        // 其他未知退出码
        return SimilarityScores(-2.0, -2.0);  // 程序错误
      }
    } catch (const exception &e) {
      cerr << "调用semantic_approacher失败: " << e.what() << endl;
      return SimilarityScores(-2.0, -2.0);  // 程序错误
    }
  }

  /**
   * 获取完整相似度分析结果（三个数值）
   * @param input_a 输入A
   * @param input_b 输入B
   * @return CompleteSimilarityResult结构体，包含基础分数、语义分数和提升幅度
   */
  CompleteSimilarityResult getCompleteSimilarity(const string &input_a, const string &input_b) {
    // 调用基础approacher获取原始分数
    double basic_score = callApproacher(input_a, input_b, false, true);

    // 调用语义增强approacher获取增强分数
    double semantic_score = callSemanticApproacher(input_a, input_b, true);

    // 返回完整结果
    return CompleteSimilarityResult(basic_score, semantic_score);
  }

  /**
   * 获取完整特征贡献度信息
   * @param input_a 输入A
   * @param input_b 输入B
   * @return FullInfoResult结构体，包含相似度和特征贡献度信息
   */
  FullInfoResult getFullInfo(const string &input_a, const string &input_b) {
    FullInfoResult result;

    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
    command += "./semantic_approacher -f -q";

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto [output, exit_code] = executeCommand(command);

      if (exit_code != 0) {
        // 程序执行错误
        return result; // 返回默认的-2.0错误值
      }

      // 解析静默模式输出格式: forward,reverse,main,feature1:contrib1,feature2:contrib2,...
      if (output.empty()) {
        return result; // 返回默认的-2.0错误值
      }

      // 去除换行符
      string clean_output = output;
      if (!clean_output.empty() && clean_output.back() == '\n') {
        clean_output.pop_back();
      }

      stringstream ss(clean_output);
      string item;
      int index = 0;

      while (getline(ss, item, ',')) {
        if (index == 0) {
          try { result.forward_similarity = stod(item); } catch (...) {}
        } else if (index == 1) {
          try { result.reverse_similarity = stod(item); } catch (...) {}
        } else if (index == 2) {
          try { result.main_similarity = stod(item); } catch (...) {}
        } else {
          // 解析特征贡献度 格式: "feature:percent"
          size_t colon_pos = item.find(':');
          if (colon_pos != string::npos) {
            string feature = item.substr(0, colon_pos);
            string percent_str = item.substr(colon_pos + 1);
            try {
              double percent = stod(percent_str);
              result.feature_contributions.push_back(FeatureContribution(feature, percent));
            } catch (...) {}
          }
        }
        index++;
      }

    } catch (const exception& e) {
      // 错误情况，保持默认的-2.0值
    }

    return result;
  }

  /**
   * 获取详细分析结果（非静默模式）
   * @param input_a 输入A
   * @param input_b 输入B
   * @param use_semantic 是否使用语义增强
   * @return 完整输出文本
   */
  string getDetailedAnalysis(const string &input_a, const string &input_b,
                             bool use_semantic = true) {
    string command = "cd " + lib_path + " && ";
    command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";

    if (use_semantic) {
      command += "./semantic_approacher";
    } else {
      command += "./approacher";
    }

    command += " \"" + input_a + "\" \"" + input_b + "\"";

    try {
      auto result = executeCommand(command);
      string output = result.first;
      int exit_code = result.second;

      if (exit_code != 0) {
        return "错误: 程序执行失败 (退出码: " + to_string(exit_code) + ")";
      }
      return output;
    } catch (const exception &e) {
      return "错误: " + string(e.what());
    }
  }
};

/**
 * 交互式测试函数
 */
void runInteractiveTest(ApproacherLibCaller &caller) {
  cout << "\n=== 交互式测试模式 ===" << endl;
  cout << "输入 'quit' 退出" << endl;

  string input_a, input_b;

  while (true) {
    cout << "\n输入对象A: ";
    if (!getline(cin, input_a) || input_a == "quit")
      break;

    cout << "输入对象B: ";
    if (!getline(cin, input_b) || input_b == "quit")
      break;

    cout << "\n=== 基础分析器结果 ===" << endl;
    double basic_score = caller.callApproacher(input_a, input_b, false, true);
    cout << "相似度: " << basic_score << endl;

    cout << "\n=== 语义增强分析器结果 ===" << endl;
    double semantic_score =
        caller.callSemanticApproacher(input_a, input_b, true);
    cout << "相似度: " << semantic_score << endl;

    if (semantic_score > basic_score) {
      cout << "语义增强提升: +" << (semantic_score - basic_score) << endl;
    }
  }
}

int main() {
  cout << "Approacher Library 调用程序演示" << endl;
  cout << "=================================" << endl;

  ApproacherLibCaller caller;

  // 预定义测试用例
  vector<pair<string, string>> test_cases = {
      {"red,apple", "green,apple"},
      {"book,red", "book"},
      {"key=value", "key"},
      {"book,green", "red,apple"},
      {"good_content,red", "content,apple"}};

  cout << "\n1. 批量测试预定义用例" << endl;
  cout << "========================" << endl;

  for (size_t i = 0; i < test_cases.size(); i++) {
    const auto &test_case = test_cases[i];
    cout << "\n[测试 " << (i + 1) << "] \"" << test_case.first << "\" vs \""
         << test_case.second << "\"" << endl;

    // 基础分析器测试
    double basic_score =
        caller.callApproacher(test_case.first, test_case.second);
    cout << "基础分析器: " << explainScore(basic_score) << endl;

    // 语义增强分析器测试（三分数）
    ThreeScores three_scores =
        caller.callSemanticApproacherThree(test_case.first, test_case.second);
    cout << "语义分析:   主=" << explainScore(three_scores.main_similarity)
         << ", A→B=" << explainScore(three_scores.partial_a_to_b)
         << ", B→A=" << explainScore(three_scores.partial_b_to_a) << endl;
  }

  cout << "\n2. 完整分析示例（三个数值）" << endl;
  cout << "================================" << endl;

  // 演示getCompleteSimilarity函数
  vector<pair<string, string>> complete_test_cases = {
      {"beautiful,red,apple", "apple"},  // 应该有语义提升
      {"red,apple", "green,apple"},      // 无语义提升
      {"key=value", "key"}               // 特殊匹配
  };

  for (size_t i = 0; i < complete_test_cases.size(); i++) {
    const auto &test_case = complete_test_cases[i];
    cout << "\n[完整分析 " << (i + 1) << "] \"" << test_case.first << "\" vs \""
         << test_case.second << "\"" << endl;

    CompleteSimilarityResult result = caller.getCompleteSimilarity(test_case.first, test_case.second);

    cout << "  基础分数:     " << result.basic_score << endl;
    cout << "  语义增强分数: " << result.semantic_score << endl;
    cout << "  语义提升:     " << result.enhancement_boost << endl;

    if (result.has_error()) {
      cout << "  状态: 程序错误" << endl;
    } else if (result.no_match()) {
      cout << "  状态: 无匹配概念" << endl;
    } else if (result.enhancement_boost > 0.1) {
      cout << "  状态: 语义增强有效 (+)" << endl;
    } else {
      cout << "  状态: 无语义增强" << endl;
    }
  }

  cout << "\n3. 详细分析示例" << endl;
  cout << "===============" << endl;

  string detailed_result =
      caller.getDetailedAnalysis("apple", "red,apple", true);
  cout << "\n详细分析结果:" << endl;
  cout << detailed_result << endl;

  cout << "\n4. 性能基准测试" << endl;
  cout << "===============" << endl;

  const int benchmark_rounds = 10;
  clock_t start = clock();

  for (int i = 0; i < benchmark_rounds; i++) {
    caller.callSemanticApproacher("key=value", "key", true);
  }

  clock_t end = clock();
  double elapsed = double(end - start) / CLOCKS_PER_SEC;

  cout << "执行 " << benchmark_rounds << " 次调用耗时: " << elapsed << " 秒"
       << endl;
  cout << "平均每次调用: " << (elapsed / benchmark_rounds) << " 秒" << endl;

  cout << "\n5. 函数调用示例" << endl;
  cout << "===============" << endl;
  cout << "以下展示如何在C++代码中调用approacher功能:" << endl;
  cout << endl;
  cout << "// 基础调用" << endl;
  cout << "ApproacherLibCaller caller;" << endl;
  cout
      << "double score = caller.callApproacher(\"red,apple\", \"green,apple\");"
      << endl;
  cout << endl;
  cout << "// 语义增强调用（单分数）" << endl;
  cout << "double semantic_score = "
          "caller.callSemanticApproacher(\"book\", \"green\");"
       << endl;
  cout << endl;
  cout << "// 语义增强调用（三分数）" << endl;
  cout << "ThreeScores scores = "
          "caller.callSemanticApproacherThree(\"beautiful,red,apple\", \"apple\");"
       << endl;
  cout << "// scores.main_similarity: 语义增强后主相似度" << endl;
  cout << "// scores.partial_a_to_b: A→B方向分相似度" << endl;
  cout << "// scores.partial_b_to_a: B→A方向分相似度" << endl;
  cout << endl;
  cout << "// 语义增强调用（双分数，保持兼容）" << endl;
  cout << "SimilarityScores scores2 = "
          "caller.callSemanticApproacherDual(\"beautiful,red,apple\", \"apple\");"
       << endl;
  cout << "// scores2.basic_score: 基础相似度" << endl;
  cout << "// scores2.enhanced_score: 语义增强相似度" << endl;
  cout << endl;
  cout << "// 完整相似度分析（推荐使用）" << endl;
  cout << "CompleteSimilarityResult result = "
          "caller.getCompleteSimilarity(\"beautiful,red,apple\", \"apple\");"
       << endl;
  cout << "// result.basic_score: 基础分数" << endl;
  cout << "// result.semantic_score: 语义增强分数" << endl;
  cout << "// result.enhancement_boost: 语义提升幅度" << endl;
  cout << "// result.has_error(): 检查是否有错误" << endl;
  cout << "// result.no_match(): 检查是否无匹配" << endl;
  cout << endl;
  cout << "// 详细分析" << endl;
  cout << "string analysis = caller.getDetailedAnalysis(\"input_a\", "
          "\"input_b\");"
       << endl;

  // 询问是否进入交互模式
  cout << "\n是否进入交互式测试模式? (y/n): ";
  string response;
  if (getline(cin, response) && (response == "y" || response == "Y")) {
    runInteractiveTest(caller);
  }

  cout << "\n程序结束。" << endl;
  return 0;
}
