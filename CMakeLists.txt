cmake_minimum_required(VERSION 3.22)
project(CameraDroplets LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce high-performance optimizations
add_compile_options(-O3 -march=native -Wall -Wextra)

# Locate dependencies inside the Linux environment
find_package(AWSSDK REQUIRED COMPONENTS s3)
find_package(librdkafka REQUIRED)
find_package(nlohmann_json REQUIRED)

# Explicitly handle FFmpeg raw variables if standard config targets aren't present
find_program(FFMPEG_EXEC ffmpeg REQUIRED)

# Since moodycamel is header-only, tell CMake where to find its paths manually
# if a full CMake package config file isn't present
find_path(MOODYCAMEL_INCLUDE_DIR NAMES concurrentqueue.h PATHS /usr/local/include/moodycamel)

add_executable(camdrops src/main.cpp)

target_include_directories(camdrops 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MOODYCAMEL_INCLUDE_DIR}
)

target_link_libraries(camdrops
    PRIVATE
        avformat avcodec avutil swscale  # Core FFmpeg binary links
        ${AWSSDK_LIBRARIES}
        librdkafka::rdkafka
        modern-cpp-kafka::modern-cpp-kafka
        nlohmann_json::nlohmann_json
)