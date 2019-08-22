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

#include "core/logger.hpp"
#include "core/macros.hpp"
#include "core/mutex.hpp"

#include <atomic>
#include <csignal>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace fetch {

namespace {
constexpr std::size_t DEFAULT_TIMEOUT_MS = 3000;
}

ProductionMutex::ProductionMutex(int, std::string const &)
{}

DebugMutex::MutexTimeout::MutexTimeout(std::string filename, int const &line)
  : filename_(std::move(filename))
  , line_(line)
{
  LOG_STACK_TRACE_POINT;

  thread_ = std::thread([=]() {
    LOG_LAMBDA_STACK_TRACE_POINT;

    // define the point at which the deadline has been reached
    Timepoint const deadline = created_ + std::chrono::milliseconds(DEFAULT_TIMEOUT_MS);

    while (running_)
    {
      // exit waiting loop when the dead line has been reached
      if (Clock::now() >= deadline)
      {
        break;
      }

      // wait
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (running_)
    {
      this->Eval();
    }
  });
}

DebugMutex::MutexTimeout::~MutexTimeout()
{
  running_ = false;
  thread_.join();
}

void DebugMutex::MutexTimeout::Eval()
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOG_ERROR("DebugMutex", "The system will terminate, mutex timed out: ", filename_, " ",
                  line_);

  // Send a sigint to ourselves since we have a handler for this
  kill(0, SIGINT);
}

DebugMutex::DebugMutex(int line, std::string file)
  : line_(line)
  , file_(std::move(file))
{}

void DebugMutex::lock()
{
  LOG_STACK_TRACE_POINT;
  {
    FETCH_LOCK(lock_mutex_);
    locked_ = Clock::now();
  }

  std::mutex::lock();

  timeout_ = std::make_unique<MutexTimeout>(file_, line_);
  fetch::logger.RegisterLock(this);
  thread_id_ = std::this_thread::get_id();
}

void DebugMutex::unlock()
{
  LOG_STACK_TRACE_POINT;

  double total_time = 0;

  {
    FETCH_LOCK(lock_mutex_);
    Timepoint const end_time   = Clock::now();
    Duration const  delta_time = end_time - locked_;
    total_time                 = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(delta_time).count());
  }

  timeout_.reset(nullptr);
  fetch::logger.RegisterUnlock(this, total_time, file_, line_);

  std::mutex::unlock();
}

int DebugMutex::line() const
{
  return line_;
}

std::string DebugMutex::filename() const
{
  return file_;
}

std::string DebugMutex::AsString()
{
  std::stringstream ss;
  ss << "Locked by thread #" << fetch::log::ReadableThread::GetThreadID(thread_id_) << " in "
     << filename() << " on " << line();
  return ss.str();
}

std::thread::id DebugMutex::thread_id() const
{
  return thread_id_;
}

}  // namespace fetch
