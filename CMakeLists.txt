cmake_minimum_required(VERSION 3.10.0)

project(AiAssistant_inCpp VERSION 0.1.0 LANGUAGES C CXX)

# =========================
# C++ Standard
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Build Type
# =========================
set(CMAKE_BUILD_TYPE Release)

# =========================
# Find Packages
# =========================
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenCV REQUIRED)

# =========================
# Find Libraries
# =========================
find_library(MYSQLCPPCONN_LIBRARY
    NAMES mysqlcppconn mysqlcppconn8
    PATHS /usr/lib /usr/lib64 /usr/local/lib
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    PATHS /usr/lib /usr/lib64 /usr/local/lib
)

# =========================
# Include Directories
# =========================
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src

    ${PROJECT_SOURCE_DIR}/HttpServer/include
    ${PROJECT_SOURCE_DIR}/AIApps/ChatServer/include

    /usr/include/mysql-cppconn-8
    /usr/include/mysql

    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}

    /usr/local/include
    ${OpenCV_INCLUDE_DIRS}
)

# =========================
# Source Files
# =========================

# Main
set(MAIN_SRC
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# Http Server
file(GLOB_RECURSE HTTP_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/HttpServer/src/*.cpp"
)

# Chat Server
file(GLOB_RECURSE CHAT_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/AIApps/ChatServer/src/*.cpp"
)

# Project Source
file(GLOB_RECURSE PROJECT_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# =========================
# Executable
# =========================
add_executable(AiAssistant_inCpp
    ${MAIN_SRC}
    ${PROJECT_SRC}
    ${HTTP_SERVER_SRC}
    ${CHAT_SERVER_SRC}
)

# =========================
# Link Libraries
# =========================
target_link_libraries(AiAssistant_inCpp

    # Thread
    pthread

    # Muduo
    muduo_net
    muduo_base

    # MySQL
    mysqlcppconn
    mysqlclient

    # OpenSSL
    ssl
    crypto

    # CURL
    ${CURL_LIBRARIES}

    # OpenCV
    ${OpenCV_LIBS}

    # ONNX Runtime
    ${ONNXRUNTIME_LIBRARY}

    # RabbitMQ
    SimpleAmqpClient
    rabbitmq
)

# =========================
# Link Directories
# =========================
link_directories(/usr/local/lib)

# =========================
# Compile Options
# =========================
target_compile_options(AiAssistant_inCpp PRIVATE
    -Wall
    -O2
)

# =========================
# Testing
# =========================
include(CTest)
enable_testing()

# =========================
# CPack
# =========================
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# =========================
# Debug Messages
# =========================
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")

message(STATUS "OpenCV Include Dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")

message(STATUS "ONNX Runtime Library: ${ONNXRUNTIME_LIBRARY}")
message(STATUS "MySQL Connector Library: ${MYSQLCPPCONN_LIBRARY}")

message(STATUS "Main Source: ${MAIN_SRC}")
message(STATUS "Project Sources: ${PROJECT_SRC}")
message(STATUS "HTTP Server Sources: ${HTTP_SERVER_SRC}")
message(STATUS "Chat Server Sources: ${CHAT_SERVER_SRC}")

