# 推荐系统服务

一个轻量级的 C++ 推荐系统服务框架，基于 DNN 的 CTR 预估。

## 功能特性

- **请求解析**：解析基于 Protobuf 的推荐请求
- **特征查询**：从特征存储中查询用户和物品特征
- **特征工程**：基于哈希的特征提取，支持时间衰减
- **Embedding 查找**：从参数服务器查找 Embedding
- **DNN 推理**：带 ReLU 激活的多层感知机
- **Top-K 排序**：返回预估 CTR 最高的 Top-K 物品

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                      OnlineServer                           │
├─────────────────────────────────────────────────────────────┤
│  请求解析 → 特征查询 → PB 样本构建                           │
│      ↓          ↓          ↓                                │
│  特征哈希 → Embedding 查找 → Embedding 拼接                  │
│      ↓          ↓          ↓                                │
│  前向传播 → Sigmoid → Top-K 响应                             │
└─────────────────────────────────────────────────────────────┘
```

## 项目结构

```
recsys_serving/
├── CMakeLists.txt          # CMake 构建配置
├── proto/
│   ├── ReqResp.proto       # 请求/响应 Protobuf 定义
│   └── Sample.proto        # 样本/特征 Protobuf 定义
├── src/
│   ├── common.h            # 常量、全局变量、结构体定义
│   ├── utils.h             # 工具函数声明
│   ├── utils.cpp           # 工具函数实现
│   ├── online_server.h     # OnlineServer 类声明
│   ├── online_server.cpp   # OnlineServer 类实现
│   └── main.cpp            # 测试数据构造和程序入口
└── README_CH.md
```

## 环境要求

- C++17 编译器
- CMake 3.16+
- Protocol Buffers

### macOS 安装

```bash
brew install cmake protobuf
```

### Ubuntu 安装

```bash
sudo apt-get install cmake libprotobuf-dev protobuf-compiler
```

## 构建

```bash
mkdir build && cd build
cmake -B . ..
make -j4
```

## 运行

```bash
./recsys_serving
```

## 输出示例

```
debug respBody: items {
  item_id: 10024
  score: 1
}
items {
  item_id: 10031
  score: 1
}
...
```

## 模型架构

DNN 模型包含 4 个全连接层：

| 层   | 输入维度 | 输出维度 | 激活函数 |
|------|----------|----------|----------|
| 1    | 104      | 128      | ReLU     |
| 2    | 128      | 64       | ReLU     |
| 3    | 64       | 32       | ReLU     |
| 4    | 32       | 1        | 无       |

- **输入**：13 个特征槽 × 8 维 Embedding = 104 维
- **输出**：CTR 概率 (0-1)

## 特征槽定义

| 槽 ID | 特征描述 |
|-------|----------|
| 1     | 用户 ID   |
| 2     | 用户年龄  |
| 3     | 用户性别  |
| 4     | 点击历史（多值特征）|
| 5     | 请求小时  |
| 6     | 请求星期几|
| 7     | 设备      |
| 8     | 网络      |
| 9     | 场景 ID   |
| 10    | IP 地址   |
| 100   | 物品 ID   |
| 101   | 物品一级类目|
| 102   | 物品二级类目|

## 处理流程

1. **请求解析**：从 Protobuf 二进制解析请求，提取用户 ID、候选物品列表等
2. **特征查询**：根据用户 ID 和物品 ID 查询特征（年龄、性别、点击序列等）
3. **样本构建**：将特征填充到 Sample Protobuf 结构
4. **特征哈希**：将特征转换为哈希值，映射到固定维度空间
5. **Embedding 查找**：根据哈希值查找 Embedding 向量
6. **池化操作**：对多值特征进行加和/平均池化
7. **Embedding 拼接**：将所有特征的 Embedding 拼接成输入向量
8. **前向传播**：通过 DNN 计算 CTR 预估分数
9. **排序返回**：按分数降序排序，返回 Top-K 结果

## 开源协议

MIT
