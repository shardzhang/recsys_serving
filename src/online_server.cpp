#include "online_server.h"
#include "utils.h"
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <cstdio>

using reco_pb::ReqBody;
using reco_pb::RespBody;

// ============ Request Parser 模块 ============
int OnlineServer::RequestParser(const string &req) {
    request = req;
    ReqBody req_body;
    if (!req_body.ParseFromString(req)) {
        fprintf(stderr, "request parse failed!\n");
        return request_parse_error;
    }
    user_info.user_id = req_body.user_id();
    context.reqid = req_body.reqid();
    context.reqdt = req_body.reqdt();
    context.device = req_body.device();
    context.network = req_body.network();
    context.scene_id = req_body.scene_id();
    context.ip = req_body.ip();
    item_infos.clear();
    for (const auto &item : req_body.items()) {
        item_infos.emplace_back(item.item_id(), 0.0);
    }
    if (item_infos.empty()) {
        fprintf(stderr, "candidates empty!");
        return request_parse_error;
    }
    return OK;
}

// ============ Feature Query 模块 ============
int OnlineServer::FeatureQuery() {
    // 1. 查询用户特征（每条请求仅查一次）
    auto user_key = std::to_string(user_info.user_id);
    if (!feature_map.count(user_key)) {
        fprintf(stderr, "not find user feature!");
        return query_user_feature_error;
    }
    const auto &user_feature = feature_map.at(user_key);
    vector<string> user_fields = split(user_feature, '$');
    if (user_fields.size() < 3) {
        fprintf(stderr, "user feature format error!");
        return query_user_feature_error;
    }
    user_info.age = user_fields[0];
    user_info.sex = user_fields[1];
    string click_list = user_fields[2];
    user_info.click_list.clear();
    for (const auto &e : split(click_list, ',')) {
        vector<string> elem = split(e, ':');
        if (elem.size() < 3) continue;
        user_info.click_list.push_back({elem[0], elem[1], elem[2]});
    }

    // 2. 查询物品特征（每条物品查一次）
    for (auto &item : item_infos) {
        auto item_key = std::to_string(item.item_id);
        if (!feature_map.count(item_key)) {
            fprintf(stderr, "not find item feature, item_id=%d!\n", item.item_id);
            continue;
        }
        const auto &item_feature = feature_map.at(item_key);
        vector<string> item_fields = split(item_feature, '$');
        if (item_fields.size() < 2) continue;
        item.item_feature.first_type = item_fields[0];
        item.item_feature.second_type = item_fields[1];
    }
    return OK;
}

// ============ PB Example 模块 ============
void OnlineServer::ConstructPbExamples(vector<reco_pb::Sample> &samples) {
    for (int i = 0; i < item_infos.size() && i < samples.size(); ++i) {
        ::reco_pb::Sample &sample = samples[i];
        const ItemInfo &item = item_infos[i];

        // Fill user feature
        ::reco_pb::UserFeature *user_feature = sample.mutable_user_feature();
        user_feature->set_user_id(user_info.user_id);
        user_feature->set_age(user_info.age);
        user_feature->set_sex(user_info.sex);
        auto click_list = user_feature->mutable_user_click_list();
        for (const auto &list : user_info.click_list) {
            auto elem = click_list->add_click_list();
            elem->set_item_id(stoi(list[0]));
            elem->set_cnt(stoi(list[1]));
            elem->set_timestamp(stoi(list[2]));
        }

        // Fill context feature
        ::reco_pb::ContextFeature *context_feature = sample.mutable_context_feature();
        context_feature->set_reqid(context.reqid);
        context_feature->set_reqdt(context.reqdt);
        context_feature->set_device(context.device);
        context_feature->set_network(context.network);
        context_feature->set_scene_id(context.scene_id);
        context_feature->set_ip(context.ip);

        // Fill item feature
        ::reco_pb::ItemFeature *item_feature = sample.add_item_features();
        item_feature->set_item_id(item.item_id);
        item_feature->set_first_type(item.item_feature.first_type);
        item_feature->set_second_type(item.item_feature.second_type);
    }
}

// ============ Feature Extract 模块 ============
void OnlineServer::OneHashExample(const reco_pb::Sample &sample,
                                  reco_pb::HashSample *hash_sample) {
    // Part1. 填充用户和上下文哈希特征值，并为每个特征域编号（slot_id）
    ::reco_pb::HashSample_User *user = hash_sample->mutable_user();

    // user_id，slot=1
    user->add_slot_id(1);
    user->add_hash_val(Hash(std::to_string(sample.user_feature().user_id())));

    // user_age，slot=2
    user->add_slot_id(2);
    user->add_hash_val(Hash(sample.user_feature().age()));

    // user_sex，slot=3
    user->add_slot_id(3);
    user->add_hash_val(Hash(sample.user_feature().sex()));

    // user_click_list，slot=4（多值特征：逐元素哈希 + 时间衰减 + 分桶）
    for (const auto &elem : sample.user_feature().user_click_list().click_list()) {
        user->add_slot_id(4);
        int item_id = elem.item_id();
        int cnt = elem.cnt();
        int timestamp = elem.timestamp();
        double duration = (time(nullptr) - timestamp) / 3600.0 / 24.0;
        double decay = UserBehaviorTimeDecay(duration);
        int bucket = (int)(round(3 * log2(1 + cnt * decay)));
        string result = std::to_string(item_id) + "_" + std::to_string(bucket);
        user->add_hash_val(Hash(result));
    }

    // context_reqdt_hour，slot=5
    user->add_slot_id(5);
    time_t reqdt = sample.context_feature().reqdt();
    user->add_hash_val(Hash(std::to_string(localtime(&reqdt)->tm_hour)));

    // context_reqdt_weekday，slot=6
    user->add_slot_id(6);
    user->add_hash_val(Hash(std::to_string(localtime(&reqdt)->tm_wday)));

    // device，slot=7
    user->add_slot_id(7);
    user->add_hash_val(Hash(sample.context_feature().device()));

    // network，slot=8
    user->add_slot_id(8);
    user->add_hash_val(Hash(sample.context_feature().network()));

    // scene_id，slot=9
    user->add_slot_id(9);
    user->add_hash_val(Hash(sample.context_feature().scene_id()));

    // ip，slot=10
    user->add_slot_id(10);
    user->add_hash_val(Hash(sample.context_feature().ip()));

    // Part2. 填充物品哈希特征值
    ::reco_pb::HashSample_Item *item = hash_sample->mutable_item();
    const ::reco_pb::ItemFeature &item_feature = sample.item_features(0);

    // item_id，slot=100
    item->add_slot_id(100);
    item->add_hash_val(Hash(std::to_string(item_feature.item_id())));

    // item_first_type，slot=101
    item->add_slot_id(101);
    item->add_hash_val(Hash(item_feature.first_type()));

    // item_second_type，slot=102
    item->add_slot_id(102);
    item->add_hash_val(Hash(item_feature.second_type()));
}

// ============ Embedding Lookup 模块 ============
void OnlineServer::EmbeddingLookup(const reco_pb::HashSample &hash_sample,
                                   std::unordered_map<size_t, vector<double>> &average_pooling_map) {
    std::unordered_map<size_t, vector<double>> slot_emb_map;
    std::unordered_map<size_t, int> slot_count_map;

    // 用户 + 上下文部分
    const ::reco_pb::HashSample_User &user = hash_sample.user();
    for (int i = 0; i < user.hash_val().size(); ++i) {
        size_t slot_id = user.slot_id(i);
        size_t hash = user.hash_val(i);
        vector<double> emb = emb_lookup_table.count(hash)
            ? emb_lookup_table.at(hash)
            : RamdomEmbedding();
        SUM_POOLING(slot_emb_map, slot_id, emb);
        slot_count_map[slot_id]++;
    }

    // 物品部分
    const ::reco_pb::HashSample_Item &item = hash_sample.item();
    for (int i = 0; i < item.hash_val().size(); ++i) {
        size_t slot_id = item.slot_id(i);
        size_t hash = item.hash_val(i);
        vector<double> emb = emb_lookup_table.count(hash)
            ? emb_lookup_table.at(hash)
            : RamdomEmbedding();
        SUM_POOLING(slot_emb_map, slot_id, emb);
        slot_count_map[slot_id]++;
    }

    // 对多值特征做平均池化，固定输入 emb 维度。
    AVERAGE_POOLING(slot_emb_map, average_pooling_map, slot_count_map);
}

// ============ Embedding Concat 模块 ============
void OnlineServer::EmbeddingConcate(const std::unordered_map<size_t, vector<double>> &average_pooling_map,
                                    vector<double> &input_embedding) {
    for (const auto &e : average_pooling_map) {
        size_t slot_id = e.first;
        const vector<double> &emb = e.second;
        if (!slot_pos_map.count(slot_id)) {
            continue;
        }
        size_t pos = slot_pos_map.at(slot_id);
        for (size_t k = pos; k < pos + EMB_DIM; ++k) {
            input_embedding[k] = emb[k - pos];
        }
    }
}

// ============ Feed NN Model && Predict 模块 ============
double OnlineServer::ForwardPropagation(const vector<double> &input_embedding,
                                        const vector<vector<vector<double>>> &weights,
                                        const vector<vector<double>> &bias) {
    vector<double> input = input_embedding;
    vector<double> output;
    for (int i = 0; i < weights.size(); ++i) {
        output = MatrixMul(input, weights[i]);
        ADD(output, bias[i]);
        if (i != weights.size() - 1) {
            for (auto &v : output) {
                v = std::max(0.0, v); // ReLU
            }
        }
        input = output;
    }
    assert(output.size() == 1 && "最后一层输出维度应为 1");
    return output[0];
}

// ============ Monitor && Log 模块 ============
void OnlineServer::MonitorReport() {
    if (monitor.candidates_num > 0) {
        monitor.avg_score = monitor.sum_score / monitor.candidates_num;
    }
}

// ============ Response 模块 ============
bool OnlineServer::SetResponse(string &resp) {
    std::sort(item_infos.begin(), item_infos.end(), [](const ItemInfo &a, const ItemInfo &b) {
        return a.score > b.score;
    });

    RespBody resp_body;
    int cnt = 0;
    for (const auto &iteminfo : item_infos) {
        if (cnt >= result_size) break;
        ++cnt;
        ::reco_pb::ItemInfo *item = resp_body.add_items();
        item->set_item_id(iteminfo.item_id);
        item->set_score(iteminfo.score);
    }
    return resp_body.SerializeToString(&resp);
}

// ============ 主处理流程 ============
int OnlineServer::Process() {
    // Part1. Request Parser
    if (RequestParser(request) != OK) {
        fprintf(stderr, "RequestParser error!\n");
        return request_parse_error;
    }

    // Part2. Feature Query
    if (FeatureQuery() != OK) {
        fprintf(stderr, "FeatureQuery error!\n");
        return query_user_feature_error;
    }

    // Part3. PB Example
    vector<reco_pb::Sample> samples(item_infos.size());
    ConstructPbExamples(samples);

    // Part4-7. 对 N 个候选循环预估（可多线程并行）
    for (int idx = 0; idx < samples.size(); ++idx) {
        const auto &sample = samples[idx];

        reco_pb::HashSample hash_sample;
        OneHashExample(sample, &hash_sample);

        std::unordered_map<size_t, vector<double>> average_pooling_map;
        EmbeddingLookup(hash_sample, average_pooling_map);

        vector<double> input_embedding(slot_pos_map.size() * EMB_DIM, 0.0);
        EmbeddingConcate(average_pooling_map, input_embedding);

        double logit = ForwardPropagation(input_embedding, nn_weights, nn_bias);
        double pctr = Sigmoid(logit);
        item_infos[idx].score = pctr;

        monitor.sum_score += pctr;
        monitor.candidates_num++;
    }

    // Part8. Monitor && Log
    MonitorReport();

    // Part9. Response
    if (!SetResponse(response)) {
        fprintf(stderr, "SetResponse error!\n");
        return response_error;
    }
    return OK;
}
