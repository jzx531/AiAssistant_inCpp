# AIUtil 目录详细解析

## 1. AIUtil 整体定位

`AIApps/ChatServer/AIUtil/` 是整个项目里 AI 相关能力的核心工具层。

它并不是单一的“大模型调用目录”，而是把一组与 AI 业务强相关、但又不适合直接塞进 handler 的能力单独抽出来，主要包括：

- 大模型调用封装
- 多模型策略切换
- 工具调用（MCP 风格）
- Prompt 配置管理
- 语音识别 / 语音合成
- 图像识别
- Session ID 生成
- RabbitMQ 异步消息支持
- Base64 编解码

从职责上看，`AIUtil` 处于：

- 上层：`ChatSendHandler`、`ChatSpeechHandler`、`AIUploadSendHandler` 等业务入口
- 下层：第三方 AI 平台 API、本地 ONNX 推理、RabbitMQ、Base64 等底层能力

所以它的本质是：

> 把 ChatServer 中所有 AI 能力相关的“可复用组件”和“外部平台适配逻辑”集中起来。

---

## 2. AIUtil 目录下有哪些类/模块

当前目录下主要有这些文件：

### 2.1 大模型调用主链路

- `AIHelper.h / AIHelper.cpp`
- `AIStrategy.h / AIStrategy.cpp`
- `AIFactory.h / AIFactory.cpp`
- `AIConfig.h / AIConfig.cpp`
- `AIToolRegistry.h / AIToolRegistry.cpp`

### 2.2 语音能力

- `AISpeechProcessor.h / AISpeechProcessor.cpp`

### 2.3 图像识别能力

- `ImageRecognizer.h / ImageRecognizer.cpp`

### 2.4 会话辅助

- `AISessionIdGenerator.h / AISessionIdGenerator.cpp`

### 2.5 异步消息能力

- `MQManager.h / MQManager.cpp`

### 2.6 编解码工具

- `base64.h / base64.cpp`

---

## 3. 先看 AIUtil 的整体调用关系

如果从聊天主链路看，核心调用关系是：

```text
ChatSendHandler / ChatCreateAndSendHandler
-> AIHelper
-> StrategyFactory
-> AIStrategy (具体模型策略)
-> executeCurl()
-> 第三方模型 API

如果是 MCP 模式：
-> AIConfig 读取 config.json
-> AIToolRegistry 执行工具
-> 再次调用模型

消息持久化：
-> AIHelper::addMessage
-> MQManager::publish
-> RabbitMQ
```

如果是语音链路：

```text
ChatSpeechHandler
-> AISpeechProcessor
-> 百度 OAuth Token
-> 百度 TTS / ASR 接口
```

如果是图像上传链路：

```text
AIUploadSendHandler
-> base64_decode
-> ImageRecognizer
-> ONNX Runtime + OpenCV
-> 返回分类结果
```

所以 `AIUtil` 实际上不是单模块，而是三条支线：

1. 聊天模型支线
2. 语音支线
3. 图像支线

---

## 4. AIHelper：AI 会话级核心调度器

文件：

- `AIHelper.h`
- `AIHelper.cpp`

### 4.1 它的定位

`AIHelper` 是 AI 聊天链路中最核心的类。

可以把它理解为：

> 一个“单会话级”的 AI 运行时对象

因为它内部维护了当前会话的历史消息 `messages`，并负责：

- 保存上下文
- 切换模型策略
- 构造模型请求
- 发起 CURL 请求
- 解析模型结果
- 工具调用二段式推理
- 把消息异步入库

### 4.2 它的主要成员

核心成员有两个：

```cpp
std::shared_ptr<AIStrategy> strategy;
std::vector<std::pair<std::string, long long>> messages;
```

含义：

- `strategy`：当前模型策略
- `messages`：当前会话上下文，按“用户/AI 交替消息”存储

### 4.3 主要功能函数

#### `AIHelper()`

构造时默认创建策略 `"1"`，也就是默认阿里云普通聊天模型。

#### `setStrategy()`

切换当前策略对象。

#### `addMessage()`

作用：

- 给当前会话追加一条消息
- 生成时间戳
- 调用 `pushMessageToMysql()` 异步入库

#### `restoreMessage()`

作用：

- 从数据库恢复历史消息到内存会话上下文

这个函数在 `ChatServer::readDataFromMySQL()` 启动恢复流程中会被调用。

#### `chat()`

这是最核心函数，负责完成一次聊天调用。

它分两种模式：

1. 普通模型模式
2. MCP 工具调用模式

##### 普通模式流程

```text
切换 strategy
-> addMessage(用户问题)
-> strategy->buildRequest(messages)
-> executeCurl(payload)
-> strategy->parseResponse(response)
-> addMessage(AI 回复)
-> 返回答案
```

##### MCP 模式流程

```text
切换到 MCP strategy
-> AIConfig 读取 config.json
-> buildPrompt(userQuestion)
-> 第一次模型调用：判断是否需要工具
-> parseAIResponse()
-> 如果不需要工具：直接回答
-> 如果需要工具：AIToolRegistry::invoke()
-> buildToolResultPrompt()
-> 第二次模型调用：结合工具结果生成最终答案
-> 保存用户消息和 AI 回复
```

#### `request()`

作用：

- 允许外部直接传自定义 payload，底层仍复用 `executeCurl()`

#### `executeCurl()`

作用：

- 用 libcurl 发 HTTP 请求到当前策略指定的模型接口
- 自动带上 `Authorization: Bearer <apiKey>`
- 自动解析返回 JSON

这是模型请求真正落地的地方。

#### `pushMessageToMysql()`

作用：

- 把聊天消息构造成 SQL
- 不直接执行写库
- 而是丢给 `MQManager::instance().publish("sql_queue", sql)`

所以 `AIHelper` 不只是 AI 调用器，它还承担了：

- 会话上下文管理
- 对话持久化入口

### 4.4 需要注意的配置项

`AIHelper` 本身不直接读环境变量，但它强依赖：

- `AIStrategy` 提供的 API Key / URL / model
- MCP 模式下 `AIConfig` 读取的 `config.json`
- RabbitMQ 队列 `sql_queue`

所以它是 AI 调用总调度器，但真正配置散落在它依赖的类里。

---

## 5. AIStrategy：多模型策略抽象

文件：

- `AIStrategy.h`
- `AIStrategy.cpp`

### 5.1 它的定位

`AIStrategy` 是多模型适配层的核心抽象。

它把不同模型厂商/不同接口风格统一成一组标准能力：

```cpp
virtual std::string getApiUrl() const = 0;
virtual std::string getApiKey() const = 0;
virtual std::string getModel() const = 0;
virtual json buildRequest(...) const = 0;
virtual std::string parseResponse(...) const = 0;
```

也就是说，一个新模型想接入项目，至少要回答 5 个问题：

1. 请求打到哪个 URL
2. API Key 从哪里来
3. 模型名是什么
4. 请求 JSON 怎么拼
5. 返回 JSON 怎么解析

### 5.2 公共字段

```cpp
bool isMCPModel = false;
```

作用：

- 标记当前策略是否走工具调用链路

这个字段被 `AIHelper::chat()` 用来分流：

- 普通调用
- MCP 两阶段调用

---

## 6. 具体策略类详解

### 6.1 `AliyunStrategy`

作用：

- 对接阿里 DashScope 兼容模式聊天接口

#### 配置来源

- 环境变量：`DASHSCOPE_API_KEY`

#### 接口 URL

```text
https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
```

#### 模型名

```text
qwen-plus
```

#### 请求体结构

```json
{
  "model": "qwen-plus",
  "messages": [
    {"role": "user", "content": "..."},
    {"role": "assistant", "content": "..."}
  ]
}
```

#### 响应解析路径

```text
choices[0].message.content
```

#### 是否 MCP 模型

- 否，`isMCPModel = false`

---

### 6.2 `DouBaoStrategy`

作用：

- 对接火山引擎豆包聊天接口

#### 配置来源

- 环境变量：`DOUBAO_API_KEY`

#### 接口 URL

```text
https://ark.cn-beijing.volces.com/api/v3/chat/completions
```

#### 模型名

```text
doubao-seed-1-6-thinking-250715
```

#### 请求体结构

与 `AliyunStrategy` 类似，也是：

```json
{
  "model": "doubao-seed-1-6-thinking-250715",
  "messages": [...]
}
```

#### 响应解析路径

```text
choices[0].message.content
```

#### 是否 MCP 模型

- 否

---

### 6.3 `AliyunRAGStrategy`

作用：

- 对接阿里云知识库 / 应用式 completion 接口
- 用于 RAG 风格问答

#### 配置来源

- 环境变量：`DASHSCOPE_API_KEY`
- 环境变量：`Knowledge_Base_ID`

#### 接口 URL 生成方式

```text
https://dashscope.aliyuncs.com/api/v1/apps/<Knowledge_Base_ID>/completion
```

#### 模型名

- 当前 `getModel()` 返回空字符串
- 说明这个接口更像“App / 知识库应用接口”，不是普通 chat completion 的 model 参数模式

#### 请求体结构

```json
{
  "input": {
    "messages": [...]
  },
  "parameters": {}
}
```

#### 响应解析路径

```text
output.text
```

#### 是否 MCP 模型

- 否

---

### 6.4 `AliyunMcpStrategy`

作用：

- 对接阿里云聊天接口
- 但在业务上标记为 MCP 工具调用模式

#### 配置来源

- 环境变量：`DASHSCOPE_API_KEY`

#### 接口 URL

```text
https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
```

#### 模型名

```text
qwen-plus
```

#### 请求体结构

与普通阿里云聊天相同：

```json
{
  "model": "qwen-plus",
  "messages": [...]
}
```

#### 响应解析路径

```text
choices[0].message.content
```

#### 是否 MCP 模型

- 是，`isMCPModel = true`

真正的 MCP 行为不是接口自己支持，而是：

- `AIHelper` 看到 `isMCPModel = true`
- 转而启用 `AIConfig + AIToolRegistry` 的两段式工具调用流程

---

## 7. AIFactory：策略注册与创建工厂

文件：

- `AIFactory.h`
- `AIFactory.cpp`

### 7.1 它的定位

`AIFactory` 负责把“模型编号/名称”映射成具体策略对象。

核心类：

- `StrategyFactory`
- `StrategyRegister<T>`

### 7.2 它的作用

项目没有在业务里写一堆：

```cpp
if (modelType == "1") ...
else if (modelType == "2") ...
```

而是通过工厂注册机制，把创建逻辑集中管理。

### 7.3 当前注册关系

在 `AIStrategy.cpp` 底部注册了：

```cpp
"1" -> AliyunStrategy
"2" -> DouBaoStrategy
"3" -> AliyunRAGStrategy
"4" -> AliyunMcpStrategy
```

### 7.4 工程价值

这让 `AIHelper` 可以只写：

```cpp
setStrategy(StrategyFactory::instance().create(modelType));
```

而不用知道具体是哪一家模型。

所以 `AIFactory` 的功能不是“配置类”，而是：

- 模型策略实例化入口
- 扩展新模型时的统一注册点

---

## 8. AIConfig：MCP Prompt 与工具配置管理器

文件：

- `AIConfig.h`
- `AIConfig.cpp`

### 8.1 它的定位

`AIConfig` 主要服务于 MCP 工具调用模式。

它负责：

- 从配置文件加载 Prompt 模板
- 加载可用工具清单
- 构造“工具判断阶段”的提示词
- 解析模型是否要调用工具
- 构造“工具执行结果回灌阶段”的提示词

### 8.2 核心数据结构

#### `AITool`

表示一个工具定义：

- `name`
- `params`
- `desc`

#### `AIToolCall`

表示一次模型输出的工具调用意图：

- `toolName`
- `args`
- `isToolCall`

### 8.3 主要功能函数

#### `loadFromFile(path)`

作用：

- 读取 `config.json`
- 解析 `prompt_template`
- 解析 `tools[]`

#### `buildToolList()`

作用：

- 把工具列表拼成可插入 Prompt 的文本描述

#### `buildPrompt(userInput)`

作用：

- 用 `prompt_template` 替换 `{user_input}` 和 `{tool_list}`
- 生成第一次工具判断请求的 Prompt

#### `parseAIResponse(response)`

作用：

- 尝试把模型返回解析为 JSON
- 如果存在 `tool` 和 `args`，则判定为工具调用
- 否则视为普通文本回答

#### `buildToolResultPrompt(...)`

作用：

- 把工具调用结果再次包装成 Prompt
- 交给模型做第二次总结回答

### 8.4 当前配置文件来源

在 `AIHelper::chat()` 中写死加载：

```cpp
../AIApps/ChatServer/resource/config.json
```

当前 `config.json` 内容核心包括：

- `prompt_template`
- `tools`

### 8.5 它需要哪些 AI 配置

`AIConfig` 自己不需要 API Key，但需要：

- 配置文件路径
- Prompt 模板内容
- 工具列表定义

这类配置属于“AI 行为配置”，不是“AI 接口认证配置”。

---

## 9. AIToolRegistry：本地工具注册表

文件：

- `AIToolRegistry.h`
- `AIToolRegistry.cpp`

### 9.1 它的定位

`AIToolRegistry` 是 MCP 工具调用阶段的本地执行器。

它负责：

- 注册工具名到函数的映射
- 接收模型输出的工具调用指令
- 在本地执行对应工具函数
- 返回 JSON 结果给 AI 再总结

### 9.2 核心结构

```cpp
using ToolFunc = std::function<json(const json&)>;
std::unordered_map<std::string, ToolFunc> tools_;
```

本质上就是：

```text
工具名 -> 可执行函数
```

### 9.3 主要功能函数

#### `AIToolRegistry()`

构造时自动注册内置工具：

- `get_weather`
- `get_time`

#### `registerTool(name, func)`

作用：

- 注册一个新工具

#### `invoke(name, args)`

作用：

- 根据名称查找工具
- 找到后执行对应函数
- 找不到则抛异常

#### `hasTool(name)`

作用：

- 判断某个工具是否已注册

### 9.4 内置工具说明

#### `getWeather(args)`

作用：

- 从 `args.city` 中读取城市名
- 调 `wttr.in` 天气服务
- 返回天气文本

##### 依赖接口 URL

```text
https://wttr.in/<city>?format=3&lang=zh
```

##### 输入参数

- `city`

##### 返回格式

```json
{
  "city": "北京",
  "weather": "北京: ..."
}
```

#### `getTime(args)`

作用：

- 返回当前系统时间

##### 输入参数

- 无

##### 返回格式

```json
{
  "time": "2026-05-23 12:34:56"
}
```

### 9.5 配置关系

要注意：

- `AIToolRegistry` 里真正可执行的工具，是代码注册的
- `AIConfig` 里让模型“看到”的工具，是配置文件里声明的

所以新增工具时，通常两边都要改：

1. `AIToolRegistry` 注册实现
2. `config.json` 声明工具描述

---

## 10. AISpeechProcessor：语音识别与语音合成封装

文件：

- `AISpeechProcessor.h`
- `AISpeechProcessor.cpp`

### 10.1 它的定位

`AISpeechProcessor` 是百度语音接口的封装器。

它负责：

- 通过 `client_id + client_secret` 获取 OAuth token
- 进行语音识别（ASR）
- 进行语音合成（TTS）

### 10.2 构造参数

```cpp
AISpeechProcessor(const std::string& clientId,
                  const std::string& clientSecret,
                  const std::string& cuid = "...")
```

#### 参数含义

- `clientId`：百度应用的 client id
- `clientSecret`：百度应用的 client secret
- `cuid`：设备或用户唯一标识

构造时会自动调用：

```cpp
token_ = getAccessToken();
```

### 10.3 主要功能函数

#### `getAccessToken()`

作用：

- 调百度 OAuth 接口获取 access token

##### 接口 URL

```text
https://aip.baidubce.com/oauth/2.0/token
```

##### 请求方式

- `POST`

##### 请求体

```text
grant_type=client_credentials&client_id=<id>&client_secret=<secret>
```

##### 返回关键字段

- `access_token`

---

#### `recognize(speechData, format, rate, channel)`

作用：

- 调百度 ASR 语音识别接口
- 输入 Base64 编码后的语音数据
- 返回识别文本

##### 接口 URL

```text
https://vop.baidu.com/server_api
```

##### 请求方式

- `POST`

##### 请求体关键字段

- `format`
- `rate`
- `channel`
- `cuid`
- `token`
- `len`
- `speech`

##### 返回关键字段

```text
result[0]
```

---

#### `synthesize(text, format, lang, speed, pitch, volume)`

作用：

- 调百度 TTS 异步任务接口
- 先创建任务
- 再轮询查询任务状态
- 最终返回音频 URL

##### 第一步：创建任务接口

URL：

```text
https://aip.baidubce.com/rpc/2.0/tts/v1/create?access_token=<token>
```

请求体关键字段：

- `text`
- `format`
- `lang`
- `speed`
- `pitch`
- `volume`
- `enable_subtitle`

返回关键字段：

- `task_id`

##### 第二步：查询任务接口

URL：

```text
https://aip.baidubce.com/rpc/2.0/tts/v1/query?access_token=<token>
```

请求体关键字段：

- `task_ids`

返回关键字段：

```text
tasks_info[0].task_status
tasks_info[0].task_result.speech_url
```

### 10.4 它需要哪些配置

`AISpeechProcessor` 依赖这些配置：

- 环境变量：`BAIDU_CLIENT_ID`
- 环境变量：`BAIDU_CLIENT_SECRET`
- 可选构造参数：`cuid`

其中 `ChatSpeechHandler` 中实际读取了：

- `BAIDU_CLIENT_ID`
- `BAIDU_CLIENT_SECRET`

---

## 11. ImageRecognizer：本地图像识别封装

文件：

- `ImageRecognizer.h`
- `ImageRecognizer.cpp`

### 11.1 它的定位

`ImageRecognizer` 负责本地图片分类推理，不依赖云端大模型 API。

它基于：

- `OpenCV`
- `ONNX Runtime`

来完成：

- 读取图片
- 预处理
- 调 ONNX 模型推理
- 输出分类标签

### 11.2 构造参数

```cpp
ImageRecognizer(const std::string& model_path,
                const std::string& label_path = "/root/imagenet_classes.txt")
```

#### 参数含义

- `model_path`：ONNX 模型路径
- `label_path`：分类标签文件路径

### 11.3 主要功能函数

#### `LoadLabels(label_path)`

作用：

- 从标签文件中读取类别名称

#### `PredictFromFile(image_path)`

作用：

- 从图片文件路径读取图片并分类

#### `PredictFromBuffer(image_data)`

作用：

- 从内存二进制图片数据解码并分类

#### `PredictFromMat(img)`

作用：

- 对 OpenCV 的 `cv::Mat` 直接做推理
- 包括 resize、归一化、NCHW 转换、ONNX 推理、argmax 分类

### 11.4 它需要哪些配置

这不是云端 AI API 配置，而是本地模型资源配置：

- ONNX 模型文件路径
- 标签文件路径

当前在 `AIUploadSendHandler.cpp` 中硬编码使用：

```cpp
/root/models/mobilenetv2/mobilenetv2-7.onnx
```

默认标签路径在构造函数默认参数中是：

```cpp
/root/imagenet_classes.txt
```

这意味着如果部署环境不同，这两个路径都需要调整。

---

## 12. AISessionIdGenerator：AI 会话 ID 生成器

文件：

- `AISessionIdGenerator.h`
- `AISessionIdGenerator.cpp`

### 12.1 它的定位

`AISessionIdGenerator` 是一个很小的辅助类，用于生成新的聊天会话 ID。

### 12.2 工作原理

它在构造时调用：

```cpp
std::srand(static_cast<unsigned>(std::time(nullptr)));
```

生成时：

```cpp
now = 当前时间戳
randVal = 随机数
rawId = now ^ randVal
return std::to_string(rawId)
```

### 12.3 使用位置

主要在：

- `ChatCreateAndSendHandler.cpp`

用于创建新聊天会话。

### 12.4 是否需要 AI 配置

- 不需要

它是纯本地工具类。

---

## 13. MQManager：异步消息发布与消费线程池

文件：

- `MQManager.h`
- `MQManager.cpp`

### 13.1 它的定位

虽然它名字在 `AIUtil` 下，但本质上它不是 AI 推理类，而是 AI 聊天消息异步持久化的配套工具。

它负责：

- 向 RabbitMQ 发布消息
- 启动消费者线程池消费消息

### 13.2 `MQManager` 的作用

核心职责：

- 维护 RabbitMQ Channel 连接池
- 提供 `publish(queue, msg)`

当前 `AIHelper::pushMessageToMysql()` 通过它把 SQL 发进：

```text
sql_queue
```

### 13.3 `RabbitMQThreadPool` 的作用

核心职责：

- 启动多个消费线程
- 每个线程独立消费队列消息
- 收到消息后调用外部 handler

在 `main.cpp` 中，实际传入的 handler 是：

- `executeMysql(sql)`

也就是说 MQ 的消息内容本质就是 SQL 语句。

### 13.4 它需要哪些配置

虽然不是 AI 平台 API 配置，但它依赖运行环境配置：

- RabbitMQ host：当前默认 `localhost`
- Queue 名：当前默认 `sql_queue`
- 端口：`5672`
- 用户名：`guest`
- 密码：`guest`

这些目前大多硬编码在 `MQManager.cpp` 和 `main.cpp` 中。

---

## 14. base64：Base64 编解码工具

文件：

- `base64.h`
- `base64.cpp`

### 14.1 它的定位

`base64` 不是项目自定义 AI 算法，而是一个通用工具模块，用于：

- 二进制与 Base64 之间转换

### 14.2 使用场景

当前源码里明确看到：

- `AIUploadSendHandler.cpp` 中使用 `base64_decode(imageBase64)`

这说明前端上传图片时，图片数据是 Base64 文本，后端先解码，再交给 `ImageRecognizer` 识别。

另外 `AISpeechProcessor.h` 也包含了 `base64.h`，意味着后续语音数据场景也可能用它做封装。

### 14.3 是否需要 AI 配置

- 不需要

它是纯本地工具库。

---

## 15. 需要配置 AI 项的类与接口清单

下面把“哪些类需要写 AI 配置”集中整理出来。

---

## 16. 配置总表

### 16.1 `AliyunStrategy`

#### 用途

- 阿里云普通聊天

#### 必填环境变量

- `DASHSCOPE_API_KEY`

#### 固定接口 URL

```text
https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
```

#### 固定模型名

```text
qwen-plus
```

#### 请求返回关键字段

- 请求：`model`, `messages`
- 返回：`choices[0].message.content`

---

### 16.2 `DouBaoStrategy`

#### 用途

- 豆包聊天模型

#### 必填环境变量

- `DOUBAO_API_KEY`

#### 固定接口 URL

```text
https://ark.cn-beijing.volces.com/api/v3/chat/completions
```

#### 固定模型名

```text
doubao-seed-1-6-thinking-250715
```

#### 请求返回关键字段

- 请求：`model`, `messages`
- 返回：`choices[0].message.content`

---

### 16.3 `AliyunRAGStrategy`

#### 用途

- 阿里云知识库 / RAG 问答

#### 必填环境变量

- `DASHSCOPE_API_KEY`
- `Knowledge_Base_ID`

#### 动态接口 URL

```text
https://dashscope.aliyuncs.com/api/v1/apps/<Knowledge_Base_ID>/completion
```

#### 模型名

- 当前未显式使用

#### 请求返回关键字段

- 请求：`input.messages`, `parameters`
- 返回：`output.text`

---

### 16.4 `AliyunMcpStrategy`

#### 用途

- 阿里云聊天 + 本地工具调用工作流

#### 必填环境变量

- `DASHSCOPE_API_KEY`

#### 固定接口 URL

```text
https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
```

#### 固定模型名

```text
qwen-plus
```

#### 额外必需配置文件

- `../AIApps/ChatServer/resource/config.json`

#### 请求返回关键字段

- 请求：`model`, `messages`
- 返回：`choices[0].message.content`

#### 额外说明

MCP 功能不仅依赖这个策略，还依赖：

- `AIConfig`
- `AIToolRegistry`
- `config.json`

---

### 16.5 `AIConfig`

#### 用途

- MCP Prompt 与工具描述配置

#### 必填配置文件

- `resource/config.json`

#### 配置项

- `prompt_template`
- `tools[]`

#### 当前样例字段

```json
{
  "prompt_template": "...",
  "tools": [
    {
      "name": "get_weather",
      "params": {"city": "北京"},
      "desc": "获取天气"
    }
  ]
}
```

---

### 16.6 `AIToolRegistry`

#### 用途

- 执行本地工具

#### 当前内置外部接口

天气接口：

```text
https://wttr.in/<city>?format=3&lang=zh
```

#### 当前内置工具

- `get_weather`
- `get_time`

#### 配置说明

严格说它不依赖 API Key，但新增工具时通常需要同步修改：

1. `AIToolRegistry.cpp`
2. `config.json`

---

### 16.7 `AISpeechProcessor`

#### 用途

- 百度 ASR / TTS

#### 必填环境变量

- `BAIDU_CLIENT_ID`
- `BAIDU_CLIENT_SECRET`

#### 可选配置

- `cuid`

#### 涉及接口 URL

OAuth：

```text
https://aip.baidubce.com/oauth/2.0/token
```

ASR：

```text
https://vop.baidu.com/server_api
```

TTS 创建任务：

```text
https://aip.baidubce.com/rpc/2.0/tts/v1/create?access_token=<token>
```

TTS 查询任务：

```text
https://aip.baidubce.com/rpc/2.0/tts/v1/query?access_token=<token>
```

---

### 16.8 `ImageRecognizer`

#### 用途

- 本地图像分类推理

#### 必填本地资源路径

- ONNX 模型路径
- label 文件路径

#### 当前硬编码路径

模型：

```text
/root/models/mobilenetv2/mobilenetv2-7.onnx
```

默认标签文件：

```text
/root/imagenet_classes.txt
```

#### 说明

这不是云端 API 配置，但属于 AI 运行资源配置。

---

## 17. 哪些类不需要 AI 配置

以下类主要是本地工具/辅助组件，不需要写 AI 平台配置：

- `AIFactory`
- `AISessionIdGenerator`
- `base64`

另外：

- `MQManager` 不需要 AI 平台 Key，但需要 RabbitMQ 运行参数配置

---

## 18. 推荐你如何理解 AIUtil 的分层

为了避免后面读代码越看越乱，建议把 `AIUtil` 记成下面 4 层。

### 18.1 调度层

- `AIHelper`

负责把一次 AI 会话真正跑起来。

### 18.2 模型适配层

- `AIStrategy`
- `AIFactory`

负责不同模型平台的统一接入。

### 18.3 AI 行为增强层

- `AIConfig`
- `AIToolRegistry`

负责工具调用、Prompt 模板、两阶段推理。

### 18.4 能力工具层

- `AISpeechProcessor`
- `ImageRecognizer`
- `AISessionIdGenerator`
- `MQManager`
- `base64`

负责具体功能支撑。

---

## 19. 最后一句总结

`AIUtil` 不是“杂项工具目录”，而是整个 ChatServer 的 AI 能力中台。

如果只看一句话，可以这样记：

- `AIHelper` 负责调度一次 AI 会话
- `AIStrategy` 负责接不同模型
- `AIFactory` 负责创建模型策略
- `AIConfig` 负责 MCP Prompt 配置
- `AIToolRegistry` 负责执行工具
- `AISpeechProcessor` 负责语音
- `ImageRecognizer` 负责图像识别
- `MQManager` 负责异步消息
- `AISessionIdGenerator` 负责会话 ID
- `base64` 负责编解码

如果你后面要扩这个项目，最常改的 AI 配置入口通常会是：

1. `AIStrategy.cpp` 里的 URL / model
2. 环境变量里的 API Key
3. `resource/config.json` 的 Prompt 和 tools
4. `AISpeechProcessor` 的百度语音账号参数
5. `ImageRecognizer` 的模型和标签路径
