#ifndef RECSYS_SERVING_COMMON_H
#define RECSYS_SERVING_COMMON_H

#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;

const int MAX_DIM = 1L << 10; // 特征空间维度 1024
const int EMB_DIM = 8;        // 特征 emb 维度 8

// global feature data
// 特征数据。实际应存储在 redis 中，这里通过全局变量模拟
extern std::unordered_map<string, string> feature_map;

// global embedding data
// emb 参数。实际应存储在 PS（Parameter Server）中，这里通过全局变量模拟
extern std::unordered_map<size_t, vector<double>> emb_lookup_table;

// global embedding pos config
// slot_id 对应 input_emb 位置。实际应从配置 conf 解析加载，这里通过全局变量模拟
extern std::unordered_map<size_t, size_t> slot_pos_map;

// global NN weights
// DNN 各全连接层权重。实际应从离线模型解析加载，这里通过全局变量模拟
// nn_weights[k] 表示从第 k-1 层到第 k 层的全连接权重，形状为 [input_dim][output_dim]
extern vector<vector<vector<double>>> nn_weights;

// global NN bias
// DNN 各全连接层偏置，nn_bias[k] 表示第 k 层偏置，长度 = 该层输出维度
extern vector<vector<double>> nn_bias;

// 错误码定义：约定 0 表示成功，非 0 表示各类错误
typedef enum {
  OK = 0,
  request_parse_error = 1,
  query_user_feature_error = 2,
  query_item_feature_error = 3,
  logic_error = 4,
  response_error = 5,
} ERROR_CODE;

// 上下文信息
struct Context {
  string reqid;
  int reqdt;
  string device;
  string network;
  string scene_id;
  string ip;
};

// 物品信息
struct ItemInfo {
  struct ItemFeature {
    string first_type;
    string second_type;
  };

  ItemInfo(int item_id, double score = 0) : item_id(item_id), score(score) {}
  int item_id;
  double score;              // 物品打分（预估结果）
  ItemFeature item_feature;  // 物品特征
};

// 用户信息
struct UserInfo {
  UserInfo(int user_id = 0) : user_id(user_id) {}
  int user_id;                          // id
  string age;                           // 年龄特征
  string sex;                           // 性别特征
  vector<vector<string>> click_list;    // 历史点击行为序列 [item_id, cnt, timestamp]
};

// 监控信息
struct Monitor {
  double sum_score = 0.0;   // 累积打分
  double avg_score = 0.0;   // 平均打分
  int candidates_num = 0;   // 候选物品个数
  double elapsed = 0.0;     // 计算耗时
};

#endif // RECSYS_SERVING_COMMON_H
