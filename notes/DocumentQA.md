# Document QA With A Handmade Vector Database

这份文档说明如何在你当前项目架构上做一个“文档问答助手”，并且不依赖现成向量数据库，而是自己手搓一个最小可用版本。

目标是：

1. 用户上传文档
2. 后端解析文本
3. 文本切片
4. 为每个切片生成向量
5. 向量入库
6. 用户提问时做相似度检索
7. 把检索结果拼进 prompt，让模型基于文档回答

这本质上是一个最小版 RAG 系统。

## 1. 为什么这个项目适合做 Document QA

你当前已经有：

1. HTTP 服务框架
2. Session 管理
3. ChatServer 结构
4. 文件上传相关代码基础
5. 与大模型交互的基础设施

所以做文档问答时，不需要推翻重来，只需要新增：

1. 文档上传入口
2. 文本解析流程
3. 向量化流程
4. 手搓向量数据库
5. 检索增强问答流程

## 2. 整体架构

建议拆成下面几个模块：

1. DocumentUploadHandler
2. DocumentParseService
3. TextChunkService
4. EmbeddingService
5. VectorStore
6. DocumentSearchService
7. DocumentQAHandler

数据流如下：

1. 上传文档
2. 提取纯文本
3. 文本切块
4. 每块生成 embedding
5. embedding 与元数据保存到本地向量库
6. 用户提问
7. 问题生成 embedding
8. 在向量库中检索 topK 相关 chunk
9. 组装 prompt
10. 调用 LLM 生成答案

## 3. 你要支持哪些文档类型

建议先分阶段做。

### 第一阶段

先只支持：

1. `txt`
2. `md`
3. `json`
4. `csv`

因为这些最容易解析，直接读文本即可。

### 第二阶段

再支持：

1. `pdf`
2. `docx`

这两类需要额外解析库，复杂度明显更高。

如果你想先尽快做出可用版本，第一阶段就足够了。

## 4. 文档问答的核心步骤

### 4.1 上传

新增接口：

1. `POST /docqa/upload`
2. `POST /docqa/ask`
3. `GET /docqa/list`
4. `GET /docqa/chunks?docId=...`

上传时保存：

1. 原始文件
2. 文档元信息
3. 解析后的纯文本

建议元信息包括：

1. `docId`
2. `sessionId` 或 `userId`
3. `fileName`
4. `fileType`
5. `uploadTime`
6. `textLength`

### 4.2 文本解析

对 `txt/md/json/csv`：

1. 直接读取文本内容
2. 做基础清洗

清洗建议：

1. 统一换行
2. 去掉过多空行
3. 去掉明显无意义空白
4. 保留标题和段落边界

### 4.3 文本切片

不要整篇文档直接做 embedding，否则：

1. 粒度太粗
2. 检索不准
3. prompt 太大

建议切片策略：

1. 每块 300 到 800 字符
2. 相邻块重叠 50 到 120 字符

例如：

1. chunkSize = 500
2. overlap = 80

每个 chunk 要记录：

1. `chunkId`
2. `docId`
3. `index`
4. `text`
5. `startOffset`
6. `endOffset`

### 4.4 生成向量

你需要一个 embedding 模型，把：

1. chunk 文本
2. 用户 query

都转成固定维度向量。

可选路线：

1. 调本地 `Ollama` 的 embedding 接口
2. 调在线 embedding API
3. 自己接 ONNX embedding 模型

如果想贴合你当前项目，最容易先做的是：

1. 直接调用 HTTP embedding API

你后续也可以把 embedding 模型本地化。

## 5. 手搓向量数据库怎么做

这里的“手搓向量数据库”并不意味着要做一个高性能 Milvus/Faiss 替代品，而是做一个当前项目可用的最小版本。

最小目标：

1. 能存向量
2. 能按文档/用户隔离
3. 能做相似度检索
4. 能返回 topK chunk

## 6. 向量库最小数据结构

可以先用内存 + 文件落盘。

### 6.1 ChunkRecord

```cpp
struct ChunkRecord {
    std::string chunkId;
    std::string docId;
    std::string sessionId;
    int chunkIndex;
    std::string text;
    std::vector<float> embedding;
};
```

### 6.2 VectorStore

```cpp
class VectorStore {
public:
    void add(const ChunkRecord& record);
    std::vector<ChunkRecord> search(
        const std::string& sessionId,
        const std::vector<float>& queryEmbedding,
        size_t topK) const;

    void saveToDisk(const std::string& path) const;
    void loadFromDisk(const std::string& path);

private:
    std::vector<ChunkRecord> records_;
};
```

这就是最小版“向量数据库”。

## 7. 相似度检索怎么实现

最常见的是余弦相似度。

公式：

1. `dot(a, b)`
2. `norm(a)`
3. `cosine = dot(a, b) / (|a| * |b|)`

伪代码：

```cpp
float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    float dot = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    return dot / (std::sqrt(normA) * std::sqrt(normB) + 1e-8f);
}
```

搜索流程：

1. 遍历当前用户的所有 chunk
2. 计算 query 与 chunk 的相似度
3. 按分数排序
4. 返回 topK

这就是最基础的暴力检索。

### 为什么这够用

如果你的数据规模一开始只是：

1. 几十篇文档
2. 每篇几十到几百个 chunk

那暴力遍历完全够用，开发成本最低。

## 8. 向量库落盘方式

建议先用 JSON 落盘，简单可读。

例如：

```json
[
  {
    "chunkId": "c1",
    "docId": "d1",
    "sessionId": "s1",
    "chunkIndex": 0,
    "text": "...",
    "embedding": [0.12, -0.33, 0.57]
  }
]
```

优点：

1. 容易调试
2. 不依赖数据库扩展
3. 和你现在项目风格兼容

缺点：

1. 数据大了会慢
2. 内存和 I/O 效率一般

但对于第一版是完全够用的。

## 9. 问答阶段怎么做

当用户提问时：

1. 用户问题生成 embedding
2. 去向量库检索 topK chunk
3. 把这些 chunk 拼进上下文
4. 再调用 LLM 回答

### 检索后 prompt 示例

```text
You are a document question answering assistant.
Answer only based on the provided context.
If the answer is not in the context, say you do not know.

Question:
<user question>

Context:
[Chunk 1]
...

[Chunk 2]
...

[Chunk 3]
...
```

这个阶段的 prompt 重点不是“让模型自由发挥”，而是：

1. 只基于检索结果回答
2. 不知道就说不知道
3. 避免幻觉

## 10. 在你当前架构里怎么落地

建议新增一个新模块，例如：

1. `AIApps/DocumentQAServer/`

或者更省事：

1. 直接扩展现有 `ChatServer`

### 推荐目录结构

```text
AIApps/DocumentQA/
  include/
    handlers/
      DocumentUploadHandler.h
      DocumentAskHandler.h
    service/
      DocumentParseService.h
      TextChunkService.h
      EmbeddingService.h
      DocumentSearchService.h
    store/
      VectorStore.h
    model/
      ChunkRecord.h
      DocumentMeta.h
  src/
    handlers/
    service/
    store/
```

## 11. 建议的最小接口

### 11.1 上传文档

`POST /docqa/upload`

输入：

1. 文件

输出：

1. `docId`
2. `chunkCount`
3. `status`

### 11.2 提问

`POST /docqa/ask`

输入：

```json
{
  "docId": "d1",
  "question": "这份文档主要讲了什么？"
}
```

输出：

```json
{
  "answer": "...",
  "references": [
    {"chunkId": "c3", "score": 0.88},
    {"chunkId": "c7", "score": 0.83}
  ]
}
```

## 12. 第一版不要做得太重

第一版建议只做这些：

1. 支持 `txt/md`
2. 固定 chunkSize 和 overlap
3. 固定 topK = 3 或 5
4. 内存向量库 + JSON 落盘
5. 暴力检索
6. 单用户 session 隔离

这样更容易先跑通。

## 13. 后续增强方向

当第一版跑通后，再做：

1. 支持 PDF / DOCX
2. 按标题和段落做更智能分块
3. 支持多文档联合检索
4. 支持 metadata filter
5. 支持 rerank
6. 支持更快的 ANN 检索
7. 支持 embedding 缓存
8. 支持文档删除和增量更新

## 14. 手搓向量数据库的关键点总结

你真正要自己实现的核心只有这些：

1. 一个保存 `text + embedding + metadata` 的结构
2. 一个添加记录的接口
3. 一个余弦相似度函数
4. 一个 topK 排序检索函数
5. 一个 JSON 落盘和加载机制

这就足够称为当前项目里的“手搓向量数据库”。

## 15. 推荐实现顺序

最推荐按这个顺序开发：

1. 先支持 `txt/md` 上传
2. 写文本切片服务
3. 接 embedding 接口
4. 写最小版 `VectorStore`
5. 做 `/docqa/ask`
6. 把检索结果接给 LLM
7. 最后再做前端页面

## 16. 一句话结论

在你当前架构下，做文档问答助手最合理的方式是：

1. 复用现有 HTTP 和会话体系
2. 新增文档上传、切片、embedding、检索模块
3. 用内存 + JSON 落盘手搓一个最小版向量数据库
4. 用余弦相似度做 topK 检索
5. 把检索结果拼进 prompt 做 RAG 问答

这条路线技术上清晰、实现成本可控，而且非常适合你当前项目继续扩展。
