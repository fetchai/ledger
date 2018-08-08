#pragma once
#include "core/abstract_mutex.hpp"
#include "core/commandline/vt100.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <execinfo.h>
#include <iomanip>
#include <iostream>
#include <mutex>
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
      thread_number_[thread] = int(++thread_count_);
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

  ContextDetails(void *instance = nullptr) : instance_(instance)
  {
    id_ = std::this_thread::get_id();
  }

  ContextDetails(shared_type ctx, shared_type parent, std::string context,
                 std::string filename = "", int const &line = 0, void *instance = nullptr)
    : context_(std::move(context))
    , filename_(std::move(filename))
    , line_(line)
    , parent_(std::move(parent))
    , derived_from_(std::move(ctx))
    , instance_(instance)
  {
    id_ = std::this_thread::get_id();
  }

  ContextDetails(shared_type parent, std::string context, std::string filename = "",
                 int const &line = 0, void *instance = nullptr)
    : context_(std::move(context))
    , filename_(std::move(filename))
    , line_(line)
    , parent_(std::move(parent))
    , instance_(instance)
  {
    id_ = std::this_thread::get_id();
  }

  ~ContextDetails() = default;

  shared_type parent() { return parent_; }

  shared_type derived_from() { return derived_from_; }

  std::string context(std::size_t const &n = std::size_t(-1)) const
  {
    if (context_.size() > n) return context_.substr(0, n);
    return context_;
  }
  std::string filename() const { return filename_; }
  int         line() const { return line_; }

  std::thread::id thread_id() const { return id_; }
  void *          instance() const { return instance_; }

private:
  std::string     context_  = "(root)";
  std::string     filename_ = "";
  int             line_     = 0;
  shared_type     parent_;
  shared_type     derived_from_;
  std::thread::id id_       = std::this_thread::get_id();
  void *          instance_ = nullptr;
};

class Context
{
public:
  Context(void *instance = nullptr);
  using shared_type = std::shared_ptr<ContextDetails>;
  Context(shared_type ctx, std::string const &context, std::string const &filename = "",
          int const &line = 0, void *instance = nullptr);
  Context(std::string const &context, std::string const &filename = "", int const &line = 0,
          void *instance = nullptr);

  Context(Context const &context)
  {
    details_ = context.details_;
    primary_ = false;
  }

  Context const &operator=(Context const &context) = delete;

  ~Context();

  shared_type details() const { return details_; }

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

  enum
  {
    ERROR     = 0,
    WARNING   = 1,
    INFO      = 2,
    DEBUG     = 3,
    HIGHLIGHT = 4
  };

  virtual void StartEntry(int type, shared_context_type ctx)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    using namespace fetch::commandline::VT100;
    int color = 9, bg_color = 9;
    switch (type)
    {
    case INFO:
      color = 3;
      break;
    case WARNING:
      color = 6;
      break;
    case ERROR:
      color = 1;
      break;
    case DEBUG:
      color = 7;
      break;
    case HIGHLIGHT:
      bg_color = 4;
      color    = 7;
      break;
    }

    int thread_number = ReadableThread::GetThreadID(std::this_thread::get_id());

    std::chrono::system_clock::time_point now      = std::chrono::system_clock::now();
    auto                                  duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "[ " << GetColor(color, bg_color)
              << std::put_time(std::localtime(&now_c), "%F %T");

    std::cout << "." << std::setw(3) << millis << DefaultAttributes() << ", #" << std::setw(2)
              << thread_number;
    std::cout << ": " << std::setw(15) << ctx->instance() << std::setw(20) << ctx->context(18)
              << " ] ";
    std::cout << GetColor(color, bg_color);

#endif
  }

  template <typename T>
  void Append(T const &v)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    std::cout << v;
#endif
  }

  virtual void Append(std::string const &s)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    std::cout << s;
#endif
  }

  virtual void CloseEntry(int type)
  {
#ifndef FETCH_DISABLE_COUT_LOGGING
    using namespace fetch::commandline::VT100;
    std::cout << DefaultAttributes() << std::endl;
#endif
  }

private:
};

namespace details {

class LogWrapper
{
public:
  using shared_context_type = std::shared_ptr<ContextDetails>;

  LogWrapper() { log_ = new DefaultLogger(); }

  ~LogWrapper()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_ != nullptr) delete log_;
  }

  void DisableLogger()
  {
    if (log_ != nullptr) delete log_;
    log_ = nullptr;
  }

  template <typename... Args>
  void Info(Args... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::INFO, TopContextImpl());
      Unroll<Args...>::Append(this, args...);
      this->log_->CloseEntry(DefaultLogger::INFO);
    }
  }

  template <typename... Args>
  void Warn(Args... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::WARNING, TopContextImpl());
      Unroll<Args...>::Append(this, args...);
      this->log_->CloseEntry(DefaultLogger::WARNING);
    }
  }

  template <typename... Args>
  void Highlight(Args... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::HIGHLIGHT, TopContextImpl());
      Unroll<Args...>::Append(this, args...);
      this->log_->CloseEntry(DefaultLogger::HIGHLIGHT);
    }
  }

  template <typename... Args>
  void Error(Args... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::ERROR, TopContextImpl());
      Unroll<Args...>::Append(this, args...);
      this->log_->CloseEntry(DefaultLogger::ERROR);

      StackTrace();
    }

    //    exit(-1);
  }

  template <typename... Args>
  void Debug(Args... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::DEBUG, TopContextImpl());
      Unroll<Args...>::Append(this, args...);
      this->log_->CloseEntry(DefaultLogger::DEBUG);
    }
  }

  void Debug(const std::vector<std::string> &items)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
    {
      this->log_->StartEntry(DefaultLogger::DEBUG, TopContextImpl());
      for (auto &item : items)
      {
        this->log_->Append(item);
      }
      this->log_->CloseEntry(DefaultLogger::DEBUG);
    }
  }

  void SetContext(shared_context_type ctx)
  {
    std::thread::id             id = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
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
    if (this->log_ != nullptr)
    {
      active_locks_.insert(ptr);
    }
  }

  void RegisterUnlock(fetch::mutex::AbstractMutex *ptr, double spent_time,
                      const std::string &filename, int line)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (this->log_ != nullptr)
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
      if (t.peak < spent_time) t.peak = spent_time;

      ++t.calls;

      auto it = active_locks_.find(ptr);
      if (it != active_locks_.end())
      {
        active_locks_.erase(it);
      }
    }
  }

  void StackTrace(shared_context_type ctx, uint32_t max = uint32_t(-1), bool show_locks = true,
                  std::string const &trace_name = "Stack trace")
  {

    if (!ctx)
    {
      std::cerr << "Stack trace context invalid" << std::endl;
      return;
    }

    std::cout << trace_name << " for #" << ReadableThread::GetThreadID(ctx->thread_id())
              << std::endl;
    PrintTrace(ctx, max);

    if (show_locks)
    {
      std::vector<std::thread::id> locked_threads;

      std::cout << std::endl;
      std::cout << "Active locks: " << std::endl;
      for (auto &l : active_locks_)
      {
        std::cout << "  - " << l->AsString() << std::endl;
        locked_threads.push_back(l->thread_id());
      }
      std::cout << std::endl;
      for (auto &id : locked_threads)
      {
        std::cout << "Additionally trace for #" << ReadableThread::GetThreadID(id) << std::endl;
        ctx = context_[id];
        PrintTrace(ctx);
        std::cout << std::endl;
      }
    }
  }

  void StackTrace(uint32_t max = uint32_t(-1), bool show_locks = true)
  {
    shared_context_type ctx = TopContextImpl();
    StackTrace(ctx, max, show_locks);
  }

  void UpdateContextTime(shared_context_type const &ctx, double spent_time)
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
    if (t.peak < spent_time) t.peak = spent_time;

    ++t.calls;
  }

  void PrintTimings(std::size_t max = 50)
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

    std::cout << "Profile for monitored function calls: " << std::endl;
    for (std::size_t i = 0; i < N; ++i)
    {
      std::cout << std::setw(3) << i << std::setw(20)
                << double(all_timings[i].total) / double(all_timings[i].calls) << " ";
      std::cout << std::setw(20) << all_timings[i].peak << " ";
      std::cout << std::setw(20) << all_timings[i].calls << " ";
      std::cout << std::setw(20) << all_timings[i].total << " ";
      std::cout << all_timings[i].context << " " << all_timings[i].filename << " "
                << all_timings[i].line;
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  void PrintMutexTimings(std::size_t max = 50)
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

    std::cout << "Mutex timings: " << std::endl;
    for (std::size_t i = 0; i < N; ++i)
    {
      std::cout << std::setw(3) << i << std::setw(20)
                << double(all_timings[i].total) / double(all_timings[i].calls) << " ";
      std::cout << std::setw(20) << all_timings[i].peak << " ";
      std::cout << std::setw(20) << all_timings[i].calls << " ";
      std::cout << std::setw(20) << all_timings[i].total << " ";
      std::cout << all_timings[i].context << " " << all_timings[i].filename << " "
                << all_timings[i].line;
      std::cout << std::endl;
    }
    std::cout << std::endl;
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

  shared_context_type TopContextImpl()
  {
    std::thread::id id = std::this_thread::get_id();

    shared_context_type ret;

    auto it = context_.find(id);
    if (it != context_.end())
    {
      ret = it->second;
    }
    else
    {
      ret = std::make_shared<ContextDetails>();
      context_.emplace(id, ret);
    }

    return ret;
  }

  template <typename T, typename... Args>
  struct Unroll
  {
    static void Append(LogWrapper *cls, T const &v, Args... args)
    {
      cls->log_->Append(v);
      Unroll<Args...>::Append(cls, args...);
    }
  };

  template <typename T>
  struct Unroll<T>
  {
    static void Append(LogWrapper *cls, T const &v) { cls->log_->Append(v); }
  };

  void PrintTrace(shared_context_type ctx, uint32_t max = uint32_t(-1))
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
      if (i >= max) break;
    }
  }

  DefaultLogger *                                          log_;
  mutable std::mutex                                       mutex_;
  std::unordered_map<std::thread::id, shared_context_type> context_;
};
}  // namespace details
}  // namespace log

extern log::details::LogWrapper logger;
}  // namespace fetch

// TODO(EJF): Move somewhere else

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
    fetch::logger.Info("Trace: \n", trace.str());               \
  }

#else

#define LOG_STACK_TRACE_POINT_WITH_INSTANCE
#define LOG_STACK_TRACE_POINT
#define LOG_LAMBDA_STACK_TRACE_POINT
#define LOG_CONTEXT_VARIABLE(name)
#define LOG_SET_CONTEXT_VARIABLE(name)
#define LOG_PRINT_STACK_TRACE(name, custom_name)

#endif

//#define LOG_STACK_TRACE_POINT
//#define LOG_LAMBDA_STACK_TRACE_POINT
