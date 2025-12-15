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
class ApproacherLibCaller {
private:
  string lib_path;

  /**
   * 执行命令并获取输出
   */
  string executeCommand(const string &command) {
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
    if (exit_code != 0) {
      throw runtime_error("命令执行失败 可能是库中无相应概念 (退出码: " +
                          to_string(exit_code) + ")");
    }

    return result;
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

        // 尝试转换为数值
        double value = stod(trimmed);
        last_valid_number = value;
      } catch (...) {
        // 如果不是数值，忽略这一行
        continue;
      }
    }

    if (last_valid_number < 0) {
      throw invalid_argument("无法从输出中解析数值");
    }

    return last_valid_number;
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
      string result = executeCommand(command);
      return parseNumericResult(result);
    } catch (const exception &e) {
      cerr << "调用approacher失败: " << e.what() << endl;
      return -1.0;
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
      string result = executeCommand(command);
      return parseNumericResult(result);
    } catch (const exception &e) {
      cerr << "调用semantic_approacher失败: " << e.what() << endl;
      return -1.0;
    }
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
      return executeCommand(command);
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
    cout << "基础分析器: " << basic_score << endl;

    // 语义增强分析器测试
    double semantic_score =
        caller.callSemanticApproacher(test_case.first, test_case.second);
    cout << "语义增强:   " << semantic_score << endl;

    if (semantic_score > basic_score) {
      cout << "语义提升:   +" << (semantic_score - basic_score) << endl;
    }
  }

  cout << "\n2. 详细分析示例" << endl;
  cout << "===============" << endl;

  string detailed_result =
      caller.getDetailedAnalysis("apple", "red,apple", true);
  cout << "\n详细分析结果:" << endl;
  cout << detailed_result << endl;

  cout << "\n3. 性能基准测试" << endl;
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

  cout << "\n4. 函数调用示例" << endl;
  cout << "===============" << endl;
  cout << "以下展示如何在C++代码中调用approacher功能:" << endl;
  cout << endl;
  cout << "// 基础调用" << endl;
  cout << "ApproacherLibCaller caller;" << endl;
  cout
      << "double score = caller.callApproacher(\"red,apple\", \"green,apple\");"
      << endl;
  cout << endl;
  cout << "// 语义增强调用" << endl;
  cout << "double semantic_score = "
          "caller.callSemanticApproacher(\"book\", \"green\");"
       << endl;
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
