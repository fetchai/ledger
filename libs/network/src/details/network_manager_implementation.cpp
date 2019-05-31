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

#include "network/details/network_manager_implementation.hpp"

#include "core/threading.hpp"

namespace fetch {
namespace network {
namespace details {

void NetworkManagerImplementation::Start()
{
  FETCH_LOCK(thread_mutex_);
  running_ = true;

  if (threads_.size() == 0)
  {
    owning_thread_ = std::this_thread::get_id();
    shared_work_   = std::make_shared<asio::io_service::work>(*io_service_);

    for (std::size_t i = 0; i < number_of_threads_; ++i)
    {
      auto thread = std::make_shared<std::thread>([this, i]() {
        SetThreadName(name_, i);

        this->Work();
      });
      threads_.push_back(thread);
    }
  }
}

void NetworkManagerImplementation::Work()
{
  io_service_->run();
}

void NetworkManagerImplementation::Stop()
{
  std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);
  running_ = false;

  if (threads_.empty())
  {
    return;
  }

  for (auto &thread : threads_)
  {
    if (std::this_thread::get_id() == thread->get_id())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Thread pools must not be killed by a thread they own.");
      return;
    }
  }

  shared_work_.reset();
  io_service_->stop();

  // Allow a period of time for any pending thread to finish
  // starting. It doesn't need to be long, just basically us
  // yielding our slice is enough.
  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  for (auto &thread : threads_)
  {
    thread->join();
  }

  threads_.clear();
  io_service_ = std::make_unique<asio::io_service>();
}

bool NetworkManagerImplementation::Running()
{
  return running_;
}

}  // namespace details
}  // namespace network
}  // namespace fetch
