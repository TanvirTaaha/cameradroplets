cmake_minimum_required(VERSION 3.22)
project(CameraDroplets LANGUAGES CXX VERSION 0.1.0)
set(PROJECT_AUTHOR "Tanvir Taaha <tanvir.taaha@gmail.com>")

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Enforce high-performance optimizations
add_compile_options(-O3 -march=native -Wall -Wextra)

set(LOCAL_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/lib_src")

# Locate dependencies inside the Linux environment
find_package(PkgConfig REQUIRED)
find_package(AWSSDK REQUIRED COMPONENTS s3)

pkg_check_modules(RDKAFKA REQUIRED rdkafka)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale)

add_library(librdkafka::rdkafka INTERFACE IMPORTED)
target_include_directories(librdkafka::rdkafka INTERFACE ${RDKAFKA_INCLUDE_DIRS})
target_link_directories(librdkafka::rdkafka INTERFACE ${RDKAFKA_LIBRARY_DIRS})
target_link_libraries(librdkafka::rdkafka INTERFACE ${RDKAFKA_LIBRARIES})



set(CONCURRENTQUEUE_LOCAL_DIR "${LOCAL_THIRD_PARTY_DIR}/concurrentqueue")
set(NLOHMANN_JSON_LOCAL_DIR "${LOCAL_THIRD_PARTY_DIR}/json")
set(MODERN_CPP_KAFKA_LOCAL_DIR "${LOCAL_THIRD_PARTY_DIR}/modern-cpp-kafka")

if(EXISTS "${CONCURRENTQUEUE_LOCAL_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_CONCURRENTQUEUE "${CONCURRENTQUEUE_LOCAL_DIR}")
    message(STATUS "Using local concurrentqueue at ${CONCURRENTQUEUE_LOCAL_DIR}")
endif()

if(EXISTS "${NLOHMANN_JSON_LOCAL_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON "${NLOHMANN_JSON_LOCAL_DIR}")
    message(STATUS "Using local nlohmann_json at ${NLOHMANN_JSON_LOCAL_DIR}")
endif()

if(EXISTS "${MODERN_CPP_KAFKA_LOCAL_DIR}")
    set(FETCHCONTENT_SOURCE_DIR_MODERN_CPP_KAFKA "${MODERN_CPP_KAFKA_LOCAL_DIR}")
    message(STATUS "Using local modern_cpp_kafka at ${MODERN_CPP_KAFKA_LOCAL_DIR}")
endif()

FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
    GIT_TAG v1.0.4
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_Declare(
    modern_cpp_kafka
    GIT_REPOSITORY https://github.com/morganstanley/modern-cpp-kafka.git
)

FetchContent_MakeAvailable(concurrentqueue nlohmann_json modern_cpp_kafka)

add_library(moodycamel::concurrentqueue INTERFACE IMPORTED)
target_include_directories(
    moodycamel::concurrentqueue
    INTERFACE
        ${concurrentqueue_SOURCE_DIR}
)

if(NOT TARGET modern-cpp-kafka::modern-cpp-kafka)
    add_library(modern-cpp-kafka::modern-cpp-kafka INTERFACE IMPORTED)
    target_include_directories(
        modern-cpp-kafka::modern-cpp-kafka
        INTERFACE
            ${modern_cpp_kafka_SOURCE_DIR}/include
    )
    target_link_libraries(
        modern-cpp-kafka::modern-cpp-kafka
        INTERFACE
            librdkafka::rdkafka
    )
endif()

# Explicitly handle FFmpeg raw variables if standard config targets aren't present
find_program(FFMPEG_EXEC ffmpeg REQUIRED)

add_executable(camdrops src/main.cpp)

target_include_directories(camdrops 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(camdrops
    PRIVATE
        PkgConfig::FFMPEG
        ${AWSSDK_LIBRARIES}
        librdkafka::rdkafka
        moodycamel::concurrentqueue
        nlohmann_json::nlohmann_json
        modern-cpp-kafka::modern-cpp-kafka
)
