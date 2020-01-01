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

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

namespace fetch {
namespace service {
class ServiceClient;
}

namespace network {
class AbstractConnection;

class AbstractConnectionRegister : public std::enable_shared_from_this<AbstractConnectionRegister>
{
public:
  using ConnectionHandleType    = uint64_t;
  using WeakServiceClientType   = std::weak_ptr<service::ServiceClient>;
  using SharedServiceClientType = std::shared_ptr<service::ServiceClient>;
  using ServiceMapType          = std::unordered_map<ConnectionHandleType, WeakServiceClientType>;

  static constexpr char const *LOGGING_NAME = "AbstractConnectionRegister";

  AbstractConnectionRegister()
    : number_of_services_{0}
  {}

  AbstractConnectionRegister(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister(AbstractConnectionRegister &&other)      = delete;
  AbstractConnectionRegister &operator=(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister &operator=(AbstractConnectionRegister &&other) = delete;

  virtual ~AbstractConnectionRegister() = default;

  virtual void Leave(ConnectionHandleType id)                   = 0;
  virtual void Enter(std::weak_ptr<AbstractConnection> const &) = 0;

  SharedServiceClientType GetService(ConnectionHandleType const &i)
  {
    FETCH_LOCK(service_lock_);
    if (services_.find(i) == services_.end())
    {
      return nullptr;
    }
    return services_[i].lock();
  }

  // TODO(issue 1673): Rename this to match ServiceClients below.
  void WithServices(std::function<void(ServiceMapType const &)> const &f) const
  {
    FETCH_LOCK(service_lock_);
    f(services_);
  }

  void VisitServiceClients(std::function<void(ServiceMapType::value_type const &)> const &f) const
  {
    std::list<ServiceMapType ::value_type> keys;

    {
      FETCH_LOCK(service_lock_);
      for (auto &item : services_)
      {
        keys.push_back(item);
      }
    }

    for (auto &item : keys)
    {
      auto k = item.first;
      FETCH_LOCK(service_lock_);
      if (services_.find(k) != services_.end())
      {
        f(item);
      }
    }
  }

  void VisitServiceClients(
      std::function<void(ConnectionHandleType const &, SharedServiceClientType)> const &f) const
  {
    FETCH_LOG_WARN(LOGGING_NAME, "About to visit ", services_.size(), " service clients");
    std::list<ServiceMapType ::value_type> keys;

    {
      FETCH_LOCK(service_lock_);
      for (auto &item : services_)
      {
        keys.push_back(item);
      }
    }

    for (auto &item : keys)
    {
      auto v = item.second.lock();
      if (v)
      {
        auto k = item.first;
        FETCH_LOCK(service_lock_);
        if (services_.find(k) != services_.end())
        {
          f(k, v);
        }
      }
    }
  }

  uint64_t number_of_services() const
  {
    return number_of_services_;
  }

protected:
  void RemoveService(ConnectionHandleType const &n)
  {
    --number_of_services_;
    FETCH_LOCK(service_lock_);
    auto it = services_.find(n);
    if (it != services_.end())
    {
      services_.erase(it);
    }
  }

  void AddService(ConnectionHandleType const &n, WeakServiceClientType const &ptr)
  {
    ++number_of_services_;

    FETCH_LOCK(service_lock_);
    services_[n] = ptr;
  }

private:
  mutable Mutex         service_lock_;
  ServiceMapType        services_;
  std::atomic<uint64_t> number_of_services_;
};

}  // namespace network
}  // namespace fetch
