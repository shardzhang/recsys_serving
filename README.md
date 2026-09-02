# RecSys Serving

A lightweight C++ recommendation system serving framework with DNN-based CTR prediction.

## Features

- **Request Parsing**: Parse protobuf-based recommendation requests
- **Feature Query**: Query user and item features from feature store
- **Feature Engineering**: Hash-based feature extraction with time decay
- **Embedding Lookup**: Lookup embeddings from parameter server (simulated with local memory in demo)
- **DNN Inference**: Multi-layer perceptron with ReLU activation
- **Top-K Ranking**: Return top-K items by predicted CTR

## Project Structure

```
recsys_serving/
├── CMakeLists.txt          # CMake build configuration
├── proto/
│   ├── ReqResp.proto       # Request/Response protobuf definition
│   └── Sample.proto        # Sample/Feature protobuf definition
├── src/
│   ├── common.h            # Constants, global variables, structs
│   ├── utils.h             # Utility function declarations
│   ├── utils.cpp           # Utility function implementations
│   ├── online_server.h     # OnlineServer class declaration
│   ├── online_server.cpp   # OnlineServer class implementation
│   └── main.cpp            # Test data construction and entry point
└── README.md
```

## Prerequisites

- C++17 compiler
- CMake 3.16+
- Protocol Buffers

### Install on macOS

```bash
brew install cmake protobuf
```

### Install on Ubuntu

```bash
sudo apt-get install cmake libprotobuf-dev protobuf-compiler
```

## Build

```bash
mkdir build && cd build
cmake -B . ..
make -j4
```

## Run

```bash
./recsys_serving
```

## Example Output

```
debug respBody: items {
  item_id: 10100
  score: 0.537841037694204
}
items {
  item_id: 10005
  score: 0.53316884944500553
}
items {
  item_id: 10080
  score: 0.53283963462103945
}
...
```

## Model Architecture

The DNN model has 4 fully-connected layers:

| Layer | Input Dim | Output Dim | Activation |
|-------|-----------|------------|------------|
| 1     | 104       | 128        | ReLU       |
| 2     | 128       | 64         | ReLU       |
| 3     | 64        | 32         | ReLU       |
| 4     | 32        | 1          | None       |

- **Input**: 13 feature slots × 8 embedding dimensions = 104
- **Output**: CTR probability (0-1)
- **Weight Initialization**: Xavier initialization (`scale = sqrt(6 / (fan_in + fan_out))`)
- **Bias Initialization**: Zero

## Feature Slots

| Slot ID | Feature Description |
|---------|---------------------|
| 1       | User ID             |
| 2       | User Age            |
| 3       | User Sex            |
| 4       | Click History (multi-value) |
| 5       | Request Hour        |
| 6       | Request Weekday     |
| 7       | Device              |
| 8       | Network             |
| 9       | Scene ID            |
| 10      | IP Address          |
| 100     | Item ID             |
| 101     | Item First Type     |
| 102     | Item Second Type    |

## Processing Flow

1. **Request Parsing**: Parse protobuf request to extract user ID and candidate item list
2. **Feature Query**: Query user and item features (age, sex, click history, etc.)
3. **Sample Construction**: Fill features into Sample protobuf structure
4. **Feature Hashing**: Convert features to hash values, mapping to fixed dimension space
5. **Embedding Lookup**: Lookup embedding vectors by hash values
6. **Pooling**: Sum/average pooling for multi-value features
7. **Embedding Concat**: Concatenate all feature embeddings into input vector
8. **Forward Propagation**: Calculate CTR prediction score through DNN
9. **Sort & Return**: Sort by score in descending order, return Top-K results

## License

MIT
