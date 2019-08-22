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

#include "core/logging.hpp"  // temporary

#include <execinfo.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {

class DebugMutex;

namespace log {

class ContextDetails
{
public:
  using shared_type = std::shared_ptr<ContextDetails>;

  explicit ContextDetails(void *instance = nullptr)
    : instance_(instance)
  {}

  ContextDetails(shared_type ctx, shared_type parent, std::string context,
                 std::string filename = std::string(), int line = 0, void *instance = nullptr)
    : context_(std::move(context))
    , filename_(std::move(filename))
    , line_(line)
    , parent_(std::move(parent))
    , derived_from_(std::move(ctx))
    , instance_(instance)
  {}

  ContextDetails(shared_type parent, std::string context, std::string filename = std::string(),
                 int line = 0, void *instance = nullptr)
    : context_(std::move(context))
    , filename_(std::move(filename))
    , line_(line)
    , parent_(std::move(parent))
    , instance_(instance)
  {}

  ~ContextDetails() = default;

  shared_type parent()
  {
    return parent_;
  }

  shared_type derived_from()
  {
    return derived_from_;
  }

  std::string context(std::size_t n = std::size_t(-1)) const
  {
    if (context_.size() > n)
    {
      return context_.substr(0, n);
    }
    return context_;
  }

  std::string filename() const
  {
    return filename_;
  }

  int line() const
  {
    return line_;
  }

  std::thread::id thread_id() const
  {
    return id_;
  }

  void *instance() const
  {
    return instance_;
  }

private:
  std::string           context_ = "(root)";
  std::string           filename_;
  int                   line_ = 0;
  shared_type           parent_;
  shared_type           derived_from_;
  std::thread::id const id_       = std::this_thread::get_id();
  void *                instance_ = nullptr;
};

class Context
{
public:
  using shared_type = std::shared_ptr<ContextDetails>;

  explicit Context(void *instance = nullptr);
  Context(shared_type ctx, std::string const &context, std::string const &filename = "",
          int line = 0, void *instance = nullptr);
  explicit Context(std::string const &context, std::string const &filename = "", int line = 0,
                   void *instance = nullptr);

  Context(Context const &context)
    : details_{context.details_}
    , primary_{false}
  {}

  Context const &operator=(Context const &context) = delete;

  ~Context();

  shared_type details() const
  {
    return details_;
  }

private:
  shared_type                                    details_;
  bool                                           primary_ = true;
  std::chrono::high_resolution_clock::time_point created_;
};

class DefaultLogger
{
public:
  using shared_context_type = std::shared_ptr<ContextDetails>;

  DefaultLogger()          = default;
  virtual ~DefaultLogger() = default;

  enum class Level
  {
    ERROR     = 0,
    WARNING   = 1,
    INFO      = 2,
    DEBUG     = 3,
    HIGHLIGHT = 4
  };

  virtual void StartEntry(Level level, char const *name, shared_context_type ctx);

  template <typename T>
  void Append(T &&v)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    std::cout << std::forward<T>(v);
#endif
  }

  virtual void Append(std::string const &s)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    std::cout << s;
#endif
  }
};

namespace details {

class LogWrapper
{
public:
  using shared_context_type = std::shared_ptr<ContextDetails>;

  LogWrapper();

  ~LogWrapper();

  void DisableLogger();

  void Debug(std::vector<std::string> const &items);

  void SetContext(shared_context_type ctx);

  shared_context_type TopContext();

  void RegisterLock(fetch::DebugMutex *ptr);

  void RegisterUnlock(fetch::DebugMutex *ptr, double spent_time, const std::string &filename,
                      int line);

  void StackTrace(shared_context_type ctx, uint32_t max = uint32_t(-1), bool show_locks = true,
                  std::string const &trace_name = "Stack trace") const;

  void StackTrace(uint32_t max = uint32_t(-1), bool show_locks = true);

  void UpdateContextTime(shared_context_type ctx, double spent_time);

private:
  struct TimingDetails
  {
    double      total = 0;
    double      peak  = 0;
    uint64_t    calls = 0;
    int         line  = 0;
    std::string context;
    std::string filename;
  };

  std::unordered_set<fetch::DebugMutex *>        active_locks_;
  std::unordered_map<std::string, TimingDetails> mutex_timings_;

  std::unordered_map<std::string, TimingDetails> timings_;

  mutable std::mutex timing_mutex_;

  shared_context_type TopContextImpl();

  void PrintTrace(shared_context_type ctx, uint32_t max = uint32_t(-1)) const;
  std::unique_ptr<DefaultLogger>                           log_;
  mutable std::mutex                                       mutex_;
  std::unordered_map<std::thread::id, shared_context_type> context_;
};

}  // namespace details
}  // namespace log

extern log::details::LogWrapper logger;
}  // namespace fetch

#ifndef __FUNCTION_NAME__
#ifdef WIN32  // WINDOWS
#define __FUNCTION_NAME__ __FUNCTION__
#else  //*NIX
#define __FUNCTION_NAME__ __func__
#endif
#endif

#ifndef NDEBUG
#define FETCH_HAS_STACK_TRACE
#endif

#ifdef FETCH_HAS_STACK_TRACE

#define LOG_STACK_TRACE_POINT_WITH_INSTANCE \
  fetch::log::Context log_context(__FUNCTION_NAME__, __FILE__, __LINE__, this);

#define LOG_STACK_TRACE_POINT \
  fetch::log::Context log_context(__FUNCTION_NAME__, __FILE__, __LINE__);

#define LOG_LAMBDA_STACK_TRACE_POINT                                                         \
  fetch::log::Context log_lambda_context(log_context.details(), __FUNCTION_NAME__, __FILE__, \
                                         __LINE__)

#define LOG_CONTEXT_VARIABLE(name) std::shared_ptr<fetch::log::ContextDetails> name;

#define LOG_SET_CONTEXT_VARIABLE(name) name = fetch::logger.TopContext();

#define LOG_PRINT_STACK_TRACE(name, custom_name) \
  fetch::logger.StackTrace(name, uint32_t(-1), false, custom_name);

#else

#define LOG_STACK_TRACE_POINT_WITH_INSTANCE
#define LOG_STACK_TRACE_POINT
#define LOG_LAMBDA_STACK_TRACE_POINT
#define LOG_CONTEXT_VARIABLE(name)
#define LOG_SET_CONTEXT_VARIABLE(name)
#define LOG_PRINT_STACK_TRACE(name, custom_name)

#endif
