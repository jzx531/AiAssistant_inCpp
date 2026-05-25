# GomokuServer 设计建议

## 目标

如果要在 `AIApps` 下扩充一个新的 AI 应用，做成“智能五子棋”，建议新建一个独立子项目：`AIApps/GomokuServer`。

这样做的好处：

- 目录职责清晰，不和 `ChatServer` 逻辑混在一起
- 可以复用你当前的 `HttpServer`、`SessionManager`、路由、中间件体系
- 后续既可以做本地算法 AI，也可以扩展成接入大模型的棋局分析应用

## 建议目录结构

```text
AIApps/
  GomokuServer/
    include/
      GomokuServer.h
      handlers/
        GomokuEntryHandler.h
        GomokuMoveHandler.h
        GomokuAIMoveHandler.h
        GomokuResetHandler.h
        GomokuStateHandler.h
      game/
        GomokuBoard.h
        GomokuRuleEngine.h
        GomokuAI.h
        GomokuSession.h
      ai/
        GomokuStrategy.h
        MinimaxStrategy.h
        LlmGomokuStrategy.h
    src/
      GomokuServer.cpp
      handlers/
        GomokuEntryHandler.cpp
        GomokuMoveHandler.cpp
        GomokuAIMoveHandler.cpp
        GomokuResetHandler.cpp
        GomokuStateHandler.cpp
      game/
        GomokuBoard.cpp
        GomokuRuleEngine.cpp
        GomokuAI.cpp
        GomokuSession.cpp
      ai/
        GomokuStrategy.cpp
        MinimaxStrategy.cpp
        LlmGomokuStrategy.cpp
    resource/
      gomoku.html
      gomoku.css
      gomoku.js
      prompt.txt
    test/
      test_gomoku.cpp
```

## 每类文件的职责

### `GomokuServer.h/.cpp`

作为整个五子棋应用入口。

主要职责：

- 持有 `http::HttpServer`
- 初始化路由、中间件、会话
- 管理用户对应的棋局状态
- 提供统一的响应封装方法

例如可以维护：

```cpp
std::unordered_map<int, std::shared_ptr<GomokuSession>> gameSessions_;
std::mutex mutexForGameSessions_;
```

### `handlers/`

#### `GomokuEntryHandler`

- 返回五子棋页面 `gomoku.html`

#### `GomokuMoveHandler`

- 处理用户落子请求
- 校验坐标是否合法
- 校验当前位置是否为空
- 校验是否轮到用户落子
- 落子后更新棋局状态

#### `GomokuAIMoveHandler`

- 调用 AI 计算下一步落子
- 写回棋盘
- 判断 AI 是否获胜

#### `GomokuResetHandler`

- 重置当前棋局

#### `GomokuStateHandler`

- 返回当前棋局 JSON 状态

### `game/GomokuBoard.h/.cpp`

负责纯棋盘数据。

建议维护：

```cpp
static constexpr int kBoardSize = 32;
std::vector<std::vector<int>> board_;
```

建议提供这些接口：

- `placeStone(int x, int y, int color)`
- `isEmpty(int x, int y) const`
- `reset()`
- `toJson() const`
- `get(int x, int y) const`

其中颜色可以约定：

- `0`：空
- `1`：黑子
- `2`：白子

### `game/GomokuRuleEngine.h/.cpp`

负责纯规则判断，不掺杂 UI 和网络逻辑。

建议提供：

- `checkWin(const GomokuBoard&, int x, int y, int color)`
- `isValidMove(const GomokuBoard&, int x, int y)`
- `isBoardFull(const GomokuBoard&)`

### `game/GomokuSession.h/.cpp`

表示一整局棋的状态。

建议包含：

- 棋盘对象
- 当前回合
- 是否结束
- 胜者
- 对战模式

例如：

```cpp
class GomokuSession {
public:
    GomokuBoard board;
    int currentTurn = 1;
    bool gameOver = false;
    int winner = 0;
    std::string mode = "player-vs-ai";
};
```

### `game/GomokuAI.h/.cpp`

作为 AI 统一入口。

内部可以组合不同策略，例如：

- 简单启发式策略
- Minimax 搜索策略
- 大模型推荐策略

接口可以先做成：

```cpp
class GomokuAI {
public:
    std::pair<int, int> nextMove(const GomokuBoard& board, int aiColor);
};
```

### `ai/GomokuStrategy.h`

策略接口层，便于切换不同 AI 实现。

```cpp
class GomokuStrategy {
public:
    virtual ~GomokuStrategy() = default;
    virtual std::pair<int, int> nextMove(const GomokuBoard&, int aiColor) = 0;
};
```

### `ai/MinimaxStrategy.h/.cpp`

建议作为第一版智能对手。

优点：

- 不依赖外部模型
- 可控、稳定
- 易于调试
- 响应速度可接受

### `ai/LlmGomokuStrategy.h/.cpp`

如果后面要做“大模型五子棋分析”，再加这一层。

不建议一开始就让 LLM 直接决定落子是否合法。

更适合承担：

- 给用户推荐一步
- 解释为什么推荐这个位置
- 分析当前局势优劣
- 生成对局讲解

## 前端资源文件

### `resource/gomoku.html`

负责五子棋页面骨架：

- 32x32 棋盘容器
- 开始按钮
- 重置按钮
- 当前状态显示

### `resource/gomoku.js`

负责前端交互逻辑：

- 点击棋盘发送落子请求
- 拉取棋局状态
- 渲染棋子
- 请求 AI 落子

### `resource/gomoku.css`

负责页面样式。

### `resource/prompt.txt`

如果后面接入大模型，可以把提示词模板放在这里。

## 建议暴露的 HTTP 接口

```text
GET  /gomoku
GET  /gomoku/state
POST /gomoku/move
POST /gomoku/ai-move
POST /gomoku/reset
```

各接口含义：

- `GET /gomoku`：返回前端页面
- `GET /gomoku/state`：返回当前棋局状态
- `POST /gomoku/move`：用户落子
- `POST /gomoku/ai-move`：AI 落子
- `POST /gomoku/reset`：重置棋局

## 建议返回的 JSON 结构

### 用户落子返回

```json
{
  "success": true,
  "board": [[0,0,1],[...]],
  "nextTurn": 2,
  "winner": 0,
  "gameOver": false
}
```

### AI 落子返回

```json
{
  "success": true,
  "aiMove": { "x": 7, "y": 7 },
  "winner": 0,
  "gameOver": false
}
```

## 第一版最小可运行文件集合

如果只是先做最小版本，不需要一开始就把所有抽象层都补齐。

最少可以只写这些：

```text
AIApps/GomokuServer/
  include/
    GomokuServer.h
    handlers/
      GomokuEntryHandler.h
      GomokuMoveHandler.h
      GomokuAIMoveHandler.h
    game/
      GomokuBoard.h
      GomokuRuleEngine.h
      GomokuAI.h
  src/
    GomokuServer.cpp
    handlers/
      GomokuEntryHandler.cpp
      GomokuMoveHandler.cpp
      GomokuAIMoveHandler.cpp
    game/
      GomokuBoard.cpp
      GomokuRuleEngine.cpp
      GomokuAI.cpp
  resource/
    gomoku.html
    gomoku.js
    gomoku.css
```

## 建议的开发顺序

1. 先写 `GomokuBoard`
2. 再写 `GomokuRuleEngine`
3. 再写 `GomokuMoveHandler`
4. 再写 `gomoku.html` 和 `gomoku.js`
5. 再接 `GomokuAI`
6. 最后如果有需要，再扩展 `LlmGomokuStrategy`

## 第一版不建议先做的内容

- 不要先接大模型
- 不要先做联网对战
- 不要先做数据库持久化
- 不要先把系统拆得太重

第一版最推荐的目标是：

- 单人对 AI
- 页面可交互
- 可判胜负
- 可重置

## 额外建议

如果想和现有 `ChatServer` 风格保持一致，可以：

- `GomokuServer` 也使用 `SessionManager`
- 每个登录用户绑定自己的 `GomokuSession`
- handler 继续沿用 `RouterHandler` 风格
- 静态资源继续放在 `resource/`

如果后续要升级成“智能五子棋助手”，可以在五子棋页面中再叠加两个能力：

- AI 推荐下一步
- AI 局势讲解

这样它就不只是小游戏，而是一个真正的 AI 应用。
