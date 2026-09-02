#include "common.h"
#include "online_server.h"
#include "utils.h"
#include "ReqResp.pb.h"
#include "Sample.pb.h"
#include <ctime>
#include <cstdio>
#include <cstdlib>

using reco_pb::ReqBody;
using reco_pb::RespBody;

// 全局变量定义
std::unordered_map<string, string> feature_map;
std::unordered_map<size_t, vector<double>> emb_lookup_table;
std::unordered_map<size_t, size_t> slot_pos_map;
vector<vector<vector<double>>> nn_weights;
vector<vector<double>> nn_bias;

// =========================== 测试运行 ===========================

int run() {
    // Part1. 准备测试数据

    // Construct Request
    string request;
    {
        ReqBody req_body;
        req_body.set_reqid("238cafd823k937dfaxkg");
        req_body.set_reqdt(time(nullptr));
        req_body.set_user_id(86001);
        req_body.set_device("A10086");
        req_body.set_network("5G");
        req_body.set_scene_id("101");
        req_body.set_ip("255.255.255.0");

        // 构造 100 个候选物品（10001 ~ 10100）
        for (int item_id = 10001; item_id <= 10100; ++item_id) {
            auto item = req_body.add_items();
            item->set_item_id(item_id);
        }
        if (!req_body.SerializeToString(&request)) {
            fprintf(stderr, "request error!\n");
            return -1;
        }
    }

    // Construct Feature
    {
        feature_map.insert({"86001", "18$male$10001:7:1696243092,10010:12:1696103837,100107:2:1692802411"});
        for (int i = 10001; i <= 10100; ++i) {
            string appid = std::to_string(i);
            int first_type = rand() % 20 + 100;
            int second_type = rand() % 20 + 80;
            char item_feature[20];
            std::snprintf(item_feature, 20, "%i$%i", first_type, second_type);
            feature_map.insert({appid, item_feature});
        }
    }

    // Construct Embedding
    {
        for (int i = 0; i < 1000; ++i) {
            size_t hash = rand() % MAX_DIM;
            const vector<double> &emb = RamdomEmbedding();
            emb_lookup_table.insert({hash, emb});
        }
    }

    // Construct slot pos
    {
        // 13 个 feature field（10 用户/上下文 + 3 物品）
        vector<size_t> slots = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 101, 102};
        size_t idx = 0;
        for (size_t slot : slots) {
            slot_pos_map[slot] = idx;
            idx += EMB_DIM;
        }
    }

    // Construct DNN weights + bias
    {
        // DNN 共 4 层：input(104) -> 128 -> 64 -> 32 -> 1
        vector<int> layers = {128, 64, 32, 1};
        int input_dim = slot_pos_map.size() * EMB_DIM;
        for (int k = 0; k < 4; ++k) {
            // Xavier 初始化：scale = sqrt(6 / (fan_in + fan_out))
            double scale = sqrt(6.0 / (input_dim + layers[k]));
            vector<vector<double>> mat;
            for (int i = 0; i < input_dim; ++i) {
                vector<double> weights(layers[k], 0.0);
                for (int j = 0; j < layers[k]; ++j) {
                    weights[j] = ((rand() % 10000) / 5000.0 - 1.0) * scale;
                }
                mat.push_back(weights);
            }
            input_dim = layers[k];
            nn_weights.push_back(mat);
        }

        for (int k = 0; k < 4; ++k) {
            vector<double> bias(layers[k], 0.0);
            nn_bias.push_back(bias);
        }
    }

    // Part2. 启动服务（返回 Top30）
    OnlineServer online_server(request, 30);
    online_server.Process();

    // Part3. 打印结果
    {
        const string &response = online_server.GetResponse();
        ::reco_pb::RespBody respBody;
        respBody.ParseFromString(response);
        printf("debug respBody: %s\n", respBody.DebugString().c_str());
    }

    return 0;
}

int main() {
    std::srand(time(nullptr));
    return run();
}
