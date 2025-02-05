#pragma once

#include "Server.hpp"
#include <boost/json.hpp>
#include <map>
#include <optional>

namespace MyHttp {

namespace bj = boost::json;

class ServerBuilder
{
public:
  ServerBuilder(Executor ex, std::string logName = "MyHttp::ServerBuilder")
    : mExecutor(std::move(ex))
    , mLogger(std::move(logName))
  {
  }

  ServerBuilder(Executor ex,
                Executor acptEx,
                std::string logName = "MyHttp::ServerBuilder")
    : mExecutor(std::move(ex))
    , mAcptExecutor(std::move(acptEx))
    , mLogger(std::move(logName))
  {
  }

  /// 服务器构建函子
  using BuildServer = std::unique_ptr<Server> (*)(
    Executor ex,             // 主执行器
    Executor acptEx,         // 接受连接的执行器（可能为假值）
    std::string logName,     // 日志名称
    const bj::value& details // 详细配置
  );

  /// 注册服务器
  void register_server(const std::string& name, BuildServer buildServer);

  /// 注册 MyHttp 的内置服务器
  void register_builtins();

  /// 服务器配置类
  struct ServerConfig
  {
    /// 类型
    std::string mType;
    /// 监听地址
    std::string mHost;
    /// 监听端口
    std::uint16_t mPort;
    /// 监听队列长度
    int mBacklog{ ba::socket_base::max_listen_connections };
    /// 详细配置
    bj::value mDetails;

    /// 转换到 JSON 值对象
    bj::value to_jval() const noexcept;
    /// 从 JSON 值对象设置，出错时抛出异常
    void jval_to(bj::value jval) noexcept(false);
  };

  /// 服务器列表
  using Servers =
    std::map<std::string, std::pair<ServerConfig, std::unique_ptr<Server>>>;

  /// 从 JSON 值对象构建
  Servers build_jval(bj::value jval);

  /// 从 JSON 字符串构建
  Servers build_json(std::string_view json);

  /// 从 JSON 配置文件构建
  Servers build_json_file(const std::string& path);

  /// 启动所有服务器
  static void start_all(Servers& servers);

  /// 停止所有服务器
  static void stop_all(Servers& servers);

private:
  My::log::Logger mLogger;
  Executor mExecutor, mAcptExecutor;
  std::map<std::string, BuildServer> mBuilders;
};

} // namespace MyHttp
