#include "approacher_caller.hpp"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

// ThreeScores 实现
ThreeScores::ThreeScores(double main, double a_to_b, double b_to_a)
    : main_similarity(main), partial_a_to_b(a_to_b), partial_b_to_a(b_to_a) {}

// SimilarityScores 实现
SimilarityScores::SimilarityScores(double basic, double enhanced)
    : basic_score(basic), enhanced_score(enhanced) {}

// CompleteSimilarityResult 实现
CompleteSimilarityResult::CompleteSimilarityResult(double basic,
                                                   double semantic)
    : basic_score(basic), semantic_score(semantic),
      enhancement_boost(semantic - basic) {}

bool CompleteSimilarityResult::has_error() const {
  return basic_score == -2.0 || semantic_score == -2.0;
}

bool CompleteSimilarityResult::no_match() const {
  return basic_score == -1.0 || semantic_score == -1.0;
}

// FeatureContribution 实现
FeatureContribution::FeatureContribution(const string &feat, double contrib)
    : feature(feat), contribution_percent(contrib) {}

// FullInfoResult 实现
FullInfoResult::FullInfoResult(double forward, double reverse, double main)
    : forward_similarity(forward), reverse_similarity(reverse),
      main_similarity(main) {}

bool FullInfoResult::has_error() const { return main_similarity == -2.0; }

bool FullInfoResult::no_match() const { return main_similarity == -1.0; }

// explainScore 实现
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

// ApproacherLibCaller 实现
ApproacherLibCaller::ApproacherLibCaller(const string &lib_dir)
    : lib_path(lib_dir) {}

pair<string, int> ApproacherLibCaller::executeCommand(const string &command) {
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

double ApproacherLibCaller::parseNumericResult(const string &output) {
  stringstream ss(output);
  string line;
  double last_valid_number = -1.0;

  while (getline(ss, line)) {
    try {
      size_t start = line.find_first_not_of(" \t\r\n");
      if (start == string::npos)
        continue;
      size_t end = line.find_last_not_of(" \t\r\n");
      string trimmed = line.substr(start, end - start + 1);

      size_t comma_pos = trimmed.find(',');
      if (comma_pos != string::npos) {
        string enhanced_str = trimmed.substr(comma_pos + 1);
        double enhanced_value = stod(enhanced_str);
        last_valid_number = enhanced_value;
      } else {
        double value = stod(trimmed);
        last_valid_number = value;
      }
    } catch (...) {
      continue;
    }
  }

  if (last_valid_number < -2.0) {
    throw invalid_argument("无法从输出中解析数值");
  }

  return last_valid_number;
}

vector<string> ApproacherLibCaller::parseCommaInput(const string &input) {
  vector<string> result;
  stringstream ss(input);
  string item;

  while (getline(ss, item, ',')) {
    size_t start = item.find_first_not_of(" \t");
    if (start == string::npos)
      continue;

    size_t end = item.find_last_not_of(" \t");
    item = item.substr(start, end - start + 1);

    if (!item.empty()) {
      result.push_back(item);
    }
  }

  return result;
}

ThreeScores ApproacherLibCaller::parseThreeScores(const string &output) {
  stringstream ss(output);
  string line;
  ThreeScores scores;

  while (getline(ss, line)) {
    try {
      size_t start = line.find_first_not_of(" \t\r\n");
      if (start == string::npos)
        continue;
      size_t end = line.find_last_not_of(" \t\r\n");
      string trimmed = line.substr(start, end - start + 1);

      size_t first_comma = trimmed.find(',');
      if (first_comma != string::npos) {
        size_t second_comma = trimmed.find(',', first_comma + 1);
        if (second_comma != string::npos) {
          string main_str = trimmed.substr(0, first_comma);
          string a_to_b_str =
              trimmed.substr(first_comma + 1, second_comma - first_comma - 1);
          string b_to_a_str = trimmed.substr(second_comma + 1);

          scores.main_similarity = stod(main_str);
          scores.partial_a_to_b = stod(a_to_b_str);
          scores.partial_b_to_a = stod(b_to_a_str);
          return scores;
        } else {
          string first_str = trimmed.substr(0, first_comma);
          string second_str = trimmed.substr(first_comma + 1);
          double main_score = stod(second_str);
          scores.main_similarity = main_score;
          scores.partial_a_to_b = main_score;
          scores.partial_b_to_a = main_score;
          return scores;
        }
      } else {
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

  return scores;
}

SimilarityScores ApproacherLibCaller::parseDualScores(const string &output) {
  stringstream ss(output);
  string line;
  SimilarityScores scores;

  while (getline(ss, line)) {
    try {
      size_t start = line.find_first_not_of(" \t\r\n");
      if (start == string::npos)
        continue;
      size_t end = line.find_last_not_of(" \t\r\n");
      string trimmed = line.substr(start, end - start + 1);

      size_t comma_pos = trimmed.find(',');
      if (comma_pos != string::npos) {
        string basic_str = trimmed.substr(0, comma_pos);
        string enhanced_str = trimmed.substr(comma_pos + 1);

        scores.basic_score = stod(basic_str);
        scores.enhanced_score = stod(enhanced_str);
        return scores;
      } else {
        double value = stod(trimmed);
        scores.basic_score = value;
        scores.enhanced_score = value;
        return scores;
      }
    } catch (...) {
      continue;
    }
  }

  return scores;
}

double ApproacherLibCaller::callApproacher(const string &input_a,
                                           const string &input_b,
                                           bool use_fuzzy, bool quiet_mode) {
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

    if (exit_code == 1) {
      return -2.0;
    } else if (exit_code == 0) {
      return parseNumericResult(output);
    } else {
      return -2.0;
    }
  } catch (const exception &e) {
    cerr << "调用approacher失败: " << e.what() << endl;
    return -2.0;
  }
}

double ApproacherLibCaller::callSemanticApproacher(const string &input_a,
                                                   const string &input_b,
                                                   bool quiet_mode) {
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

    if (exit_code == 1) {
      return -2.0;
    } else if (exit_code == 0) {
      return parseNumericResult(output);
    } else {
      return -2.0;
    }
  } catch (const exception &e) {
    cerr << "调用semantic_approacher失败: " << e.what() << endl;
    return -2.0;
  }
}

ThreeScores ApproacherLibCaller::callSemanticApproacherThree(
    const string &input_a, const string &input_b, bool quiet_mode) {
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

    if (exit_code == 1) {
      return ThreeScores(-2.0, -2.0, -2.0);
    } else if (exit_code == 0) {
      return parseThreeScores(output);
    } else {
      return ThreeScores(-2.0, -2.0, -2.0);
    }
  } catch (const exception &e) {
    cerr << "调用semantic_approacher失败: " << e.what() << endl;
    return ThreeScores(-2.0, -2.0, -2.0);
  }
}

SimilarityScores ApproacherLibCaller::callSemanticApproacherDual(
    const string &input_a, const string &input_b, bool quiet_mode) {
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

    if (exit_code == 1) {
      return SimilarityScores(-2.0, -2.0);
    } else if (exit_code == 0) {
      return parseDualScores(output);
    } else {
      return SimilarityScores(-2.0, -2.0);
    }
  } catch (const exception &e) {
    cerr << "调用semantic_approacher失败: " << e.what() << endl;
    return SimilarityScores(-2.0, -2.0);
  }
}

CompleteSimilarityResult
ApproacherLibCaller::getCompleteSimilarity(const string &input_a,
                                           const string &input_b) {
  double basic_score = callApproacher(input_a, input_b, false, true);
  double semantic_score = callSemanticApproacher(input_a, input_b, true);
  return CompleteSimilarityResult(basic_score, semantic_score);
}

FullInfoResult ApproacherLibCaller::getFullInfo(const string &input_a,
                                                const string &input_b) {
  FullInfoResult result;

  string command = "cd " + lib_path + " && ";
  command += "export LD_LIBRARY_PATH='./lib:$LD_LIBRARY_PATH' && ";
  command += "./semantic_approacher -f -q";
  command += " \"" + input_a + "\" \"" + input_b + "\"";

  try {
    auto [output, exit_code] = executeCommand(command);

    if (exit_code != 0) {
      return result;
    }

    if (output.empty()) {
      return result;
    }

    string clean_output = output;
    if (!clean_output.empty() && clean_output.back() == '\n') {
      clean_output.pop_back();
    }

    stringstream ss(clean_output);
    string item;
    int index = 0;

    while (getline(ss, item, ',')) {
      if (index == 0) {
        try {
          result.forward_similarity = stod(item);
        } catch (...) {
        }
      } else if (index == 1) {
        try {
          result.reverse_similarity = stod(item);
        } catch (...) {
        }
      } else if (index == 2) {
        try {
          result.main_similarity = stod(item);
        } catch (...) {
        }
      } else {
        size_t colon_pos = item.find(':');
        if (colon_pos != string::npos) {
          string feature = item.substr(0, colon_pos);
          string percent_str = item.substr(colon_pos + 1);
          try {
            double percent = stod(percent_str);
            result.feature_contributions.push_back(
                FeatureContribution(feature, percent));
          } catch (...) {
          }
        }
      }
      index++;
    }

  } catch (const exception &e) {
    // 错误情况，保持默认的-2.0值
  }

  return result;
}

string ApproacherLibCaller::getDetailedAnalysis(const string &input_a,
                                                const string &input_b,
                                                bool use_semantic) {
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
