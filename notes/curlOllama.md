# Curl Ollama Test

这个文件用于手动测试本地 `Ollama` 是否能被当前环境访问。

## 1. Windows PowerShell 测试

如果 `Ollama` 跑在 Windows 本机，先测试模型列表：

```powershell
curl http://127.0.0.1:11434/api/tags
```

测试生成接口：

```powershell
curl http://127.0.0.1:11434/api/generate -Method Post -ContentType "application/json" -Body '{"model":"qwen2.5:7b","prompt":"Return only JSON: {\"x\":12,\"y\":8}","stream":false}'
```

如果通了，返回里应该包含：

```json
{
  "model": "qwen2.5:7b",
  "response": "...",
  "done": true
}
```

## 2. WSL / Linux 测试

如果你的后端程序跑在 WSL 里，`127.0.0.1` 指向的是 WSL 自己，不一定能访问到 Windows 上的 `Ollama`。

先测试 `host.docker.internal`：

```bash
curl http://host.docker.internal:11434/api/tags
```

再测试生成接口：

```bash
curl http://host.docker.internal:11434/api/generate \
  -H 'Content-Type: application/json' \
  -d '{"model":"qwen2.5:7b","prompt":"Return only JSON: {\"x\":12,\"y\":8}","stream":false}'
```

## 3. 如果 `host.docker.internal` 不通

在 WSL 中查看 Windows 主机网关地址：

```bash
cat /etc/resolv.conf
```

通常会看到：

```text
nameserver 172.xx.xx.x
```

然后把这个 IP 替换到下面命令里：

```bash
curl http://172.xx.xx.x:11434/api/tags
```

```bash
curl http://172.xx.xx.x:11434/api/generate \
  -H 'Content-Type: application/json' \
  -d '{"model":"qwen2.5:7b","prompt":"Return only JSON: {\"x\":12,\"y\":8}","stream":false}'
```

## 4. 预期结果

如果请求成功，说明：

1. `Ollama` 服务已经启动
2. `qwen2.5:7b` 模型已经可用
3. 当前运行环境可以访问 `Ollama`

如果失败：

1. `Connection refused` / `Couldn't connect to server`
   说明地址不对，或者 `Ollama` 没启动
2. `model not found`
   说明模型还没拉取成功
3. 返回 404 / 其他错误
   说明接口地址写错了

## 5. 这次为什么要改网关地址

你当前的运行环境分成了两部分：

1. `Ollama` 跑在 Windows 上
2. `Gomoku` 后端跑在 WSL 里

这时 `127.0.0.1` 不再表示“同一台服务”。

### 5.1 为什么 `127.0.0.1` 不通

在 Windows PowerShell 里：

```powershell
curl http://127.0.0.1:11434/api/tags
```

能成功，是因为这里访问的是 Windows 本机上的 `Ollama`。

但在 WSL 里：

```bash
curl http://127.0.0.1:11434/api/tags
```

访问的是 WSL 自己的回环地址，不是 Windows 主机，所以会出现：

```text
curl: (7) Failed to connect to 127.0.0.1 port 11434
```

### 5.2 为什么要用 WSL 网关地址

WSL 访问 Windows 主机时，通常要通过 WSL 虚拟网络里的“默认网关”地址访问。

本次你机器里查到的是：

```text
default via 172.22.48.1
```

所以在 WSL 里，Windows 主机可以通过这个地址访问：

```text
172.22.48.1
```

也就是说，原来代码里的：

```cpp
http://127.0.0.1:11434/api/generate
```

对于 WSL 内运行的后端来说是错的；应该改成：

```cpp
http://172.22.48.1:11434/api/generate
```

### 5.3 这次实际验证过程

先在 Windows 上确认 `Ollama` 正常：

```powershell
curl http://127.0.0.1:11434/api/tags
```

再在 WSL 中确认网关地址可达：

```bash
curl http://172.22.48.1:11434/api/tags
```

成功返回模型列表后，再测试生成接口：

```bash
curl http://172.22.48.1:11434/api/generate \
  -H 'Content-Type: application/json' \
  -d '{"model":"qwen2.5:7b","prompt":"Return only JSON: {\"x\":12,\"y\":8}","stream":false}'
```

这一步成功，说明：

1. WSL 到 Windows 主机的网络链路已打通
2. `Ollama` 服务可从 WSL 访问
3. `GomokuAI.cpp` 里应该使用网关地址，而不是 `127.0.0.1`

### 5.4 这次对应到代码的修改

本次将 `GomokuAI.cpp` 中的地址从：

```cpp
url_("http://127.0.0.1:11434/api/generate")
```

改成了：

```cpp
url_("http://172.22.48.1:11434/api/generate")
```

这样当后端程序运行在 WSL 中时，就能正确访问 Windows 上的 `Ollama`。

### 5.5 后续建议

这个网关 IP 可能会因为 WSL 网络重建而变化，所以更稳妥的方式不是写死 IP，而是：

1. 从配置文件读取 `Ollama` 地址
2. 或从环境变量读取 `OLLAMA_URL`

当前这次修改是为了先打通现有环境，属于可工作的最小修复。
