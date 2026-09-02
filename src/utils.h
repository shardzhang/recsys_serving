#ifndef RECSYS_SERVING_UTILS_H
#define RECSYS_SERVING_UTILS_H

#include <string>
#include <vector>

using std::string;
using std::vector;

// 激活函数
inline double Sigmoid(double logit) {
    return 1.0 / (1 + exp(-1.0 * logit));
}

// 通过随机数模拟离线模型训练所得 emb 参数
vector<double> RamdomEmbedding();

// emb 加和池化操作（多值特征，如点击序列）
void SUM_POOLING(std::unordered_map<size_t, vector<double>> &slot_emb_map,
                 size_t slot_id,
                 const vector<double> &emb);

// emb 平均池化操作（将加和池化结果归一化，保证多值特征输出维度固定）
void AVERAGE_POOLING(const std::unordered_map<size_t, vector<double>> &sum_pooling_map,
                     std::unordered_map<size_t, vector<double>> &average_pooling_map,
                     const std::unordered_map<size_t, int> &slot_count_map);

// 向量与矩阵乘法：input(1 x m) * matrix(m x n) = res(1 x n)
vector<double> MatrixMul(const vector<double> &input, const vector<vector<double>> &matrix);

// 向量按位相加
void ADD(vector<double> &a, const vector<double> &b);

// 特征哈希化函数
size_t Hash(const string &s);

// 用户行为时间衰减：距今 day 天，衰减系数 = 2^(-day / half_period)
inline double UserBehaviorTimeDecay(double day, double half_period = 7) {
    return std::pow(2, -1.0 * day / half_period);
}

// 字符串分割函数
vector<string> split(const string &s, const char &sep = ',');

#endif // RECSYS_SERVING_UTILS_H
