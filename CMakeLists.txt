cmake_minimum_required(VERSION 3.1)

project(tiny_network)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/base
    ${PROJECT_SOURCE_DIR}/src/net
    ${PROJECT_SOURCE_DIR}/src/time
    ${PROJECT_SOURCE_DIR}/src/logger
    ${PROJECT_SOURCE_DIR}/src/memory
)


# 包含这些目录下的.cc文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/time SRC_TIMER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/logger SRC_LOG)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/memory SRC_MEMORY)
aux_source_directory(. MAIN)

set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
)


add_executable(tiny_network ${MAIN} ${SRC_BASE} ${SRC_TIMER} ${SRC_NET}  ${SRC_LOG} ${SRC_MEMORY})

target_link_libraries(tiny_network pthread)