# CppAIService 学习路线

## 1. 项目一句话定位

这是一个基于 `muduo` 自研 HTTP 服务框架之上的 C++ AI 应用服务平台，核心目标不是只做一个“能聊天”的 Demo，而是把一个真实 AI 服务端常见的关键能力串起来：

- HTTP 网络通信
- 路由分发
- Session 登录态管理
- MySQL 持久化
- RabbitMQ 异步削峰
- 多模型策略切换
- MCP 风格工具调用
- TTS / 图像识别等多模态能力

从源码结构看，它可以拆成两层：

- `HttpServer/`：通用 HTTP 框架层
- `AIApps/ChatServer/`：具体 AI 业务应用层

建议把这个项目当成一个“小型 AI 平台后端”来学，而不是单纯当成“聊天接口”。

---

## 2. 整体目录结构

建议先建立目录心智图：

```text
CppAIService/
├─ CMakeLists.txt                  # 顶层构建脚本
├─ README.md                       # 项目介绍、架构说明
├─ HttpServer/                     # 自研 HTTP 服务框架
│  ├─ include/
│  │  ├─ http/                     # HttpServer / Request / Response / Context
│  │  ├─ router/                   # 路由系统
│  │  ├─ session/                  # Session 管理
│  │  ├─ middleware/               # 中间件体系，当前重点是 CORS
│  │  ├─ ssl/                      # SSL 支持
│  │  └─ utils/                    # Json / File / Mysql / DB 连接池
│  ├─ src/
│  └─ examples/
├─ AIApps/
│  └─ ChatServer/                  # AI 业务服务
│     ├─ include/
│     │  ├─ handlers/              # 各个 HTTP 接口处理器
│     │  └─ AIUtil/                # AI 能力封装
│     ├─ src/
│     │  ├─ handlers/
│     │  ├─ AIUtil/
│     │  ├─ ChatServer.cpp
│     │  └─ main.cpp
│     └─ resource/                 # HTML 页面和配置文件
└─ images/                         # README 配图
```

---

## 3. 构建层先看什么

第一步先看两个文件：

- `README.md`
- `CMakeLists.txt`

### 3.1 `README.md` 的学习价值

这个文件不是简单介绍，它实际上已经告诉你项目的设计目标：

- 多模型适配
- RAG 预留
- MCP 风格工具调用
- 多会话隔离
- TTS / 图像识别
- RabbitMQ 异步化

所以后续读代码时，不要只盯某个 handler，而要带着“这些能力分别落在哪一层”的问题去看。

### 3.2 `CMakeLists.txt` 的学习价值

这个文件能快速告诉你项目依赖了什么：

- `muduo_net` / `muduo_base`
- `OpenSSL`
- `CURL`
- `mysqlcppconn` / `mysqlclient`
- `SimpleAmqpClient` / `rabbitmq`
- `OpenCV`
- `onnxruntime`

这一步的目标不是背构建语法，而是知道这个项目横跨了：

- 网络编程
- 数据库
- 消息队列
- 第三方 HTTP API 调用
- 本地推理/视觉能力

---

## 4. 先建立系统运行主线

真正开始读代码时，第一条主线一定是程序怎么启动。

建议阅读顺序：

1. `AIApps/ChatServer/src/main.cpp`
2. `AIApps/ChatServer/include/ChatServer.h`
3. `AIApps/ChatServer/src/ChatServer.cpp`
4. `HttpServer/include/http/HttpServer.h`
5. `HttpServer/src/http/HttpServer.cpp`

### 4.1 启动入口：`main.cpp`

`main.cpp` 做了几件关键事：

- 解析端口参数 `-p`
- 创建 `ChatServer`
- 设置线程数
- 启动前先从 MySQL 恢复历史消息 `initChatMessage()`
- 启动 RabbitMQ 消费线程池
- 最终启动 HTTP 服务

这说明本项目不是“纯内存聊天服务”，而是：

- 启动时会恢复上下文
- 写库走异步消息队列
- HTTP 服务只是最外层入口

### 4.2 业务服务壳：`ChatServer`

`ChatServer` 是整个应用层的核心协调者，内部持有：

- `http::HttpServer httpServer_`
- `http::MysqlUtil mysqlUtil_`
- 在线用户表 `onlineUsers_`
- 用户多会话聊天上下文 `chatInformation`
- 图像识别实例表 `ImageRecognizerMap`
- 用户会话 ID 列表 `sessionsIdsMap`

它的职责不是处理具体业务细节，而是负责：

- 初始化数据库连接池
- 初始化 Session 管理器
- 初始化中间件
- 注册所有路由
- 管理共享状态

这一步你要学到一个很重要的工程习惯：

业务入口对象负责“组装”，而不是把所有逻辑都塞进 `main`。

### 4.3 HTTP 主链路：`HttpServer`

`HttpServer` 是框架核心，其典型调用链是：

```text
TCP 连接建立
-> onConnection
-> onMessage
-> HttpContext 解析 HTTP 报文
-> onRequest
-> handleRequest
-> middleware before
-> router.route
-> handler.handle
-> middleware after
-> HttpResponse 回包
```

这一条链路必须读懂，因为后面所有业务接口最终都走这条路径。

---

## 5. HttpServer 框架层学习路线

这一层建议单独学习，不要一开始就冲到 AI 业务代码。

### 5.1 HTTP 报文模型

优先关注：

- `HttpServer/include/http/HttpRequest.h`
- `HttpServer/include/http/HttpResponse.h`
- `HttpServer/include/http/HttpContext.h`
- 对应 `src/http/*.cpp`

学习目标：

- 请求头、请求体、方法、路径是怎么表示的
- 响应码、响应头、响应体怎么封装
- 一个 TCP 字节流怎样被解析为完整 HTTP 请求

如果这块没吃透，后面看 Handler 容易只停留在“调接口”层面。

### 5.2 路由系统

重点文件：

- `HttpServer/include/router/Router.h`
- `HttpServer/src/router/Router.cpp`
- `HttpServer/include/router/RouterHandler.h`

这一层体现了框架抽象：

- 支持精准路由
- 支持正则/动态路由
- 支持函数回调式处理
- 支持对象式处理器 `RouterHandler`

这里建议重点理解两个设计点：

1. 为什么复杂业务更适合 `RouterHandler` 对象式封装
2. 为什么框架层把“路由匹配”和“业务处理”解耦

### 5.3 Session 管理

重点文件：

- `HttpServer/include/session/Session.h`
- `HttpServer/include/session/SessionManager.h`
- `HttpServer/include/session/SessionStorage.h`
- `HttpServer/src/session/*.cpp`

关键理解点：

- Session ID 从 Cookie 读取
- 如果没有 Session，就新建并通过 `Set-Cookie` 回写
- `SessionStorage` 是抽象存储层
- 当前 `ChatServer` 用的是 `MemorySessionStorage`

这部分很适合初学者理解“无状态 HTTP 如何构建登录态”。

### 5.4 中间件机制

重点文件：

- `HttpServer/include/middleware/Middleware.h`
- `HttpServer/include/middleware/MiddlewareChain.h`
- `HttpServer/include/middleware/cors/CorsMiddleware.h`
- `HttpServer/src/middleware/cors/CorsMiddleware.cpp`

要点：

- `before()` 在路由前执行
- `after()` 在响应前执行
- CORS 预检请求通过抛出 `HttpResponse` 的方式提前结束流程

这一步要学会：

- “横切逻辑”不应该散落在每个 handler 里
- 框架中间件可以统一处理跨域、鉴权、日志等需求

### 5.5 数据库工具层

重点文件：

- `HttpServer/include/utils/MysqlUtil.h`
- `HttpServer/include/utils/db/DbConnection.h`
- `HttpServer/include/utils/db/DbConnectionPool.h`
- `HttpServer/src/utils/db/*.cpp`

这里不是 ORM，而是比较底层的数据库访问封装。

学习重点：

- `MysqlUtil` 是轻量包装
- 真正核心是 `DbConnectionPool`
- 连接池通过 `getConnection()` 借出连接，再通过自定义析构逻辑归还
- 后台线程会定期 `ping` 和重连

这一块非常适合理解 C++ 资源管理和 RAII 思路。

### 5.6 SSL 支持

如果你是第一次学这个项目，可以先略读：

- `HttpServer/include/ssl/`
- `HttpServer/src/ssl/`

先知道框架支持 SSL 即可，等 HTTP 主流程熟悉后再回头补。

---

## 6. ChatServer 业务层学习路线

框架层搞清楚之后，再进入 `AIApps/ChatServer/`，效率会高很多。

### 6.1 先看 ChatServer 如何装配业务

重点文件：

- `AIApps/ChatServer/include/ChatServer.h`
- `AIApps/ChatServer/src/ChatServer.cpp`

重点理解三个初始化函数：

- `initializeSession()`
- `initializeMiddleware()`
- `initializeRouter()`

其中 `initializeRouter()` 是业务入口地图，直接告诉你当前项目暴露了哪些接口：

- `/login`
- `/register`
- `/user/logout`
- `/chat`
- `/chat/send`
- `/chat/history`
- `/chat/send-new-session`
- `/chat/sessions`
- `/chat/tts`
- `/upload`
- `/upload/send`

建议先用它来建立“业务能力总览”，再逐个读 handler。

### 6.2 Handler 层怎么学

建议分三批：

#### 第一批：用户与会话

- `ChatLoginHandler`
- `ChatRegisterHandler`
- `ChatLogoutHandler`

目标：

- 看懂登录注册请求怎么解析
- 看懂 Session 怎么写入 `userId`、`username`、`isLoggedIn`
- 看懂在线用户表 `onlineUsers_` 的用途

其中 `ChatLoginHandler.cpp` 很适合拿来串起：

- JSON 请求解析
- 数据库查询
- Session 写入
- JSON 响应封装

#### 第二批：聊天主流程

- `ChatHandler`
- `ChatSendHandler`
- `ChatHistoryHandler`
- `ChatCreateAndSendHandler`
- `ChatSessionsHandler`

这批是项目最核心的业务。

重点理解：

- 用户发送问题后如何找到对应 `sessionId`
- `chatInformation` 为什么是：

```cpp
unordered_map<int, unordered_map<string, shared_ptr<AIHelper>>>
```

它表达的是：

- 第一层 key：用户 ID
- 第二层 key：会话 ID
- value：该会话对应的 AI 上下文对象 `AIHelper`

这就是 README 里提到的“单用户多会话隔离”的真实落点。

#### 第三批：多模态与扩展能力

- `ChatSpeechHandler`
- `AIUploadHandler`
- `AIUploadSendHandler`
- `AIMenuHandler`

这一批体现项目不是单一聊天服务，而是往 AI 应用平台演进。

其中 `ChatSpeechHandler.cpp` 可以帮助你理解：

- 业务层怎样读取环境变量
- 怎样封装第三方 TTS 服务
- 怎样把 AI 输出继续变成语音资源 URL

---

## 7. AI 核心能力应该怎么学

这一部分是整个项目最有辨识度的地方，建议花最多时间。

### 7.1 先抓住核心对象：`AIHelper`

重点文件：

- `AIApps/ChatServer/include/AIUtil/AIHelper.h`
- `AIApps/ChatServer/src/AIUtil/AIHelper.cpp`

`AIHelper` 是单个聊天会话的核心对象，负责：

- 保存历史消息 `messages`
- 根据模型类型选择策略
- 构造请求
- 发起 CURL 调用
- 解析模型响应
- 把消息异步写入 MySQL

可以把它理解为：

> “一个会话级 AI Runtime”

也就是说，不是整个系统只有一个 AI 客户端，而是每个用户会话都有自己的上下文对象。

### 7.2 再看策略模式：`AIStrategy`

重点文件：

- `AIApps/ChatServer/include/AIUtil/AIStrategy.h`
- `AIApps/ChatServer/src/AIUtil/AIStrategy.cpp`

这里是本项目 AI 设计的关键抽象。

抽象接口统一了这些能力：

- `getApiUrl()`
- `getApiKey()`
- `getModel()`
- `buildRequest()`
- `parseResponse()`

当前已有策略：

- `AliyunStrategy`
- `DouBaoStrategy`
- `AliyunRAGStrategy`
- `AliyunMcpStrategy`

学习重点不是记住每个 API 地址，而是理解：

- 为什么不同厂商/不同能力需要不同请求格式
- 为什么“请求构造”和“响应解析”必须策略化
- 为什么这样设计后 `AIHelper` 不需要知道具体模型细节

### 7.3 工厂与注册机制：`AIFactory`

重点文件：

- `AIApps/ChatServer/include/AIUtil/AIFactory.h`
- `AIApps/ChatServer/src/AIUtil/AIFactory.cpp`

这是策略模式真正落地的补全。

项目不是写一堆 `if (modelType == ...)`，而是：

- 用 `StrategyFactory` 管理创建逻辑
- 用 `StrategyRegister<T>` 静态注册策略
- 通过字符串编号创建模型策略

这是一种非常值得学习的 C++ 工程模式：

- 扩展新模型时，少改旧代码
- 业务层通过配置/参数切模型
- 核心调用链保持稳定

### 7.4 MCP 风格工具调用：`AIToolRegistry` + `AIConfig`

重点文件：

- `AIApps/ChatServer/include/AIUtil/AIToolRegistry.h`
- `AIApps/ChatServer/src/AIUtil/AIToolRegistry.cpp`
- `AIApps/ChatServer/resource/config.json`
- `AIConfig.*`

当前实现思路是：

1. 先给模型一段提示词，问它需不需要调用工具
2. 如果模型输出 JSON 工具调用描述
3. 本地用 `AIToolRegistry` 执行工具
4. 再把工具结果喂给模型，生成最终回答

这其实就是一个轻量版的 Tool Calling / MCP 思路。

当前内置工具包括：

- `get_weather`
- `get_time`

你读这一块时，重点不是“天气接口怎么调”，而是理解：

- 模型决策和工具执行是两阶段的
- 工具注册表本质是“名称 -> 函数”的映射
- Prompt 模板配置化后，业务扩展更灵活

---

## 8. 异步消息与持久化怎么学

这是项目工程味最强的一部分。

### 8.1 消息写库主线

建议把以下文件串起来读：

- `AIHelper.cpp` 的 `pushMessageToMysql()`
- `AIApps/ChatServer/include/AIUtil/MQManager.h`
- `AIApps/ChatServer/src/AIUtil/MQManager.cpp`
- `main.cpp` 里的 `RabbitMQThreadPool`

主流程是：

```text
用户/AI 消息产生
-> AIHelper::addMessage
-> pushMessageToMysql
-> MQManager::publish("sql_queue", sql)
-> RabbitMQThreadPool 消费消息
-> executeMysql(sql)
-> MySQL 落库
```

### 8.2 为什么这是重点

因为它体现了真实服务端的工程取舍：

- 对话结果先写内存，保证主流程快
- SQL 异步入队，避免阻塞请求线程
- 用 MQ 做削峰和解耦

这比“接口里直接写库”更接近线上系统设计。

### 8.3 学习时要特别关注的问题

建议带着下面几个问题去读：

1. 为什么 `AIHelper` 不直接同步写 MySQL？
2. 为什么消费者线程池在 `main()` 启动？
3. 如果 RabbitMQ 挂了，会影响哪些链路？
4. 如果消息重复消费，如何做幂等？

这些问题的答案，能帮助你把“会写代码”升级成“会理解系统设计”。

---

## 9. 数据恢复与多会话机制怎么学

重点文件：

- `ChatServer.cpp` 中 `initChatMessage()`
- `ChatServer.cpp` 中 `readDataFromMySQL()`

启动时它会从 `chat_message` 表读取历史数据，并恢复到：

- `chatInformation`
- `sessionsIdsMap`

这个过程体现出两个核心设计：

1. 内存态是运行时高频访问结构
2. 数据库是持久化和重建来源

换句话说，这个项目并不是“所有查询都实时打数据库”，而是：

- 启动时恢复
- 运行时以内存为主
- 后台异步持久化

这是很多高并发服务的典型思路。

---

## 10. 多模态能力怎么学

### 10.1 语音能力

重点文件：

- `ChatSpeechHandler.cpp`
- `AISpeechProcessor.*`

建议重点看：

- 认证信息如何获取
- 文本转语音的调用链如何封装
- 响应最终如何返回音频 URL

### 10.2 图像识别能力

重点文件：

- `ImageRecognizer.*`
- `AIUploadSendHandler.*`

从 `CMakeLists.txt` 可以看出这里接了：

- `OpenCV`
- `onnxruntime`

说明项目并不只是调云端大模型，也预留了本地视觉推理能力。

你第一次学习时可以先知道它在架构中的位置：

- HTTP 接口接收上传
- 业务层调图像识别模块
- 返回结构化结果

等把聊天主链路读顺后，再深挖这块会更合适。

---

## 11. 推荐阅读顺序

下面给你一个更实战的阅读顺序，不建议跳着看。

### 第一阶段：建立全局地图

1. `README.md`
2. `CMakeLists.txt`
3. `AIApps/ChatServer/src/main.cpp`
4. `AIApps/ChatServer/src/ChatServer.cpp`

目标：

- 知道项目由哪两层组成
- 知道程序怎么启动
- 知道路由和共享状态初始化在哪

### 第二阶段：吃透 HTTP 框架主线

1. `HttpServer/include/http/HttpServer.h`
2. `HttpServer/src/http/HttpServer.cpp`
3. `HttpRequest / HttpResponse / HttpContext`
4. `Router.h / Router.cpp`
5. `SessionManager.*`
6. `MiddlewareChain.*`
7. `CorsMiddleware.*`

目标：

- 能说清一次 HTTP 请求如何走完整链路
- 能自己扩展一个简单路由
- 能理解 Session 与中间件机制

### 第三阶段：吃透聊天业务主线

1. `ChatLoginHandler.cpp`
2. `ChatRegisterHandler.cpp`
3. `ChatSendHandler.cpp`
4. `ChatHistoryHandler.cpp`
5. `ChatSessionsHandler.cpp`
6. `ChatCreateAndSendHandler.cpp`

目标：

- 能说清从用户提问到模型回复的业务路径
- 能理解多用户多会话的数据结构
- 能自己新增一个业务接口

### 第四阶段：吃透 AI 抽象设计

1. `AIHelper.*`
2. `AIStrategy.*`
3. `AIFactory.*`
4. `AIToolRegistry.*`
5. `AIConfig.*`
6. `resource/config.json`

目标：

- 能说清策略模式为什么适合多模型
- 能说清工厂注册机制如何支持扩展
- 能自己新增一个模型策略或工具调用

### 第五阶段：吃透工程化能力

1. `MysqlUtil.*`
2. `DbConnectionPool.*`
3. `MQManager.*`
4. `ChatServer::readDataFromMySQL()`
5. `ChatSpeechHandler.*`
6. `ImageRecognizer.*`

目标：

- 理解异步写库和启动恢复机制
- 理解消息队列在架构中的作用
- 理解项目如何从“聊天”走向“平台化”

---

## 12. 每个阶段要带着哪些问题去学

### 框架层问题

1. `HttpContext` 如何判断一个完整请求已经到达？
2. `HttpServer` 为什么把路由和中间件放在 `handleRequest()`？
3. `Router` 为什么同时支持 callback 和 handler 对象？
4. Session 为什么依赖 Cookie，而不是让前端手动传？

### 业务层问题

1. 为什么 `ChatServer` 不直接处理每个请求，而是拆成 handler？
2. 为什么聊天状态放在 `AIHelper` 而不是全局 vector？
3. 为什么一个用户要用多个 `sessionId` 区分上下文？

### AI 层问题

1. 如果接入新模型，需要改哪些地方？
2. 为什么 `buildRequest()` 和 `parseResponse()` 必须绑定到策略？
3. MCP 风格工具调用为什么要两次模型请求？

### 工程层问题

1. 为什么写库要异步？
2. 为什么启动时要做历史恢复？
3. 连接池如何避免频繁创建数据库连接？

---

## 13. 学完后建议你做的 6 个练习

不要只读，最好边学边改。

### 练习 1：新增一个健康检查接口

例如新增：

- `GET /health`

目标：

- 熟悉 `initializeRouter()`
- 熟悉 `RouterHandler` 写法
- 熟悉 JSON 响应封装

### 练习 2：新增一个 AI 工具

例如：

- `get_date`
- `get_weekday`

目标：

- 熟悉 `AIToolRegistry`
- 熟悉 `config.json` 工具配置
- 理解工具调用闭环

### 练习 3：新增一个模型策略

例如新增一个新的 `XXXStrategy`。

目标：

- 熟悉 `AIStrategy` 抽象
- 熟悉 `StrategyRegister` 注册机制
- 验证项目的可扩展性设计

### 练习 4：给聊天接口增加参数校验

目标：

- 熟悉 handler 中请求解析
- 训练你发现异常路径和边界条件

### 练习 5：增加统一鉴权中间件

目标：

- 理解哪些逻辑适合下沉为 middleware
- 避免在多个 handler 中重复写登录校验

### 练习 6：画出你自己的请求时序图

至少画两条：

1. `/login` 的请求链路
2. `/chat/send` 的请求链路

目标：

- 把零散代码真正串成系统理解

---

## 14. 如果你时间有限，最小学习闭环怎么走

如果你不是要把整个项目一次学完，而是想先快速建立理解，建议按下面最小闭环：

1. `README.md`
2. `main.cpp`
3. `ChatServer.cpp`
4. `HttpServer.cpp`
5. `Router.cpp`
6. `SessionManager.cpp`
7. `ChatLoginHandler.cpp`
8. `ChatSendHandler.cpp`
9. `AIHelper.cpp`
10. `AIStrategy.cpp`
11. `AIFactory.cpp`
12. `AIToolRegistry.cpp`
13. `MQManager.cpp`

这 13 个文件基本可以帮你打通：

- 服务启动
- HTTP 请求处理
- 登录态
- 聊天主流程
- 多模型切换
- 工具调用
- 异步写库

---

## 15. 最后给你的学习建议

学这个项目，最容易犯的错有两个：

1. 一上来就盯着某个 AI 接口细节
2. 只会顺着代码看，没把结构总结出来

更好的方式是：

1. 先看启动和总装配
2. 再看 HTTP 框架主链路
3. 再看聊天业务闭环
4. 最后研究 AI 策略、工具调用和异步化设计

你真正要学到的不是“这个接口怎么写”，而是这几个工程思想：

- 框架层和业务层分离
- 会话状态与请求处理解耦
- 多模型通过策略模式解耦
- 扩展能力通过工厂注册机制解耦
- 持久化通过 MQ 异步化解耦

如果你把这几条理解透，这个项目的价值就不只是“做了一个 C++ AI 服务”，而是你已经开始接近真实后端系统设计了。
