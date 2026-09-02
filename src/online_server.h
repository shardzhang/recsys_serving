#ifndef RECSYS_SERVING_ONLINE_SERVER_H
#define RECSYS_SERVING_ONLINE_SERVER_H

#include "common.h"
#include "ReqResp.pb.h"
#include "Sample.pb.h"

class OnlineServer {
 public:
  OnlineServer(string request, int k) : request(request), result_size(k) {}
  ~OnlineServer() {}

 private:
  Context context;                       // 上下文特征
  UserInfo user_info;                    // 用户特征
  vector<ItemInfo> item_infos;           // 候选物品列表
  Monitor monitor;                       // 监控
  string request;                        // 原始请求（PB 二进制）
  string response;                       // 响应（PB 二进制）
  int result_size;                       // 返回候选个数（TopK）

 public:
  const string &GetResponse() const { return response; }

 public:
  int RequestParser(const string &req);
  int FeatureQuery();
  void ConstructPbExamples(vector<reco_pb::Sample> &samples);
  void OneHashExample(const reco_pb::Sample &sample, reco_pb::HashSample *hash_sample);
  void EmbeddingLookup(const reco_pb::HashSample &hash_sample,
                       std::unordered_map<size_t, vector<double>> &average_pooling_map);
  void EmbeddingConcate(const std::unordered_map<size_t, vector<double>> &average_pooling_map,
                        vector<double> &input_embedding);
  double ForwardPropagation(const vector<double> &input_embedding,
                            const vector<vector<vector<double>>> &weights,
                            const vector<vector<double>> &bias);
  void MonitorReport();
  bool SetResponse(string &resp);
  int Process();
};

#endif // RECSYS_SERVING_ONLINE_SERVER_H
