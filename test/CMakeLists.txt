cmake_minimum_required(VERSION 3.10)

add_executable(httpServer_tests
    test_http_server.cpp
    ${PROJECT_SOURCE_DIR}/HttpServer/src/http/HttpRequest.cpp
    ${PROJECT_SOURCE_DIR}/HttpServer/src/http/HttpResponse.cpp
    ${PROJECT_SOURCE_DIR}/HttpServer/src/http/HttpContext.cpp
    ${PROJECT_SOURCE_DIR}/HttpServer/src/router/Router.cpp
    ${PROJECT_SOURCE_DIR}/HttpServer/src/middleware/MiddlewareChain.cpp
)

target_include_directories(httpServer_tests PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/HttpServer/include
)

target_link_libraries(httpServer_tests
    muduo_net
    muduo_base
)

target_compile_options(httpServer_tests PRIVATE
    -Wall
    -O2
)

add_test(NAME httpServer_tests COMMAND httpServer_tests)
