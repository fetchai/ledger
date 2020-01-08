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

#include "core/assert.hpp"
#include "core/mutex.hpp"
#include "logging/logging.hpp"
#include "network/details/network_manager_implementation.hpp"
#include "network/fetch_asio.hpp"

#include <functional>
#include <map>
#include <utility>

namespace fetch {
namespace network {

class NetworkManager
{
public:
  using EventFunctionType = std::function<void()>;

  using ImplementationType = details::NetworkManagerImplementation;
  using PointerType        = std::shared_ptr<ImplementationType>;
  using WeakRefType        = std::weak_ptr<ImplementationType>;

  static constexpr char const *LOGGING_NAME = "NetworkManager";

  NetworkManager(std::string name, std::size_t threads)
    : pointer_{std::make_shared<ImplementationType>(std::move(name), threads)}
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
      return ptr->Post(std::forward<F>(f));
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Failed to post: network man dead.");
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    auto ptr = lock();
    if (ptr)
    {
      return ptr->Post(std::forward<F>(f), milliseconds);
    }
  }

  bool is_valid()
  {
    return is_primary() || bool(weak_pointer_.lock());
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

  PointerType lock()
  {
    if (is_primary())
    {
      return pointer_;
    }
    return weak_pointer_.lock();
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
  PointerType pointer_;
  WeakRefType weak_pointer_;
  bool        is_copy_ = false;
};

}  // namespace network
}  // namespace fetch
