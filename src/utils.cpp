#include "utils.h"
#include "common.h"
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstdlib>

// 通过随机数模拟离线模型训练所得 emb 参数
vector<double> RamdomEmbedding() {
    vector<double> emb; // 8 维 embedding
    for (int i = 0; i < EMB_DIM; ++i) {
        double v = rand() % 1000000 / 1000000.0;
        emb.push_back(v);
    }
    return emb;
}

// emb 加和池化操作（多值特征，如点击序列）
void SUM_POOLING(std::unordered_map<size_t, vector<double>> &slot_emb_map,
                 size_t slot_id,
                 const vector<double> &emb) {
    if (!slot_emb_map.count(slot_id)) {
        slot_emb_map[slot_id] = emb;
        return;
    }
    auto &old = slot_emb_map.at(slot_id);
    for (size_t i = 0; i < old.size(); ++i) {
        old[i] += emb[i];
    }
}

// emb 平均池化操作（将加和池化结果归一化，保证多值特征输出维度固定）
void AVERAGE_POOLING(const std::unordered_map<size_t, vector<double>> &sum_pooling_map,
                     std::unordered_map<size_t, vector<double>> &average_pooling_map,
                     const std::unordered_map<size_t, int> &slot_count_map) {
    for (const auto &e : sum_pooling_map) {
        size_t slot_id = e.first;
        const auto &emb = e.second;
        auto &avg_emb = average_pooling_map[slot_id];
        avg_emb.resize(emb.size());
        for (size_t i = 0; i < emb.size(); ++i) {
            avg_emb[i] = emb[i] / slot_count_map.at(slot_id);
        }
    }
}

// 向量与矩阵乘法：input(1 x m) * matrix(m x n) = res(1 x n)
vector<double> MatrixMul(const vector<double> &input, const vector<vector<double>> &matrix) {
    if (input.empty() || matrix.empty()) {
        fprintf(stderr, "input or matrix is empty!\n");
        return {};
    }

    int m = input.size();          // 向量维度（矩阵行数）
    int n = matrix[0].size();      // 矩阵列数（输出维度）
    int k = matrix.size();         // 矩阵行数
    assert(m == k && "input 维度需与矩阵行数一致");

    vector<double> res(n, 0.0);
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < k; ++j) {
            sum += input[j] * matrix[j][i];
        }
        res[i] = sum;
    }
    return res;
}

// 向量按位相加
void ADD(vector<double> &a, const vector<double> &b) {
    assert(a.size() == b.size() && "两向量维度需一致");
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] += b[i];
    }
}

// 特征哈希化函数
size_t Hash(const string &s) {
    static std::hash<string> hasher;
    return hasher(s) % MAX_DIM;
}

// 字符串分割函数
vector<string> split(const string &s, const char &sep) {
    vector<string> res;
    string word;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != sep) {
            word += s[i];
        } else {
            if (!word.empty()) res.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) res.push_back(word);
    return res;
}
