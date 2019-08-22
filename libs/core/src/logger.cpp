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

#include "core/fetch_backward.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"

// Add functionality to print a stack trace when program-terminating signals such as sigsegv are
// found
std::function<void(std::string)> backward::SignalHandling::_on_signal;
backward::SignalHandling         sh([](std::string const &fatal_msg) {
  FETCH_LOG_ERROR("FETCH_FATAL_SIGNAL_HANDLER", fatal_msg);
});

namespace fetch {

namespace {

class ReadableThread
{
public:
  static int GetThreadID(std::thread::id const &thread)
  {
    FETCH_LOCK(mutex_);

    if (thread_number_.find(thread) == thread_number_.end())
    {
      thread_number_.emplace(thread, int(++thread_count_));
    }

    return thread_number_[thread];
  }

private:
  static std::map<std::thread::id, int> thread_number_;
  static int                            thread_count_;
  static std::mutex                     mutex_;
};

}  // namespace

std::map<std::thread::id, int> ReadableThread::thread_number_ = std::map<std::thread::id, int>();
int                            ReadableThread::thread_count_  = 0;
std::mutex                     ReadableThread::mutex_;

log::details::LogWrapper logger;

namespace log {
namespace details {

LogWrapper::LogWrapper()
  : log_{std::make_unique<DefaultLogger>()}
{}

LogWrapper::~LogWrapper()
{
  FETCH_LOCK(mutex_);
  DisableLogger();
}

void LogWrapper::DisableLogger()
{
  if (log_)
  {
    log_.reset();
  }
}

void LogWrapper::Debug(std::vector<std::string> const &items)
{
  FETCH_LOCK(mutex_);
  if (log_)
  {
    log_->StartEntry(DefaultLogger::Level::DEBUG, nullptr, TopContextImpl());
    for (auto &item : items)
    {
      log_->Append(item);
    }
  }
}

void LogWrapper::SetContext(shared_context_type ctx)
{
  std::thread::id id = std::this_thread::get_id();
  FETCH_LOCK(mutex_);
  if (log_)
  {
    context_[id] = std::move(ctx);
  }
}

LogWrapper::shared_context_type LogWrapper::TopContext()
{
  FETCH_LOCK(mutex_);
  return TopContextImpl();
}

void LogWrapper::RegisterLock(fetch::DebugMutex *ptr)
{
  FETCH_LOCK(mutex_);
  if (log_)
  {
    active_locks_.insert(ptr);
  }
}

void LogWrapper::RegisterUnlock(fetch::DebugMutex *ptr, double spent_time,
                                const std::string &filename, int line)
{
  FETCH_LOCK(mutex_);
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

void LogWrapper::StackTrace(shared_context_type ctx, uint32_t max, bool show_locks,
                            std::string const &trace_name) const
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
      std::cout << "  - "
                << "Locked by thread #" << ReadableThread::GetThreadID(l->thread_id()) << " in "
                << l->filename() << " on " << l->line() << '\n';

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

void LogWrapper::StackTrace(uint32_t max, bool show_locks)
{
  shared_context_type ctx = TopContextImpl();
  StackTrace(ctx, max, show_locks);
}

void LogWrapper::UpdateContextTime(shared_context_type ctx, double spent_time)
{
  FETCH_LOCK(timing_mutex_);
  std::stringstream ss;
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

LogWrapper::shared_context_type LogWrapper::TopContextImpl()
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

void LogWrapper::PrintTrace(shared_context_type ctx, uint32_t max) const
{
  std::size_t i = 0;
  while (ctx)
  {
    std::cout << std::setw(3) << i << ": In thread #"
              << ReadableThread::GetThreadID(ctx->thread_id()) << ": ";
    std::cout << ctx->context() << " " << ctx->filename() << ", ";
    std::cout << ctx->line() << std::endl;

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

}  // namespace details

Context::Context(void *instance)
  : details_{std::make_shared<ContextDetails>(instance)}
  , created_{std::chrono::high_resolution_clock::now()}
{
  fetch::logger.SetContext(details_);
}

Context::Context(shared_type ctx, std::string const &context, std::string const &filename, int line,
                 void *instance)
  : details_{std::make_shared<ContextDetails>(ctx, fetch::logger.TopContext(), context, filename,
                                              line, instance)}
  , created_{std::chrono::high_resolution_clock::now()}
{
  fetch::logger.SetContext(details_);
}

Context::Context(std::string const &context, std::string const &filename, int line, void *instance)
  : details_{std::make_shared<ContextDetails>(fetch::logger.TopContext(), context, filename, line,
                                              instance)}
  , created_{std::chrono::high_resolution_clock::now()}
{
  fetch::logger.SetContext(details_);
}

Context::~Context()
{
  std::chrono::high_resolution_clock::time_point end_time =
      std::chrono::high_resolution_clock::now();
  double total_time =
      double(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - created_).count());
  fetch::logger.UpdateContextTime(details_, total_time);

  if (primary_ && details_->parent())
  {
    fetch::logger.SetContext(details_->parent());
  }
}

void DefaultLogger::StartEntry(Level level, char const *name, shared_context_type ctx)
{
#ifndef FETCH_DISABLE_COUT_LOGGING
  using Clock     = std::chrono::system_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  char const *level_name = "UNKNWN";
  switch (level)
  {
  case Level::INFO:
    level_name = "INFO  ";
    break;
  case Level::WARNING:
    level_name = "WARN  ";
    break;
  case Level::ERROR:
    level_name = "ERROR ";
    break;
  case Level::DEBUG:
    level_name = "DEBUG ";
    break;
  case Level::HIGHLIGHT:
    level_name = "HLIGHT";
    break;
  }

  int const       thread_number = ReadableThread::GetThreadID(std::this_thread::get_id());
  Timepoint const now           = Clock::now();
  Duration const  duration      = now.time_since_epoch();

  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

  // format and generate the time
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::cout << "[ " << std::put_time(std::localtime(&now_c), "%F %T") << "." << std::setw(3)
            << millis;

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
#endif
}

}  // namespace log
}  // namespace fetch
