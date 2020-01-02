#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fetch {
namespace network {
namespace details {

class NetworkManagerImplementation
  : public std::enable_shared_from_this<NetworkManagerImplementation>
{
public:
  static constexpr char const *LOGGING_NAME = "NetworkManagerImpl";

  NetworkManagerImplementation(std::string name, std::size_t threads)
    : name_(std::move(name))
    , number_of_threads_(threads)
    , running_{false}
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Creating network manager");
  }

  ~NetworkManagerImplementation()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Destroying network manager");
    Stop();
  }

  NetworkManagerImplementation(NetworkManagerImplementation const &) = delete;
  NetworkManagerImplementation(NetworkManagerImplementation &&)      = delete;

  void Start();
  void Work();
  void Stop();
  bool Running();

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
    if (!running_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Note, posting to a closed network manager");
    }
    io_service_->post(std::forward<F>(f));
  }

private:
  std::string const                         name_;
  std::thread::id                           owning_thread_;
  std::size_t                               number_of_threads_ = 1;
  std::vector<std::shared_ptr<std::thread>> threads_;
  std::atomic<bool>                         running_;

  std::unique_ptr<asio::io_service> io_service_ = std::make_unique<asio::io_service>();

  std::shared_ptr<asio::io_service::work> shared_work_;

  mutable Mutex thread_mutex_;
};

}  // namespace details
}  // namespace network
}  // namespace fetch
