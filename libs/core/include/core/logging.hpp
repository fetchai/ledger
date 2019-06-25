#pragma once
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

#include <sstream>
#include <string>
#include <unordered_map>

namespace fetch {
namespace detail {

template <typename T, typename... Args>
struct Unroll
{
  static void Apply(std::ostream &stream, T &&v, Args &&... args)
  {
    stream << std::forward<T>(v);
    Unroll<Args...>::Apply(stream, std::forward<Args>(args)...);
  }
};

template <typename T>
struct Unroll<T>
{
  static void Apply(std::ostream &stream, T &&v)
  {
    stream << std::forward<T>(v);
  }
};

/**
 * String formatter
 * @tparam Args
 * @param args
 * @return
 */
template <typename... Args>
std::string Format(Args &&... args)
{
  // unroll all the arguments and generate the formatted output
  std::ostringstream oss;
  Unroll<Args...>::Apply(oss, std::forward<Args>(args)...);
  return oss.str();
}

} // namespace detail

enum class LogLevel
{
  TRACE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  CRITICAL,
};

using LogLevelMap = std::unordered_map<std::string, LogLevel>;


/// @name Log Library Functions
/// @{

/**
 * Configure the runtime logging level
 *
 * @param name The name of the logger
 * @param level The level to be set by the logging implementation
 */
void SetLogLevel(char const *name, LogLevel level);

/**
 * Log a simple message
 *
 * @param level The level of log message
 * @param name The name of the origin
 * @param message The message
 */
void Log(LogLevel level, char const *name, std::string &&message);

/**
 * Retrieve the current map of active loggers and the configured level
 *
 * @return The current map of levels
 */
LogLevelMap GetLogLevelMap();

/// @}


/// @name Helper Wrappers
/// @{

template <typename... Args>
void LogTraceV2(char const *name, Args &&... args)
{
  Log(LogLevel::TRACE, name, detail::Format(std::forward<Args>(args)...));
}

template <typename... Args>
void LogDebugV2(char const *name, Args &&... args)
{
  Log(LogLevel::DEBUG, name, detail::Format(std::forward<Args>(args)...));
}

template <typename... Args>
void LogInfoV2(char const *name, Args &&... args)
{
  Log(LogLevel::INFO, name, detail::Format(std::forward<Args>(args)...));
}

template <typename... Args>
void LogWarningV2(char const *name, Args &&... args)
{
  Log(LogLevel::WARNING, name, detail::Format(std::forward<Args>(args)...));
}

template <typename... Args>
void LogErrorV2(char const *name, Args &&... args)
{
  Log(LogLevel::ERROR, name, detail::Format(std::forward<Args>(args)...));
}

template <typename... Args>
void LogCriticalV2(char const *name, Args &&... args)
{
  Log(LogLevel::CRITICAL, name, detail::Format(std::forward<Args>(args)...));
}

/// @}

/// @name Logging Macros
/// @{

// Debug
#if FETCH_COMPILE_LOGGING_LEVEL >= 6
#define FETCH_LOG_TRACE_ENABLED
#define FETCH_LOG_TRACE(name, ...) fetch::LogTraceV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_TRACE(name, ...) (void)name
#endif

// Debug
#if FETCH_COMPILE_LOGGING_LEVEL >= 5
#define FETCH_LOG_DEBUG_ENABLED
#define FETCH_LOG_DEBUG(name, ...) fetch::LogDebugV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_DEBUG(name, ...) (void)name
#endif

// Info
#if FETCH_COMPILE_LOGGING_LEVEL >= 4
#define FETCH_LOG_INFO_ENABLED
#define FETCH_LOG_INFO(name, ...) fetch::LogInfoV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_INFO(name, ...) (void)name
#endif

// Warn
#if FETCH_COMPILE_LOGGING_LEVEL >= 3
#define FETCH_LOG_WARN_ENABLED
#define FETCH_LOG_WARN(name, ...) fetch::LogWarningV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_WARN(name, ...) (void)name
#endif

// Error
#if FETCH_COMPILE_LOGGING_LEVEL >= 2
#define FETCH_LOG_ERROR_ENABLED
#define FETCH_LOG_ERROR(name, ...) fetch::LogErrorV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_ERROR(name, ...) (void)name
#endif

// Critical
#if FETCH_COMPILE_LOGGING_LEVEL >= 1
#define FETCH_LOG_CRITICAL_ENABLED
#define FETCH_LOG_CRITICAL(name, ...) fetch::LogCriticalV2(name, __VA_ARGS__)
#else
#define FETCH_LOG_CRITICAL(name, ...) (void)name
#endif

#define FETCH_LOG_VARIABLE(x) (void)x

/// @}

} // namespace fetch