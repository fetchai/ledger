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

#include "core/abstract_mutex.hpp"
#include "core/commandline/vt100.hpp"

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
namespace log {

class ReadableThread
{
public:
  static int GetThreadID(std::thread::id const &thread)
  {
    mutex_.lock();

    if (thread_number_.find(thread) == thread_number_.end())
    {
      thread_number_.emplace(thread, int(++thread_count_));
    }
    int thread_number = thread_number_[thread];
    mutex_.unlock();

    return thread_number;
  }

private:
  static std::map<std::thread::id, int> thread_number_;
  static int                            thread_count_;
  static std::mutex                     mutex_;
};

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
  std::string     context_ = "(root)";
  std::string     filename_;
  int             line_ = 0;
  shared_type     parent_;
  shared_type     derived_from_;
  std::thread::id id_       = std::this_thread::get_id();
  void *          instance_ = nullptr;
};

class Context
{
public:
  using shared_type = std::shared_ptr<ContextDetails>;

  Context(void *instance = nullptr);
  Context(shared_type ctx, std::string const &context, std::string const &filename = "",
          int line = 0, void *instance = nullptr);
  Context(std::string const &context, std::string const &filename = "", int line = 0,
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

  virtual void StartEntry(Level level, char const *name, shared_context_type ctx)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    using Clock     = std::chrono::system_clock;
    using Timepoint = Clock::time_point;
    using Duration  = Clock::duration;

    using namespace fetch::commandline::VT100;
    int color    = 9;
    int bg_color = 9;

    char const *level_name = "UNKNWN";
    switch (level)
    {
    case Level::INFO:
      color      = 3;
      level_name = "INFO  ";
      break;
    case Level::WARNING:
      color      = 6;
      level_name = "WARN  ";
      break;
    case Level::ERROR:
      color      = 1;
      level_name = "ERROR ";
      break;
    case Level::DEBUG:
      level_name = "DEBUG ";
      color      = 7;
      break;
    case Level::HIGHLIGHT:
      level_name = "HLIGHT";
      bg_color   = 4;
      color      = 7;
      break;
    }

    int const       thread_number = ReadableThread::GetThreadID(std::this_thread::get_id());
    Timepoint const now           = Clock::now();
    Duration const  duration      = now.time_since_epoch();

    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    // format and generate the time
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "[ " << GetColor(color, bg_color) << std::put_time(std::localtime(&now_c), "%F %T")
              << "." << std::setw(3) << millis << DefaultAttributes();

    // thread information
    std::cout << ", #" << std::setw(2) << thread_number << ' ' << level_name;

    // determine which of the two logging formats we should use
    bool use_name = name != nullptr;
    if (use_name && ctx->instance())
    {
      // in the case where we actually context information we should use this variant
      use_name = false;
    }

    if (use_name)
    {
      std::cout << ": " << std::setw(35) << name << " ] ";
    }
    else
    {
      std::cout << ": " << std::setw(15) << ctx->instance() << std::setw(20) << ctx->context(18)
                << " ] ";
    }
    std::cout << GetColor(color, bg_color);

#endif
  }

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

  virtual void CloseEntry(Level)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    using namespace fetch::commandline::VT100;
    std::cout << DefaultAttributes() << std::endl;
#endif
  }
};

namespace details {

class LogWrapper
{
public:
  using shared_context_type = std::shared_ptr<ContextDetails>;

  LogWrapper()
    : log_{std::make_unique<DefaultLogger>()}
  {}

  ~LogWrapper()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    DisableLogger();
  }

  void DisableLogger()
  {
    if (log_)
    {
      log_.reset();
    }
  }

  template <typename... Args>
  void Info(Args &&... args)
  {
    InfoWithName(nullptr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void InfoWithName(char const *name, Args &&... args)
  {
    Log(DefaultLogger::Level::INFO, name, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Warn(Args &&... args)
  {
    WarnWithName(nullptr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void WarnWithName(char const *name, Args &&... args)
  {
    Log(DefaultLogger::Level::WARNING, name, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Highlight(Args &&... args)
  {
    HighlightWithName(nullptr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void HighlightWithName(char const *name, Args &&... args)
  {
    Log(DefaultLogger::Level::HIGHLIGHT, name, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Debug(Args &&... args)
  {
    DebugWithName(nullptr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void DebugWithName(char const *name, Args &&... args)
  {
    Log(DefaultLogger::Level::DEBUG, name, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Error(Args &&... args)
  {
    ErrorWithName(nullptr, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void ErrorWithName(char const *name, Args &&... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      log_->StartEntry(DefaultLogger::Level::ERROR, name, TopContextImpl());
      Unroll<Args...>::Append(this, std::forward<Args>(args)...);
      log_->CloseEntry(DefaultLogger::Level::ERROR);

      StackTrace();
    }
  }

  void Debug(std::vector<std::string> const &items)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      log_->StartEntry(DefaultLogger::Level::DEBUG, nullptr, TopContextImpl());
      for (auto &item : items)
      {
        log_->Append(item);
      }
      log_->CloseEntry(DefaultLogger::Level::DEBUG);
    }
  }

  void SetContext(shared_context_type ctx)
  {
    std::thread::id             id = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      context_[id] = std::move(ctx);
    }
  }

  shared_context_type TopContext()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return TopContextImpl();
  }

  void RegisterLock(fetch::mutex::AbstractMutex *ptr)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      active_locks_.insert(ptr);
    }
  }

  void RegisterUnlock(fetch::mutex::AbstractMutex *ptr, double spent_time,
                      const std::string &filename, int line)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      std::stringstream ss;
      ss << filename << line;
      std::string s = ss.str();
      if (mutex_timings_.find(s) == mutex_timings_.end())
      {
        TimingDetails t;
        t.line     = line;
        t.context  = "Mutex";
        t.filename = filename;

        mutex_timings_[s] = t;
      }

      auto &t = mutex_timings_[s];
      t.total += spent_time;
      if (t.peak < spent_time)
      {
        t.peak = spent_time;
      }

      ++t.calls;

      auto it = active_locks_.find(ptr);
      if (it != active_locks_.end())
      {
        active_locks_.erase(it);
      }
    }
  }

  void StackTrace(shared_context_type ctx, uint32_t max = uint32_t(-1), bool show_locks = true,
                  std::string const &trace_name = "Stack trace") const
  {

    if (!ctx)
    {
      std::cerr << "Stack trace context invalid" << std::endl;
      return;
    }

    std::cout << trace_name << " for #" << ReadableThread::GetThreadID(ctx->thread_id()) << '\n';
    PrintTrace(ctx, max);

    if (show_locks)
    {
      std::vector<std::thread::id> locked_threads;

      std::cout << "\nActive locks:\n";
      for (auto &l : active_locks_)
      {
        std::cout << "  - " << l->AsString() << '\n';
        locked_threads.push_back(l->thread_id());
      }
      for (auto &id : locked_threads)
      {
        std::cout << "\nAdditionally trace for #" << ReadableThread::GetThreadID(id) << '\n';
        auto id_context{context_.find(id)};
        if (id_context != context_.cend())
        {
          PrintTrace(id_context->second);
          std::cout << '\n';
        }
      }
      std::cout << '\n';
    }
  }

  void StackTrace(uint32_t max = uint32_t(-1), bool show_locks = true)
  {
    shared_context_type ctx = TopContextImpl();
    StackTrace(ctx, max, show_locks);
  }

  void UpdateContextTime(shared_context_type ctx, double spent_time)
  {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    std::stringstream           ss;
    ss << ctx->context() << ", " << ctx->filename() << " " << ctx->line();
    std::string s = ss.str();
    if (timings_.find(s) == timings_.end())
    {
      TimingDetails t;
      t.line     = ctx->line();
      t.context  = ctx->context();
      t.filename = ctx->filename();

      timings_[s] = t;
    }

    auto &t = timings_[s];
    t.total += spent_time;
    if (t.peak < spent_time)
    {
      t.peak = spent_time;
    }

    ++t.calls;
  }

  void PrintTimings(std::size_t max = 50) const
  {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    std::lock_guard<std::mutex> lock2(mutex_);
    std::vector<TimingDetails>  all_timings;
    for (auto &t : timings_)
    {
      all_timings.push_back(t.second);
    }
    // [](TimingDetails const &a, TimingDetails const &b) { return (a.total /
    // a.calls) > (b.total / b.calls); }
    std::sort(all_timings.begin(), all_timings.end(),
              [](TimingDetails const &a, TimingDetails const &b) { return (a.peak) > (b.peak); });
    std::size_t N = std::min(max, all_timings.size());

    std::cout << "Profile for monitored function calls:\n";
    for (std::size_t i = 0; i < N; ++i)
    {
      std::cout << std::setw(3) << i << std::setw(20)
                << double(all_timings[i].total) / double(all_timings[i].calls) << " ";
      std::cout << std::setw(20) << all_timings[i].peak << " ";
      std::cout << std::setw(20) << all_timings[i].calls << " ";
      std::cout << std::setw(20) << all_timings[i].total << " ";
      std::cout << all_timings[i].context << " " << all_timings[i].filename << " "
                << all_timings[i].line << '\n';
    }
    std::cout << '\n';
  }

  void PrintMutexTimings(std::size_t max = 50) const
  {
    std::lock_guard<std::mutex> lock2(mutex_);
    std::lock_guard<std::mutex> lock(timing_mutex_);

    std::vector<TimingDetails> all_timings;
    for (auto &t : mutex_timings_)
    {
      all_timings.push_back(t.second);
    }

    std::sort(all_timings.begin(), all_timings.end(),
              [](TimingDetails const &a, TimingDetails const &b) {
                return (double(a.total) / double(a.calls)) > (double(b.total) / double(b.calls));
              });
    std::size_t N = std::min(max, all_timings.size());

    std::cout << "Mutex timings:\n";
    for (std::size_t i = 0; i < N; ++i)
    {
      std::cout << std::setw(3) << i << std::setw(20)
                << double(all_timings[i].total) / double(all_timings[i].calls) << " ";
      std::cout << std::setw(20) << all_timings[i].peak << " ";
      std::cout << std::setw(20) << all_timings[i].calls << " ";
      std::cout << std::setw(20) << all_timings[i].total << " ";
      std::cout << all_timings[i].context << " " << all_timings[i].filename << " "
                << all_timings[i].line << '\n';
    }
    std::cout << '\n';
  }

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

  std::unordered_set<fetch::mutex::AbstractMutex *> active_locks_;
  std::unordered_map<std::string, TimingDetails>    mutex_timings_;

  std::unordered_map<std::string, TimingDetails> timings_;

  mutable std::mutex timing_mutex_;

  template <typename... Args>
  void Log(DefaultLogger::Level level, char const *name, Args &&... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_)
    {
      log_->StartEntry(level, name, TopContextImpl());
      Unroll<Args...>::Append(this, std::forward<Args>(args)...);
      log_->CloseEntry(level);
    }
  }

  shared_context_type TopContextImpl()
  {
    std::thread::id id = std::this_thread::get_id();

    auto it = context_.find(id);
    if (it != context_.end())
    {
      return it->second;
    }
    else
    {
      shared_context_type ret{std::make_shared<ContextDetails>()};
      context_.emplace(id, ret);
      return ret;
    }
  }

  template <typename T, typename... Args>
  struct Unroll
  {
    static void Append(LogWrapper *cls, T &&v, Args &&... args)
    {
      cls->log_->Append(std::forward<T>(v));
      Unroll<Args...>::Append(cls, std::forward<Args>(args)...);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    static void Append(LogWrapper *cls, T &&v)
    {
      cls->log_->Append(std::forward<T>(v));
    }
  };

  void PrintTrace(shared_context_type ctx, uint32_t max = uint32_t(-1)) const
  {
    using namespace fetch::commandline::VT100;
    std::size_t i = 0;
    while (ctx)
    {
      std::cout << std::setw(3) << i << ": In thread #"
                << ReadableThread::GetThreadID(ctx->thread_id()) << ": ";
      std::cout << GetColor(5, 9) << ctx->context() << DefaultAttributes() << " " << ctx->filename()
                << ", ";
      std::cout << GetColor(3, 9) << ctx->line() << DefaultAttributes() << std::endl;

      if (ctx->derived_from())
      {
        std::cout << "*";
        ctx = ctx->derived_from();
      }
      else
      {
        ctx = ctx->parent();
      }
      ++i;
      if (i >= max)
      {
        break;
      }
    }
  }

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

#define ERROR_BACKTRACE                                         \
  {                                                             \
    constexpr int            framesMax = 20;                    \
    std::vector<std::string> results;                           \
                                                                \
    void *callstack[framesMax];                                 \
                                                                \
    int frames = backtrace(callstack, framesMax);               \
                                                                \
    char **frameStrings = backtrace_symbols(callstack, frames); \
                                                                \
    std::ostringstream trace;                                   \
                                                                \
    for (int i = 0; i < frames; ++i)                            \
    {                                                           \
      trace << frameStrings[i] << std::endl;                    \
    }                                                           \
    free(frameStrings);                                         \
                                                                \
    FETCH_LOG_INFO(LOGGING_NAME, "Trace: \n", trace.str());     \
  }

#if 1
#define FETCH_LOG_PROMISE()
#else
#define FETCH_LOG_PROMISE() FETCH_LOG_WARN(LOGGING_NAME, "Promise wait: ", __FILE__, ":", __LINE__)
#endif

// Logging macros

// Debug
#if FETCH_COMPILE_LOGGING_LEVEL >= 4
#define FETCH_LOG_DEBUG_ENABLED
#define FETCH_LOG_DEBUG(name, ...) fetch::logger.DebugWithName(name, __VA_ARGS__)
#else
#define FETCH_LOG_DEBUG(name, ...) (void)name
#endif

// Info
#if FETCH_COMPILE_LOGGING_LEVEL >= 3
#define FETCH_LOG_INFO_ENABLED
#define FETCH_LOG_INFO(name, ...) fetch::logger.InfoWithName(name, __VA_ARGS__)
#else
#define FETCH_LOG_INFO(name, ...) (void)name
#endif

// Warn
#if FETCH_COMPILE_LOGGING_LEVEL >= 2
#define FETCH_LOG_WARN_ENABLED
#define FETCH_LOG_WARN(name, ...) fetch::logger.WarnWithName(name, __VA_ARGS__)
#else
#define FETCH_LOG_WARN(name, ...) (void)name
#endif

// Error
#if FETCH_COMPILE_LOGGING_LEVEL >= 1
#define FETCH_LOG_ERROR_ENABLED
#define FETCH_LOG_ERROR(name, ...) fetch::logger.ErrorWithName(name, __VA_ARGS__)
#else
#define FETCH_LOG_ERROR(name, ...) (void)name
#endif

#define FETCH_LOG_VARIABLE(x) (void)x
