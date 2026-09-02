# RecSys Serving

A lightweight C++ recommendation system serving framework with DNN-based CTR prediction.

## Features

- **Request Parsing**: Parse protobuf-based recommendation requests
- **Feature Query**: Query user and item features from feature store
- **Feature Engineering**: Hash-based feature extraction with time decay
- **Embedding Lookup**: Lookup embeddings from parameter server
- **DNN Inference**: Multi-layer perceptron with ReLU activation
- **Top-K Ranking**: Return top-K items by predicted CTR

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      OnlineServer                           │
├─────────────────────────────────────────────────────────────┤
│  Request Parser → Feature Query → PB Example Construction   │
│         ↓              ↓              ↓                     │
│  Feature Hash → Embedding Lookup → Embedding Concat         │
│         ↓              ↓              ↓                     │
│  Forward Propagation → Sigmoid → Top-K Response             │
└─────────────────────────────────────────────────────────────┘
```

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
cmake ..
make -j4
```

## Run

```bash
./recsys_serving
```

## Example Output

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

## License

MIT
