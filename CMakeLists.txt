cmake_minimum_required(VERSION 3.16)
project(recsys_serving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
if(NOT Protobuf_FOUND AND NOT PROTOBUF_FOUND)
    message(FATAL_ERROR "protobuf not found, install via: brew install protobuf")
endif()

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)

# protoc 生成 pb.cc / pb.h
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
set(PROTO_FILES
    ${PROTO_SRC_DIR}/ReqResp.proto
    ${PROTO_SRC_DIR}/Sample.proto
)
set(GENERATED_PROTO_SRCS
    ${PROTO_GEN_DIR}/ReqResp.pb.cc
    ${PROTO_GEN_DIR}/Sample.pb.cc
)
set(GENERATED_PROTO_HDRS
    ${PROTO_GEN_DIR}/ReqResp.pb.h
    ${PROTO_GEN_DIR}/Sample.pb.h
)

add_custom_command(
    OUTPUT ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --cpp_out=${PROTO_GEN_DIR}
            -I ${PROTO_SRC_DIR}
            ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Generating protobuf sources"
)

add_custom_target(proto_gen DEPENDS ${GENERATED_PROTO_SRCS})

add_executable(recsys_serving
    src/main.cpp
    src/utils.cpp
    src/online_server.cpp
    ${GENERATED_PROTO_SRCS}
)
add_dependencies(recsys_serving proto_gen)
target_include_directories(recsys_serving PRIVATE ${PROTO_GEN_DIR})
target_link_libraries(recsys_serving PRIVATE protobuf::libprotobuf)