# HttpServer 测试说明

## 新增内容

- `test/`：HttpServer 测试目录
- `test/CMakeLists.txt`：测试专用 CMake 配置
- `test/test_http_server.cpp`：HttpServer 核心单元测试

## 当前测试覆盖点

- `HttpContext`：解析 GET 请求、POST 请求体
- `HttpResponse`：响应报文序列化
- `Router`：静态路由、动态路由匹配
- `MiddlewareChain`：请求前置和响应后置处理

这些测试重点覆盖了 HttpServer 的核心 HTTP 行为，不依赖真实监听端口启动服务。

## 运行步骤

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build --target httpServer_tests
ctest --test-dir build --output-on-failure -R httpServer_tests
```

## 运行过程记录

本次测试接入后的标准执行流程如下：

1. 使用 `cmake -S . -B build` 重新生成工程。
2. 使用 `cmake --build build --target httpServer_tests` 单独编译测试目标。
3. 使用 `ctest --test-dir build --output-on-failure -R httpServer_tests` 运行 `httpServer_tests`。

如果你的环境里已经正确安装 `muduo`，上述命令会直接编译并运行测试。

## 说明

- 根 `CMakeLists.txt` 已增加 `add_subdirectory(test)`，测试会随工程一起生成。
- 测试目标没有引入额外测试框架，直接使用普通 C++ 可执行文件配合 `CTest`。
- 如果后续你希望覆盖 `HttpServer::onRequest()` 或真实 socket 请求流程，建议下一步补一个集成测试目标，并先整理 `HttpServer.cpp` 现有实现中的编译问题。
