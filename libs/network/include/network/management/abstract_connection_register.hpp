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

#include "core/mutex.hpp"

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
  using connection_handle_type     = uint64_t;
  using weak_service_client_type   = std::weak_ptr<service::ServiceClient>;
  using shared_service_client_type = std::shared_ptr<service::ServiceClient>;
  using service_map_type = std::unordered_map<connection_handle_type, weak_service_client_type>;

  static constexpr char const *LOGGING_NAME = "AbstractConnectionRegister";

  AbstractConnectionRegister()
    : number_of_services_{0}
  {}

  AbstractConnectionRegister(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister(AbstractConnectionRegister &&other)      = delete;
  AbstractConnectionRegister &operator=(AbstractConnectionRegister const &other) = delete;
  AbstractConnectionRegister &operator=(AbstractConnectionRegister &&other) = delete;

  virtual ~AbstractConnectionRegister() = default;

  virtual void Leave(connection_handle_type id)                 = 0;
  virtual void Enter(std::weak_ptr<AbstractConnection> const &) = 0;

  shared_service_client_type GetService(connection_handle_type const &i)
  {
    std::lock_guard<mutex::Mutex> lock(service_lock_);
    if (services_.find(i) == services_.end())
    {
      return nullptr;
    }
    return services_[i].lock();
  }

  // TODO(kll): Rename this to match ServiceClients below.
  void WithServices(std::function<void(service_map_type const &)> f) const
  {
    std::lock_guard<mutex::Mutex> lock(service_lock_);
    f(services_);
  }

  void VisitServiceClients(std::function<void(service_map_type::value_type const &)> f) const
  {
    std::list<service_map_type::value_type> keys;

    {
      std::lock_guard<mutex::Mutex> lock(service_lock_);
      for (auto &item : services_)
      {
        keys.push_back(item);
      }
    }

    for (auto &item : keys)
    {
      auto                          k = item.first;
      std::lock_guard<mutex::Mutex> lock(service_lock_);
      if (services_.find(k) != services_.end())
      {
        f(item);
      }
    }
  }

  void VisitServiceClients(
      std::function<void(connection_handle_type const &, shared_service_client_type)> f) const
  {
    FETCH_LOG_WARN(LOGGING_NAME, "About to visit ", services_.size(), " service clients");
    std::list<service_map_type::value_type> keys;

    {
      std::lock_guard<mutex::Mutex> lock(service_lock_);
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
        auto                          k = item.first;
        std::lock_guard<mutex::Mutex> lock(service_lock_);
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
  void RemoveService(connection_handle_type const &n)
  {
    --number_of_services_;
    std::lock_guard<mutex::Mutex> lock(service_lock_);
    auto                          it = services_.find(n);
    if (it != services_.end())
    {
      services_.erase(it);
    }
  }

  void AddService(connection_handle_type const &n, weak_service_client_type const &ptr)
  {
    ++number_of_services_;

    std::lock_guard<mutex::Mutex> lock(service_lock_);
    services_[n] = ptr;
  }

  template <typename T>
  void ActivateSelfManage(T ptr)
  {
    ptr->ActivateSelfManage();
  }

  template <typename T>
  void DeactivateSelfManage(T ptr)
  {
    ptr->ADativateSelfManage();
  }

private:
  mutable mutex::Mutex  service_lock_{__LINE__, __FILE__};
  service_map_type      services_;
  std::atomic<uint64_t> number_of_services_;
};

}  // namespace network
}  // namespace fetch
