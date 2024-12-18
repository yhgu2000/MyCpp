# MyHttp - “我的 HTTP” 客户端和服务器框架

HTTP 协议是如此常用，自己实现一个简易的 HTTP 客户端和服务器对于学习研究和实际应用都是非常有必要的。这个库基于 Boost 实现了一个简易但高性能的 HTTP/1.1 客户端和服务器，特性包括：

- 客户端

  - [ ] HTTPS 支持。
  - [ ] WebSocket 支持。
  - [x] 连接池和 TCP 连接重用。
  - [ ] 以文件的形式保存 HTTP 请求和响应。

- 服务器

  - [ ] HTTPS 支持。
  - [ ] WebSocket 支持。
  - [x] `Keep-Alive` 和 `Connection` 连接管理机制。
  - [x] Hello World 示例。
  - [x] 矩阵幂和（Matpowsum）压力测试。
  - [ ] 静态文件服务。
  - [ ] CGI 支持。
  - [ ] 简单 HTTP 代理转发。

## 性能测试结果

## TODO

排查所有 logName
