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

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/details/network_manager_implementation.hpp"

#include "network/fetch_asio.hpp"
#include <functional>
#include <map>

namespace fetch {
namespace network {

class NetworkManager
{
public:
  using event_function_type = std::function<void()>;

  using implementation_type = details::NetworkManagerImplementation;
  using pointer_type        = std::shared_ptr<implementation_type>;
  using weak_ref_type       = std::weak_ptr<implementation_type>;

  static constexpr char const *LOGGING_NAME = "NetworkManager";

  NetworkManager(std::string name, std::size_t threads)
    : pointer_{std::make_shared<implementation_type>(std::move(name), threads)}
  {}

  NetworkManager(NetworkManager const &other)
    : is_copy_(true)
  {
    if (other.is_copy_)
    {
      weak_pointer_ = other.weak_pointer_;
    }
    else
    {
      weak_pointer_ = other.pointer_;
    }
  }

  NetworkManager(NetworkManager &&rhs) = delete;
  NetworkManager &operator=(NetworkManager const &rhs) = delete;
  NetworkManager &operator=(NetworkManager &&rhs) = delete;

  void Start()
  {
    if (is_primary())
    {
      pointer_->Start();
    }
  }

  void Stop()
  {
    if (is_primary())
    {
      pointer_->Stop();
    }
  }

  template <typename F>
  void Post(F &&f)
  {
    auto ptr = lock();
    if (ptr)
    {
      return ptr->Post(f);
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Failed to post: network man dead.");
    }
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    auto ptr = lock();
    if (ptr)
    {
      return ptr->Post(f, milliseconds);
    }
  }

  bool is_valid()
  {
    return is_primary() || !weak_pointer_.expired();
  }

  bool Running()
  {
    auto ptr = lock();
    return ptr && ptr->Running();
  }

  bool is_primary()
  {
    return (!is_copy_);
  }

  pointer_type lock()
  {
    return is_primary() ? pointer_ : weak_pointer_.lock();
  }

  template <typename IO, typename... Args>
  std::shared_ptr<IO> CreateIO(Args &&... args)
  {
    auto ptr = lock();
    if (ptr)
    {
      return ptr->CreateIO<IO>(std::forward<Args>(args)...);
    }
    TODO_FAIL("Attempted to get IO from dead TM");
    return std::shared_ptr<IO>{};
  }

private:
  pointer_type  pointer_;
  weak_ref_type weak_pointer_;
  bool          is_copy_ = false;
};
}  // namespace network
}  // namespace fetch
