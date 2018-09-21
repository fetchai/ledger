#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/fetch_asio.hpp"

#include <functional>
#include <map>
#include <memory>

namespace fetch {
namespace network {
namespace details {

class NetworkManagerImplementation
  : public std::enable_shared_from_this<NetworkManagerImplementation>
{
public:
  static constexpr char const *LOGGING_NAME = "NetworkManagerImpl";

  NetworkManagerImplementation(std::size_t threads = 1)
    : number_of_threads_(threads)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Creating network manager");
  }

  ~NetworkManagerImplementation()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Destroying network manager");
    Stop();
  }

  NetworkManagerImplementation(NetworkManagerImplementation const &) = delete;
  NetworkManagerImplementation(NetworkManagerImplementation &&)      = default;

  void Start()
  {
    std::lock_guard<std::mutex> lock(thread_mutex_);

    if (threads_.size() == 0)
    {
      owning_thread_ = std::this_thread::get_id();
      shared_work_   = std::make_shared<asio::io_service::work>(*io_service_);

      for (std::size_t i = 0; i < number_of_threads_; ++i)
      {
        threads_.push_back(std::make_shared<std::thread>([this]() { this->Work(); }));
      }
    }
  }

  void Work()
  {
    io_service_->run();
  }

  void Stop()
  {
    std::lock_guard<std::mutex> lock(thread_mutex_);
    if (std::this_thread::get_id() != owning_thread_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Same thread must start and stop NetworkManager.");
      return;
    }

    if (threads_.size() != 0)
    {
      shared_work_.reset();
      FETCH_LOG_INFO(LOGGING_NAME, "Stopping network manager");

      io_service_->stop();

      for (auto &thread : threads_)
      {
        thread->join();
      }

      threads_.clear();
      io_service_ = std::make_unique<asio::io_service>();
    }
  }

  // Must only be called within a post, then the io_service_ is always
  // guaranteed to be valid
  template <typename IO, typename... arguments>
  std::shared_ptr<IO> CreateIO(arguments &&... args)
  {
    return std::make_shared<IO>(*io_service_, std::forward<arguments>(args)...);
  }

  template <typename F>
  void Post(F &&f)
  {
    io_service_->post(std::forward<F>(f));
  }

private:
  std::thread::id                   owning_thread_;
  std::size_t                       number_of_threads_ = 1;
  std::vector<std::shared_ptr<std::thread>>        threads_;
  std::unique_ptr<asio::io_service> io_service_ = std::make_unique<asio::io_service>();

  std::shared_ptr<asio::io_service::work> shared_work_;

  mutable std::mutex thread_mutex_{};
};

}  // namespace details
}  // namespace network
}  // namespace fetch
