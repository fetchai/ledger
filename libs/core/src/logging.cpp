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

class LogRegistry
{
public:
  // Construction / Destruction
  LogRegistry();
  LogRegistry(LogRegistry const &) = delete;
  LogRegistry(LogRegistry &&)      = delete;
  ~LogRegistry()                   = default;

  void Log(LogLevel level, char const *name, std::string &&message);
  void SetLevel(char const *name, LogLevel level);
  void SetGlobalLevel(LogLevel level);

  LogLevelMap GetLogLevelMap();

  // Operators
  LogRegistry &operator=(LogRegistry const &) = delete;
  LogRegistry &operator=(LogRegistry &&) = delete;

  LogLevel global_level() const
  {
    return global_level_;
  }

private:
  using Logger     = spdlog::logger;
  using LoggerPtr  = std::shared_ptr<Logger>;
  using Registry   = std::unordered_map<std::string, LoggerPtr>;
  using Mutex      = std::mutex;
  using CounterPtr = telemetry::CounterPtr;

  Logger &GetLogger(char const *name);

  Mutex    lock_;
  Registry registry_;
  LogLevel global_level_{LogLevel::TRACE};

  // Telemetry
  CounterPtr log_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_messages_total", "The number of log messages printed")};
  CounterPtr log_trace_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_trace_messages_total", "The number of trace log messages printed")};
  CounterPtr log_debug_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_debug_messages_total", "The number of debug log messages printed")};
  CounterPtr log_info_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_info_messages_total", "The number of info log messages printed")};
  CounterPtr log_warn_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_warn_messages_total", "The number of warning log messages printed")};
  CounterPtr log_error_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_error_messages_total", "The number of error log messages printed")};
  CounterPtr log_critical_messages_{telemetry::Registry::Instance().CreateCounter(
      "ledger_log_critical_messages_total", "The number of critical log messages printed")};
};

constexpr LogLevel DEFAULT_LEVEL = LogLevel::INFO;

LogRegistry registry_;

LogLevel ConvertToLevel(spdlog::level::level_enum level)
{
  LogLevel new_level = LogLevel::INFO;

  switch (level)
  {
  case spdlog::level::trace:
    new_level = LogLevel::TRACE;
    break;
  case spdlog::level::debug:
    new_level = LogLevel::DEBUG;
    break;
  case spdlog::level::info:
    new_level = LogLevel::INFO;
    break;
  case spdlog::level::warn:
    new_level = LogLevel::WARNING;
    break;
  case spdlog::level::err:
    new_level = LogLevel::ERROR;
    break;
  case spdlog::level::critical:
    new_level = LogLevel::CRITICAL;
    break;
  case spdlog::level::off:
    break;
  }

  return new_level;
}

spdlog::level::level_enum ConvertFromLevel(LogLevel level)
{
  spdlog::level::level_enum new_level = spdlog::level::info;

  switch (level)
  {
  case LogLevel::TRACE:
    new_level = spdlog::level::trace;
    break;
  case LogLevel::DEBUG:
    new_level = spdlog::level::debug;
    break;
  case LogLevel::INFO:
    new_level = spdlog::level::info;
    break;
  case LogLevel::WARNING:
    new_level = spdlog::level::warn;
    break;
  case LogLevel::ERROR:
    new_level = spdlog::level::err;
    break;
  case LogLevel::CRITICAL:
    new_level = spdlog::level::critical;
    break;
  }

  return new_level;
}

LogRegistry::LogRegistry()
{
  spdlog::set_level(
      spdlog::level::trace);  // this should be kept in sync with the compilation level
  spdlog::set_pattern("%^[%L]%$ %Y/%m/%d %T | %-30n : %v");
}

void LogRegistry::Log(LogLevel level, char const *name, std::string &&message)
{
  if (level < global_level_)
  {
    return;
  }

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

void LogRegistry::SetGlobalLevel(LogLevel level)
{
  global_level_ = level;
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

void SetGlobalLogLevel(LogLevel level)
{
  registry_.SetGlobalLevel(level);
}

void Log(LogLevel level, char const *name, std::string &&message)
{
  registry_.Log(level, name, std::move(message));
}

LogLevelMap GetLogLevelMap()
{
  return registry_.GetLogLevelMap();
}

}  // namespace fetch
