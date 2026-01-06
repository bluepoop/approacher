#ifndef APPROACHER_CALLER_H
#define APPROACHER_CALLER_H

#include <string>
#include <utility>
#include <vector>

/**
 * 三分数结构体 - 存储主相似度和两个方向的分相似度
 */
struct ThreeScores {
  double main_similarity; // 语义增强后的主相似度
  double partial_a_to_b;  // A→B方向分相似度
  double partial_b_to_a;  // B→A方向分相似度

  ThreeScores(double main = -2.0, double a_to_b = -2.0, double b_to_a = -2.0);
};

/**
 * 双分数结构体 - 存储基础相似度和语义增强相似度（保持向后兼容）
 */
struct SimilarityScores {
  double basic_score;    // 基础相似度
  double enhanced_score; // 语义增强相似度

  SimilarityScores(double basic = -2.0, double enhanced = -2.0);
};

/**
 * 完整分析结果结构体 - 包含所有三个分数
 */
struct CompleteSimilarityResult {
  double basic_score;       // 基础approacher的分数
  double semantic_score;    // 语义增强后的分数
  double enhancement_boost; // 语义提升幅度 (semantic - basic)

  bool has_error() const;
  bool no_match() const;

  CompleteSimilarityResult(double basic = -2.0, double semantic = -2.0);
};

/**
 * 特征贡献度信息结构体
 */
struct FeatureContribution {
  std::string feature;         // 特征名
  double contribution_percent; // 贡献百分比

  FeatureContribution(const std::string &feat = "", double contrib = 0.0);
};

/**
 * 完整信息结果结构体 - 包含相似度和特征贡献度信息
 */
struct FullInfoResult {
  double forward_similarity;                              // 正向相似度 (A→B)
  double reverse_similarity;                              // 反向相似度 (B→A)
  double main_similarity;                                 // 主相似度 (总相似度)
  std::vector<FeatureContribution> feature_contributions; // 每个特征的贡献度

  bool has_error() const;
  bool no_match() const;

  FullInfoResult(double forward = -2.0, double reverse = -2.0,
                 double main = -2.0);
};

/**
 * 解释相似度分数的含义
 */
std::string explainScore(double score);

/**
 * 调用 approacher_lib 中的程序的封装类
 */
class ApproacherLibCaller {
private:
  std::string lib_path;

  std::pair<std::string, int> executeCommand(const std::string &command);
  double parseNumericResult(const std::string &output);
  std::vector<std::string> parseCommaInput(const std::string &input);
  ThreeScores parseThreeScores(const std::string &output);
  SimilarityScores parseDualScores(const std::string &output);

public:
  /**
   * 构造函数
   * @param lib_dir approacher_lib 目录路径
   */
  explicit ApproacherLibCaller(const std::string &lib_dir = "approacher_lib");

  /**
   * 调用基础approacher
   * @param input_a 输入A
   * @param input_b 输入B
   * @param use_fuzzy 是否使用模糊匹配
   * @param quiet_mode 静默模式（只返回数值）
   * @return 相似度值
   */
  double callApproacher(const std::string &input_a, const std::string &input_b,
                        bool use_fuzzy = false, bool quiet_mode = true);

  /**
   * 调用语义增强approacher
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return 相似度值
   */
  double callSemanticApproacher(const std::string &input_a,
                                const std::string &input_b,
                                bool quiet_mode = true);

  /**
   * 调用语义增强approacher并获取三分数结果
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return ThreeScores结构体，包含主相似度和两个方向的分相似度
   */
  ThreeScores callSemanticApproacherThree(const std::string &input_a,
                                          const std::string &input_b,
                                          bool quiet_mode = true);

  /**
   * 调用语义增强approacher并获取双分数结果（保持向后兼容）
   * @param input_a 输入A
   * @param input_b 输入B
   * @param quiet_mode 静默模式（只返回数值）
   * @return SimilarityScores结构体，包含基础分数和增强分数
   */
  SimilarityScores callSemanticApproacherDual(const std::string &input_a,
                                              const std::string &input_b,
                                              bool quiet_mode = true);

  /**
   * 获取完整相似度分析结果（三个数值）
   * @param input_a 输入A
   * @param input_b 输入B
   * @return CompleteSimilarityResult结构体，包含基础分数、语义分数和提升幅度
   */
  CompleteSimilarityResult getCompleteSimilarity(const std::string &input_a,
                                                 const std::string &input_b);

  /**
   * 获取完整特征贡献度信息
   * @param input_a 输入A
   * @param input_b 输入B
   * @return FullInfoResult结构体，包含相似度和特征贡献度信息
   */
  FullInfoResult getFullInfo(const std::string &input_a,
                             const std::string &input_b);

  /**
   * 获取详细分析结果（非静默模式）
   * @param input_a 输入A
   * @param input_b 输入B
   * @param use_semantic 是否使用语义增强
   * @return 完整输出文本
   */
  std::string getDetailedAnalysis(const std::string &input_a,
                                  const std::string &input_b,
                                  bool use_semantic = true);
};

#endif // APPROACHER_CALLER_H
