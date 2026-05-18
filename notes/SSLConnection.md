这两个函数本质上是在做：

```text id="mw1f0h"
SSL层 和 应用层/TCP层 之间的数据缓冲
```

但是你当前代码里：

# 它们几乎没真正发挥作用

因为整个数据流并没有完整接上。

下面详细讲。

---

# 一、onEncrypted()

```cpp id="yoz97z"
void SslConnection::onEncrypted(const char* data, size_t len) 
{
    writeBuffer_.append(data, len);
    conn_->send(&writeBuffer_);
}
```

作用：

```text id="6f7e93"
发送加密后的 TLS 数据
```

即：

```text id="cckmfp"
SSL_write()
    ↓
TLS Record（密文）
    ↓
onEncrypted()
    ↓
muduo TCP send()
```

---

# 数据流理解

这里的：

```cpp id="2frhjd"
data
```

并不是：

# 应用层明文

而是：

# OpenSSL 输出的 TLS 密文

例如：

```text id="jlwmk9"
17 03 03 00 45 ...
```

这种 TLS Record。

---

# writeBuffer_

```cpp id="w45yr6"
writeBuffer_.append(data, len);
```

说明：

你想做：

# 异步发送缓冲

即：

```text id="f4mztq"
SSL层
   ↓
writeBuffer_
   ↓
muduo发送队列
```

这属于标准 Reactor 模型。

---

# conn_->send(&writeBuffer_)

muduo：

```cpp id="hhv8gx"
TcpConnection::send(Buffer*)
```

一般会：

* 尝试立即发送
* 未发送完加入 outputBuffer_

属于：

# 非阻塞发送

---

# 这里的问题

你当前代码里：

# onEncrypted 根本没被调用

因为：

你实际使用的是：

```cpp id="u4aj34"
BIO_s_mem()
```

而不是：

```cpp id="jlwm29"
custom BIO
```

所以：

```cpp id="u5g2lq"
bioWrite()
```

不会触发。

---

# 你当前真正发送路径

实际上你用的是：

```cpp id="u01z0w"
SSL_write()
↓
writeBio_
↓
BIO_read()
↓
conn_->send()
```

即：

```cpp id="nljlwm"
send()
{
    SSL_write()

    while(BIO_pending())
    {
        BIO_read()
        conn_->send()
    }
}
```

所以：

```cpp id="r9v6xv"
onEncrypted()
```

属于：

# 没用到的旧架构代码

---

# 二、onDecrypted()

```cpp id="3llx1i"
void SslConnection::onDecrypted(const char* data, size_t len) 
{
    decryptedBuffer_.append(data, len);
}
```

作用：

```text id="xrwf6t"
缓存解密后的应用层数据
```

即：

```text id="u3qlln"
TCP密文
   ↓
SSL_read()
   ↓
应用层明文
   ↓
decryptedBuffer_
```

---

# decryptedBuffer_

这里：

```cpp id="sljlwm"
decryptedBuffer_
```

通常是：

```cpp id="b3dden"
muduo::net::Buffer
```

用途：

# TLS record 重组

因为：

一次：

```cpp id="fdhq9w"
SSL_read()
```

不一定得到完整应用层消息。

---

# 举例

比如：

客户端发送：

```text id="3b4v5m"
HTTP GET /index.html
```

可能：

```text id="36k1az"
SSL_read()
```

第一次只得到：

```text id="wdx6n4"
GET /in
```

第二次：

```text id="f4jz5v"
dex.html
```

所以：

需要：

```cpp id="6fr9kk"
decryptedBuffer_
```

累计。

---

# 但你当前代码也没真正用它

因为：

你后面：

```cpp id="l8jlwm"
messageCallback_(...)
```

直接处理：

```cpp id="l5zj7x"
decryptedBuffer
```

局部变量。

而不是：

```cpp id="jlwmvi"
decryptedBuffer_
```

成员变量。

所以：

```cpp id="b3djlwm"
onDecrypted()
```

实际上也没进入主流程。

---

# 三、你这里其实混了两套设计

这是最关键的。

---

# 第一套设计（当前真正运行）

# Memory BIO 模式

```text id="mjlwmj"
SSL_write
   ↓
writeBio_
   ↓
BIO_read
   ↓
conn_->send()
```

以及：

```text id="jlwm5d"
TCP recv
   ↓
BIO_write(readBio_)
   ↓
SSL_read()
```

这是：

# 正确的现代异步TLS模式

你现在实际上走的是这个。

---

# 第二套设计（废弃了一半）

# Custom BIO 模式

你原本似乎想做：

```text id="k8jlwm"
SSL
  ↓
custom BIO callbacks
  ↓
onEncrypted/onDecrypted
```

即：

```cpp id="zjlwm2"
bioWrite()
{
    onEncrypted()
}
```

这种结构。

---

# 但最后没完成

因为：

你最终：

```cpp id="jlwm1f"
BIO_new(BIO_s_mem())
```

了。

所以：

```cpp id="jlwmu8"
createCustomBioMethod()
```

完全没接入。

---

# 四、你现在最合理的做法

---

# 推荐

## 删除：

```cpp id="jlwmgk"
onEncrypted()
onDecrypted()
bioWrite()
bioRead()
bioCtrl()
createCustomBioMethod()
```

因为：

你根本没用 custom BIO。

---

# 保留：

```cpp id="jlwm24"
BIO_s_mem()
```

这是正确路线。

---

# 五、正确结构应该是

---

# 接收路径

```text id="jlwm9h"
muduo TCP recv
        ↓
BIO_write(readBio_)
        ↓
SSL_read()
        ↓
应用层messageCallback
```

---

# 发送路径

```text id="jlwm4u"
应用层send()
       ↓
SSL_write()
       ↓
writeBio_
       ↓
BIO_read()
       ↓
muduo conn_->send()
```

---

# 六、你现在真正缺的核心函数

你应该抽象：

```cpp id="njlwm4"
flushWriteBio()
```

---

# 推荐实现

```cpp id="jlwmry"
void SslConnection::flushWriteBio()
{
    char buf[4096];

    while (BIO_pending(writeBio_) > 0)
    {
        int n = BIO_read(writeBio_, buf, sizeof(buf));

        if (n > 0)
        {
            conn_->send(buf, n);
        }
    }
}
```

---

# 然后：

## 在：

```cpp id="jlwm7t"
SSL_write()
```

后调用：

```cpp id="s7jlwm"
flushWriteBio();
```

---

# 在：

```cpp id="6jlwmr"
SSL_do_handshake()
```

后也调用：

```cpp id="jlwmja"
flushWriteBio();
```

否则：

TLS 握手包发不出去。

---

# 七、总结

你这两个函数：

```cpp id="jlwm83"
onEncrypted()
onDecrypted()
```

本质上是：

# “旧设计遗留代码”

因为：

你已经转向：

# Memory BIO 架构

而不是：

# Custom BIO 架构

所以它们现在：

| 函数          | 状态     |
| ----------- | ------ |
| onEncrypted | 基本没用   |
| onDecrypted | 基本没用   |
| custom BIO  | 没接入    |
| Memory BIO  | 当前真正运行 |

---

你现在实际上已经接近：

# 一个真正可工作的异步 TLS Reactor 框架

这已经是：

* muduo
* nginx
* libevent-openssl
* boost::beast SSL

这一类架构思路了。
