//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/logging.hpp"
#include "core/mutex.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <unordered_map>

namespace fetch {
namespace {

auto LogCounter(char const *type, char const *hint)
{
  return telemetry::Registry::Instance().CreateCounter(
      std::string("ledger_") + type + "log_messages_total",
      std::string("The number of ") + hint + "log messages printed");
}

class LogRegistry
{
public:
  // Construction / Destruction
  LogRegistry();
  LogRegistry(LogRegistry const &) = delete;
  LogRegistry(LogRegistry &&)      = delete;
  ~LogRegistry()                   = default;

  void        Log(LogLevel level, char const *name, std::string &&message);
  void        SetLevel(char const *name, LogLevel level);
  LogLevelMap GetLogLevelMap();

  // Operators
  LogRegistry &operator=(LogRegistry const &) = delete;
  LogRegistry &operator=(LogRegistry &&) = delete;

private:
  using Logger     = spdlog::logger;
  using LoggerPtr  = std::shared_ptr<Logger>;
  using Registry   = std::unordered_map<std::string, LoggerPtr>;
  using Mutex      = std::mutex;
  using CounterPtr = telemetry::CounterPtr;

  Logger &GetLogger(char const *name);

  Mutex    lock_;
  Registry registry_;

  // Telemetry
  CounterPtr log_messages_{LogCounter("", "")};
  CounterPtr log_trace_messages_{LogCounter("trace_", "trace ")};
  CounterPtr log_debug_messages_{LogCounter("debug_", "debug ")};
  CounterPtr log_info_messages_{LogCounter("info_", "info ")};
  CounterPtr log_warn_messages_{LogCounter("warn_", "warning ")};
  CounterPtr log_error_messages_{LogCounter("error_", "error ")};
  CounterPtr log_critical_messages_{LogCounter("critical_", "critical ")};
};

constexpr LogLevel DEFAULT_LEVEL = LogLevel::INFO;

LogRegistry registry_;

constexpr LogLevel ConvertToLevel(spdlog::level::level_enum level) noexcept
{
  switch (level)
  {
  case spdlog::level::trace:
    return LogLevel::TRACE;
  case spdlog::level::debug:
    return LogLevel::DEBUG;
  case spdlog::level::info:
    return LogLevel::INFO;
  case spdlog::level::warn:
    return LogLevel::WARNING;
  case spdlog::level::err:
    return LogLevel::ERROR;
  case spdlog::level::critical:
    return LogLevel::CRITICAL;
  default:
    return LogLevel::INFO;
  }
}

constexpr spdlog::level::level_enum ConvertFromLevel(LogLevel level) noexcept
{
  switch (level)
  {
  case LogLevel::TRACE:
    return spdlog::level::trace;
  case LogLevel::DEBUG:
    return spdlog::level::debug;
  case LogLevel::INFO:
    return spdlog::level::info;
  case LogLevel::WARNING:
    return spdlog::level::warn;
  case LogLevel::ERROR:
    return spdlog::level::err;
  case LogLevel::CRITICAL:
    return spdlog::level::critical;
  default:
    return spdlog::level::info;
  }
}

LogRegistry::LogRegistry()
{
  spdlog::set_level(
      spdlog::level::trace);  // this should be kept in sync with the compilation level
  spdlog::set_pattern("%^[%L]%$ %Y/%m/%d %T | %-30n : %v");
}

void LogRegistry::Log(LogLevel level, char const *name, std::string &&message)
{
  {
    FETCH_LOCK(lock_);
    GetLogger(name).log(ConvertFromLevel(level), message);
  }

  // telemetry
  log_messages_->increment();
  switch (level)
  {
  case LogLevel::TRACE:
    log_trace_messages_->increment();
    break;
  case LogLevel::DEBUG:
    log_debug_messages_->increment();
    break;
  case LogLevel::INFO:
    log_info_messages_->increment();
    break;
  case LogLevel::WARNING:
    log_warn_messages_->increment();
    break;
  case LogLevel::ERROR:
    log_error_messages_->increment();
    break;
  case LogLevel::CRITICAL:
    log_critical_messages_->increment();
    break;
  }
}

void LogRegistry::SetLevel(char const *name, LogLevel level)
{
  FETCH_LOCK(lock_);

  auto it = registry_.find(name);
  if (it != registry_.end())
  {
    it->second->set_level(ConvertFromLevel(level));
  }
}

LogLevelMap LogRegistry::GetLogLevelMap()
{
  FETCH_LOCK(lock_);

  LogLevelMap level_map{};
  level_map.reserve(registry_.size());

  for (auto const &element : registry_)
  {
    level_map[element.first] = ConvertToLevel(element.second->level());
  }

  return level_map;
}

LogRegistry::Logger &LogRegistry::GetLogger(char const *name)
{
  auto it = registry_.find(name);
  if (it == registry_.end())
  {
    // create the new logger instance
    auto logger = spdlog::stdout_color_mt(name, spdlog::color_mode::automatic);
    logger->set_level(ConvertFromLevel(DEFAULT_LEVEL));

    // keep a reference of it
    registry_[name] = logger;

    return *logger;
  }
  else
  {
    return *(it->second);
  }
}

}  // namespace

void SetLogLevel(char const *name, LogLevel level)
{
  registry_.SetLevel(name, level);
}

void Log(LogLevel level, char const *name, std::string message)
{
  registry_.Log(level, name, std::move(message));
}

LogLevelMap GetLogLevelMap()
{
  return registry_.GetLogLevelMap();
}

}  // namespace fetch
