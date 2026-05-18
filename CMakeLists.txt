cmake_minimum_required(VERSION 3.22)
project(CameraDroplets LANGUAGES CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce high-performance optimizations
add_compile_options(-O3 -march=native -Wall -Wextra)

set(LOCAL_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/lib_src")
file(MAKE_DIRECTORY "${LOCAL_THIRD_PARTY_DIR}")

function(ensure_local_repo fetch_name local_dir repo_url repo_tag)
    set(local_path "${LOCAL_THIRD_PARTY_DIR}/${local_dir}")
    if(EXISTS "${local_path}")
        message(STATUS "Using local ${fetch_name} at ${local_path}")
    else()
        find_package(Git QUIET)
        if(NOT Git_FOUND)
            message(FATAL_ERROR "Missing local ${dep_name} and git is unavailable for fallback clone")
        endif()

        set(clone_cmd "${GIT_EXECUTABLE}" clone --depth 1 "${repo_url}" "${local_path}")
        if(NOT "${repo_tag}" STREQUAL "")
            list(INSERT clone_cmd 3 --branch "${repo_tag}")
        endif()

        execute_process(
            COMMAND ${clone_cmd}
            RESULT_VARIABLE clone_result
        )
        if(NOT clone_result EQUAL 0)
            message(FATAL_ERROR "Failed to clone ${fetch_name} from ${repo_url}")
        endif()
    endif()

    string(TOUPPER "${fetch_name}" dep_name_upper)
    set("FETCHCONTENT_SOURCE_DIR_${dep_name_upper}" "${local_path}" PARENT_SCOPE)
endfunction()

# Locate dependencies inside the Linux environment
# find_package(AWSSDK REQUIRED COMPONENTS s3)
find_package(librdkafka QUIET)
if(NOT librdkafka_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RDKAFKA REQUIRED rdkafka)

    add_library(librdkafka::rdkafka INTERFACE IMPORTED)
    target_include_directories(librdkafka::rdkafka INTERFACE ${RDKAFKA_INCLUDE_DIRS})
    target_link_directories(librdkafka::rdkafka INTERFACE ${RDKAFKA_LIBRARY_DIRS})
    target_link_libraries(librdkafka::rdkafka INTERFACE ${RDKAFKA_LIBRARIES})
endif()

ensure_local_repo(concurrentqueue concurrentqueue https://github.com/cameron314/concurrentqueue.git v1.0.4)
ensure_local_repo(nlohmann_json json https://github.com/nlohmann/json.git v3.12.0)
ensure_local_repo(modern_cpp_kafka modern-cpp-kafka https://github.com/morganstanley/modern-cpp-kafka.git "")

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
        avformat avcodec avutil swscale  # Core FFmpeg binary links
        # ${AWSSDK_LIBRARIES}
        librdkafka::rdkafka
        moodycamel::concurrentqueue
        nlohmann_json::nlohmann_json
        modern-cpp-kafka::modern-cpp-kafka
)
