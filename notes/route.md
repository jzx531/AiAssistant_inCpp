# HttpServer 中 Router 的详细解析

## 1. Router 是干什么的

`Router` 是 `HttpServer` 里的请求分发中心。

它负责把一个已经解析完成的 HTTP 请求，根据：

- 请求方法 `Method`
- 请求路径 `path`

映射到对应的业务处理逻辑。

简单说，`Router` 解决的是这件事：

> 请求已经到了，接下来该交给谁处理？

如果没有 `Router`，那 `HttpServer` 就只能在核心网络代码里写大量 `if/else` 判断路径和方法。这样框架层和业务层就会强耦合，后续每加一个接口都要改核心服务器代码。

所以 `Router` 的根本作用不是“查个表”，而是：

- 解耦框架与业务
- 管理路由注册
- 统一请求分发入口

---

## 2. Router 在整个请求链路中的位置

在这个项目里，一次请求的主链路大致是：

```text
TcpConnection 建立
-> HttpServer::onMessage
-> HttpContext 解析 HTTP 报文
-> HttpServer::onRequest
-> HttpServer::handleRequest
-> middlewareChain_.processBefore(...)
-> router_.route(...)
-> 对应 handler / callback 执行业务
-> middlewareChain_.processAfter(...)
-> HttpResponse 回包
```

从位置上看，`Router` 处于：

- HTTP 报文已经解析完成之后
- 真正业务逻辑执行之前

这意味着它不关心：

- TCP 收发
- HTTP 解析细节
- Session 存储实现
- 数据库逻辑

它只关心一件事：

- 这个请求应该分发给哪一个处理器

---

## 3. Router 的核心职责

从源码看，`Router` 有 4 个核心职责：

1. 注册路由
2. 保存路由表
3. 匹配请求
4. 调用对应处理器

对应源码文件：

- `HttpServer/include/router/Router.h`
- `HttpServer/src/router/Router.cpp`

---

## 4. Router 支持的 4 种路由形式

这个 `Router` 支持两种维度的组合。

第一维：路径匹配方式

- 精准匹配
- 动态正则匹配

第二维：处理方式

- 回调函数 `HandlerCallback`
- 对象式处理器 `HandlerPtr`

所以总共有 4 种路由形式：

1. 精准路径 + Handler 对象
2. 精准路径 + callback
3. 动态路径 + Handler 对象
4. 动态路径 + callback

这是它比“最简单路由表”更完整的地方。

---

## 5. 为什么要区分 Handler 和 callback

在 `Router.h` 中：

```cpp
using HandlerPtr = std::shared_ptr<RouterHandler>;
using HandlerCallback = std::function<void(const HttpRequest &, HttpResponse *)>;
```

这两个抽象分别适用于不同复杂度的业务。

### 5.1 callback 的作用

适合：

- 逻辑简单
- 一次性小接口
- 不需要成员变量

例如：

```cpp
server.Get("/ping", [](const HttpRequest& req, HttpResponse* resp) {
    // 简单处理
});
```

### 5.2 Handler 对象的作用

适合：

- 复杂业务
- 需要依赖外部对象
- 需要封装多个辅助函数
- 需要成员变量

本项目里的真实业务几乎都用这种方式，例如：

- `ChatLoginHandler`
- `ChatSendHandler`
- `ChatHistoryHandler`

这是因为聊天、登录、语音等逻辑都不是一两行 lambda 能优雅承载的。

所以 `Router` 同时支持两种方式，本质上是在平衡：

- 轻量开发体验
- 工程可维护性

---

## 6. 精准路由的数据结构

精准路由的关键不是只看路径，而是看：

- 请求方法
- 请求路径

源码里的路由键定义如下：

```cpp
struct RouteKey
{
    HttpRequest::Method method;
    std::string path;

    bool operator==(const RouteKey &other) const
    {
        return method == other.method && path == other.path;
    }
};
```

这说明：

- `GET /chat`
- `POST /chat`

在框架看来是两个不同路由。

这是符合 HTTP 语义的，因为方法本来就是路由选择的一部分。

### 6.1 为什么需要哈希函数

因为 `RouteKey` 是自定义结构体，想放进 `unordered_map`，就必须定义哈希。

源码中：

```cpp
struct RouteKeyHash
{
    size_t operator()(const RouteKey &key) const
    {
        size_t methodHash = std::hash<int>{}(static_cast<int>(key.method));
        size_t pathHash = std::hash<std::string>{}(key.path);
        return methodHash * 31 + pathHash;
    }
};
```

这样 `Router` 就能用哈希表做精准查找。

### 6.2 精准路由存在哪里

```cpp
std::unordered_map<RouteKey, HandlerPtr, RouteKeyHash> handlers_;
std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> callbacks_;
```

分别表示：

- `handlers_`：精准路径对应对象式处理器
- `callbacks_`：精准路径对应函数回调

这类查找是当前 `Router` 里效率最高的一类。

---

## 7. 动态路由的数据结构

对于像 `/user/:id` 这种路径，无法直接做哈希键精确查找，所以项目采用的是“预编译正则 + 线性遍历”的方式。

对应容器：

```cpp
std::vector<RouteHandlerObj> regexHandlers_;
std::vector<RouteCallbackObj> regexCallbacks_;
```

这说明动态路由不是通过哈希命中，而是：

- 逐个拿正则去匹配当前请求路径

这种实现简单直观，适合当前项目规模。

---

## 8. 动态路径是怎么转换成正则的

`Router.h` 里有一个关键函数：

```cpp
std::regex convertToRegex(const std::string &pathPattern)
{
    std::string regexPattern = "^" + std::regex_replace(
        pathPattern,
        std::regex(R"(/:([^/]+))"),
        R"(/([^/]+))") + "$";
    return std::regex(regexPattern);
}
```

它的作用是把类似：

```text
/user/:id
```

转换成：

```text
^/user/([^/]+)$
```

这样就能匹配：

- `/user/1`
- `/user/abc`
- `/user/2026`

但不能匹配：

- `/user/1/profile`

因为正则前后加了 `^` 和 `$`，要求整条路径完整匹配。

这一步的意义是：

- 把“人类可读的路由模式”转换成“机器可执行的匹配规则”

---

## 9. 路径参数提取的作用

动态路由命中以后，只知道“匹配成功”还不够，业务层通常还需要拿到路径里的变量。

项目中的实现：

```cpp
void extractPathParameters(const std::smatch &match, HttpRequest &request)
{
    for (size_t i = 1; i < match.size(); ++i)
    {
        request.setPathParameters("param" + std::to_string(i), match[i].str());
    }
}
```

例如：

- 路由模式：`/user/:id`
- 实际请求：`/user/123`

那么提取结果大致会是：

- `param1 = "123"`

注意当前实现有一个特点：

- 它并没有保留参数原始名字 `id`
- 而是统一命名为 `param1`、`param2`

所以这是一个比较基础的参数提取实现。

它的好处是已经把“路径字符串切分”这件事从业务层抽走了；不足是语义不够强。

---

## 10. 路由注册流程解析

### 10.1 精准 Handler 注册

源码：

```cpp
void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    RouteKey key{method, path};
    handlers_[key] = std::move(handler);
}
```

做了两件事：

1. 组装路由键 `RouteKey`
2. 放入 `handlers_`

### 10.2 精准 callback 注册

源码：

```cpp
void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
{
    RouteKey key{method, path};
    callbacks_[key] = std::move(callback);
}
```

和上面一样，只是目标容器不同。

### 10.3 动态 Handler 注册

源码：

```cpp
void addRegexHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    std::regex pathRegex = convertToRegex(path);
    regexHandlers_.emplace_back(method, pathRegex, handler);
}
```

多做了一步：

- 先把路径模式转换成正则
- 再存入 `regexHandlers_`

### 10.4 动态 callback 注册

原理和上面一致，只是存入 `regexCallbacks_`。

---

## 11. `Router::route()` 是整个 Router 的核心

真正的分发逻辑都在：

```cpp
bool Router::route(const HttpRequest &req, HttpResponse *resp)
```

这个函数的职责是：

- 根据请求找到对应路由
- 执行路由对应的 handler 或 callback
- 告诉上层是否匹配成功

返回值语义：

- `true`：匹配并执行了某个路由
- `false`：没有任何路由命中

这个设计很重要，因为 `Router` 只负责“路由有没有命中”，不负责“没命中时怎么构造 404”。

404 的处理交给 `HttpServer::handleRequest()`。

---

## 12. `Router::route()` 逐步执行过程

下面按源码顺序详细解释。

### 12.1 第一步：构造查找键

```cpp
RouteKey key{req.method(), req.path()};
```

这一行把当前请求中的：

- 方法
- 路径

打包成一个 `RouteKey`，准备先查精准路由表。

这是最优先的一步，因为精准路由命中效率最高。

---

### 12.2 第二步：查精准 Handler

```cpp
auto handlerIt = handlers_.find(key);
if (handlerIt != handlers_.end())
{
    handlerIt->second->handle(req, resp);
    return true;
}
```

这一步的含义：

- 去 `handlers_` 中查有没有完全匹配的对象式处理器
- 如果找到了，直接执行 `handle(req, resp)`
- 处理完立即返回 `true`

为什么要先查这个？

- 精准匹配最快
- 复杂业务通常都注册成 Handler 对象

从当前项目实际情况看，`ChatServer` 里的主业务几乎都是这种形式。

---

### 12.3 第三步：查精准 callback

```cpp
auto callbackIt = callbacks_.find(key);
if (callbackIt != callbacks_.end())
{
    callbackIt->second(req, resp);
    return true;
}
```

如果精准 Handler 没找到，再去查精准 callback。

这说明 `Router` 的优先级顺序是：

1. 精准 Handler
2. 精准 callback
3. 动态 Handler
4. 动态 callback

也就是说，如果同一个 `method + path` 同时注册了 Handler 和 callback，最终会优先命中 Handler。

---

### 12.4 第四步：查动态 Handler

```cpp
for (const auto &[method, pathRegex, handler] : regexHandlers_)
{
    std::smatch match;
    std::string pathStr(req.path());
    if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
    {
        HttpRequest newReq(req);
        extractPathParameters(match, newReq);

        handler->handle(newReq, resp);
        return true;
    }
}
```

这一步是动态路由的关键。

它做了这些事：

1. 遍历所有动态 Handler 路由
2. 先判断请求方法是否一致
3. 再用正则匹配请求路径
4. 匹配成功后复制一份请求 `newReq`
5. 把路径参数写进 `newReq`
6. 调用 `handler->handle(newReq, resp)`
7. 返回 `true`

### 为什么这里要复制 `HttpRequest`

因为原始参数是：

```cpp
const HttpRequest &req
```

不能直接改。

但动态路由需要把路径参数塞进请求对象，所以这里复制一份 `newReq`，在副本上写参数，再交给业务层。

这是一个很合理的设计：

- 保持原始请求只读
- 允许动态路由补充额外上下文

---

### 12.5 第五步：查动态 callback

```cpp
for (const auto &[method, pathRegex, callback] : regexCallbacks_)
{
    std::smatch match;
    std::string pathStr(req.path());
    if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
    {
        HttpRequest newReq(req);
        extractPathParameters(match, newReq);

        callback(req, resp);
        return true;
    }
}
```

从意图看，这里应该和动态 Handler 保持一致：

- 复制请求
- 填路径参数
- 再把增强后的请求传给 callback

但当前实现最后调用的是：

```cpp
callback(req, resp);
```

而不是：

```cpp
callback(newReq, resp);
```

这意味着：

- 虽然参数被提取到了 `newReq`
- 但 callback 实际拿到的是原始 `req`
- 所以动态 callback 根本拿不到提取后的路径参数

这是当前 `Router` 的一个明确实现问题。

---

### 12.6 第六步：返回未命中

```cpp
return false;
```

如果四轮匹配都没命中，`Router` 返回 `false`。

然后由上层 `HttpServer::handleRequest()` 负责生成 404：

```cpp
if (!router_.route(mutableReq, resp))
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}
```

这说明框架边界划分得很清楚：

- `Router` 只报告结果
- `HttpServer` 决定 HTTP 层面的响应策略

---

## 13. Router 和 HttpServer 的配合关系

`Router` 本身不直接对外暴露给业务层，而是作为 `HttpServer` 的内部成员：

```cpp
router::Router router_;
```

然后由 `HttpServer` 对外提供更友好的注册接口：

```cpp
void Get(const std::string& path, HandlerPtr handler)
void Post(const std::string& path, HandlerPtr handler)
void addRoute(HttpRequest::Method method, const std::string& path, HandlerPtr handler)
```

这样业务层只需要写：

```cpp
httpServer_.Get("/chat", std::make_shared<ChatHandler>(this));
httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
```

而不用直接操作 `Router` 的底层容器。

这个封装关系非常合理：

- `Router` 是分发实现细节
- `HttpServer` 提供业务友好的 API

---

## 14. Router 在 ChatServer 中的实际使用

`ChatServer::initializeRouter()` 是理解实际业务路由的最好入口：

```cpp
httpServer_.Get("/", std::make_shared<ChatEntryHandler>(this));
httpServer_.Get("/entry", std::make_shared<ChatEntryHandler>(this));
httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
httpServer_.Post("/register", std::make_shared<ChatRegisterHandler>(this));
httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));
httpServer_.Get("/chat", std::make_shared<ChatHandler>(this));
httpServer_.Post("/chat/send", std::make_shared<ChatSendHandler>(this));
httpServer_.Post("/chat/history", std::make_shared<ChatHistoryHandler>(this));
httpServer_.Get("/chat/sessions", std::make_shared<ChatSessionsHandler>(this));
httpServer_.Post("/chat/tts", std::make_shared<ChatSpeechHandler>(this));
```

这段代码说明：

- ChatServer 只负责声明“路径对应哪个业务处理器”
- 真正的匹配、查找、调用，全都由 `Router` 完成

也就是说，`ChatServer` 写的是：

- 路由配置

`Router` 做的是：

- 路由执行

---

## 15. Router 的工程价值

这个 `Router` 最大的价值不只是功能，而是架构组织能力。

### 15.1 解耦框架和业务

有了 `Router`，`HttpServer` 不需要知道：

- 登录怎么做
- 聊天怎么做
- TTS 怎么做

它只要知道：

- 请求来了，交给 Router

### 15.2 让接口扩展成本降低

新增接口时，通常只需要：

1. 写一个新的 Handler
2. 在 `initializeRouter()` 注册一行

不需要改 `HttpServer` 核心逻辑。

### 15.3 让业务更容易模块化

每个接口可以独立成类：

- 登录一个类
- 注册一个类
- 聊天一个类
- 历史消息一个类

这比把所有逻辑塞进一个巨大的服务器函数强很多。

### 15.4 让框架具备可复用性

把 `Router`、`HttpRequest`、`HttpResponse`、`Session`、`Middleware` 这些抽象组合起来后，这个 `HttpServer` 理论上可以支撑别的业务应用，而不只限于 ChatServer。

---

## 16. 当前 Router 的优点

从学习和工程入门角度，这个 Router 有这些优点：

1. 结构简单清晰
2. 精准匹配与动态匹配都支持
3. callback 和对象式 handler 都支持
4. 路径参数提取已经具备基本能力
5. 与 `HttpServer`、`ChatServer` 的协作边界清楚
6. 对小中型项目足够实用

---

## 17. 当前 Router 的不足和改进点

### 17.1 动态 callback 分支有实现问题

当前代码：

```cpp
HttpRequest newReq(req);
extractPathParameters(match, newReq);
callback(req, resp);
```

问题：

- 提取后的参数写在 `newReq`
- 但 callback 拿到的却是 `req`

更合理的写法应该是：

```cpp
callback(newReq, resp);
```

否则动态 callback 的路径参数提取等于白做。

### 17.2 参数名没有保留语义

当前提取后的参数名是：

- `param1`
- `param2`

而不是：

- `id`
- `orderId`

这会让业务层读取参数时语义较弱。

### 17.3 动态路由是线性扫描

动态路由当前使用 `vector` 遍历匹配，规模小问题不大，但路由很多时效率会下降。

### 17.4 没有更高级的组织能力

例如成熟框架常见的：

- 路由分组
- 前缀路由
- 命名路由
- 自动参数名绑定
- 方法链式注册

这些当前都没有。

不过这不影响它作为一个教学和中小项目框架的价值。

---

## 18. 一句话总结 Router

可以把 `Router` 理解为 `HttpServer` 内部的“请求分诊台”：

- `HttpServer` 负责接收和解析请求
- `Router` 负责判断请求该交给谁
- `Handler` 负责执行业务逻辑

所以它的本质是：

> 把“HTTP 请求”映射为“具体业务处理器”的核心桥梁。

如果没有它，框架层会迅速被业务逻辑污染；有了它，整个项目才能形成现在这种：

- 框架层通用
- 业务层独立
- 扩展成本可控

---

## 19. 建议你继续怎么学 Router

如果你想把这一块真正吃透，建议继续做这 3 件事：

1. 顺着 `HttpServer::handleRequest()` 再读一遍 `router_.route()` 的调用上下文
2. 结合 `ChatServer::initializeRouter()` 自己画一张“路径 -> Handler”映射图
3. 尝试自己加一个动态路由，比如 `/user/:id`，验证路径参数提取过程

如果后面继续深入，最值得你亲手改的一个小练习就是：

1. 修复动态 callback 分支里 `callback(req, resp)` 的问题
2. 把 `param1` 风格改成保留真实参数名的实现

这样你就不只是“看懂 Router”，而是真正能改它、扩它了。
