# Gomoku 接口约定

本文档描述当前 `resource/gomoku.js` 使用到的接口请求与响应格式。

棋盘统一约定：

- 棋盘大小：`32 x 32`
- 横纵坐标范围：`0 ~ 31`
- 棋子值：
- `0` 表示空位
- `1` 表示黑子
- `2` 表示白子

## 1. 获取当前棋局

### 请求

```http
GET /gomoku/state
```

请求体：无。

### 成功响应

```json
{
  "board": [[0, 0, 0], [0, 1, 0]],
  "nextTurn": 1,
  "gameOver": false,
  "winner": 0
}
```

字段说明：

- `board`：二维数组，大小应为 `32 x 32`
- `nextTurn`：下一手轮到谁
- `gameOver`：当前对局是否结束
- `winner`：赢家，`0` 表示未分胜负或平局，`1` 表示黑子，`2` 表示白子

## 2. 玩家落子

### 请求

```http
POST /gomoku/move
Content-Type: application/json
```

请求体：

```json
{
  "x": 10,
  "y": 12
}
```

字段说明：

- `x`：横坐标，范围 `0 ~ 31`
- `y`：纵坐标，范围 `0 ~ 31`

当前前端约定：

- 这个接口用于玩家执黑子时落子
- 黑子落完后，后端应返回 `nextTurn = 2`

### 成功响应

```json
{
  "success": true,
  "board": [[0, 0, 0], [0, 1, 0]],
  "nextTurn": 2,
  "gameOver": false,
  "winner": 0
}
```

字段说明：

- `success`：是否处理成功
- `board`：更新后的棋盘
- `nextTurn`：下一手，正常应切到白子 `2`
- `gameOver`：本次落子后是否结束
- `winner`：赢家

### 失败响应建议

```json
{
  "success": false,
  "message": "invalid move"
}
```

例如这些情况可以返回失败：

- 坐标越界
- 当前位置已有棋子
- 当前不是黑子回合
- 对局已经结束

## 3. AI 落子

### 请求

```http
POST /gomoku/ai-move
Content-Type: application/json
```

当前前端没有发送请求体，可以为空：

```json
{}
```

当前前端约定：

- 只有白子回合时才会发这个请求
- 这个接口表示“让 AI 执行白子落子”

### 成功响应

```json
{
  "success": true,
  "board": [[0, 0, 0], [0, 1, 2]],
  "aiMove": {
    "x": 11,
    "y": 12
  },
  "nextTurn": 1,
  "gameOver": false,
  "winner": 0
}
```

字段说明：

- `success`：是否处理成功
- `board`：AI 落子后的棋盘
- `aiMove.x`：AI 落子横坐标
- `aiMove.y`：AI 落子纵坐标
- `nextTurn`：下一手，正常应切回黑子 `1`
- `gameOver`：AI 落子后是否结束
- `winner`：赢家

### 失败响应建议

```json
{
  "success": false,
  "message": "ai move failed"
}
```

例如这些情况可以返回失败：

- 当前不是白子回合
- 对局已经结束
- AI 没有找到合法落点

## 4. 重置棋局

### 请求

```http
POST /gomoku/reset
Content-Type: application/json
```

当前前端没有依赖请求体，可以为空：

```json
{}
```

### 成功响应

推荐返回：

```json
{
  "success": true,
  "board": [[0, 0, 0], [0, 0, 0]],
  "nextTurn": 1,
  "gameOver": false,
  "winner": 0
}
```

虽然当前前端本地会直接重置，但后端最好也返回完整状态，方便后续联调。

## 5. 字段统一约定

建议所有接口尽量统一这些字段：

```json
{
  "success": true,
  "board": [],
  "nextTurn": 1,
  "gameOver": false,
  "winner": 0,
  "message": ""
}
```

推荐含义：

- `success`：接口是否成功
- `board`：最新棋盘状态
- `nextTurn`：下一手轮到谁
- `gameOver`：是否结束
- `winner`：胜者
- `message`：错误或提示信息

## 6. 后端实现时的关键配合点

为了和当前前端逻辑完全配合，后端应满足：

1. `POST /gomoku/move` 成功后返回 `nextTurn = 2`
2. `POST /gomoku/ai-move` 成功后返回 `nextTurn = 1`
3. `GET /gomoku/state` 返回完整棋盘
4. `board` 必须始终是二维数组
5. 坐标统一使用从 `0` 开始的下标

## 7. 当前前端依赖总结

`gomoku.js` 当前明确依赖这些响应字段：

- `/gomoku/state`
  - `board`
  - `nextTurn`
  - `gameOver`
  - `winner`

- `/gomoku/move`
  - `board`
  - `nextTurn`
  - `gameOver`
  - `winner`

- `/gomoku/ai-move`
  - `board`
  - `aiMove`
  - `nextTurn`
  - `gameOver`
  - `winner`

- `/gomoku/reset`
  - 当前前端未强依赖响应体，但建议返回完整状态
