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
#include "network/generics/callbacks.hpp"
#include "network/management/abstract_connection_register.hpp"
#include "network/service/service_client.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <unordered_map>

namespace fetch {
namespace network {

template <typename G>
class ConnectionRegisterImpl final : public AbstractConnectionRegister
{
public:
  using connection_handle_type     = typename AbstractConnection::connection_handle_type;
  using weak_connection_type       = std::weak_ptr<AbstractConnection>;
  using shared_connection_type     = std::shared_ptr<AbstractConnection>;
  using service_client_type        = service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service::ServiceClient>;
  using weak_service_client_type   = std::weak_ptr<service::ServiceClient>;
  using details_type               = G;
  using mutex_type                 = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "ConnectionRegisterImpl";

  struct LockableDetails final : public details_type, public mutex_type
  {
    LockableDetails()
      : details_type()
      , mutex_type(__LINE__, __FILE__)
    {}
  };
  using details_map_type =
      std::unordered_map<connection_handle_type, std::shared_ptr<LockableDetails>>;
  using callback_client_enter_type = std::function<void(connection_handle_type)>;
  using callback_client_leave_type = std::function<void(connection_handle_type)>;
  using connection_map_type = std::unordered_map<connection_handle_type, weak_connection_type>;

  ConnectionRegisterImpl()                                    = default;
  ConnectionRegisterImpl(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl(ConnectionRegisterImpl &&other)      = default;
  ConnectionRegisterImpl &operator=(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl &operator=(ConnectionRegisterImpl &&other) = default;

  virtual ~ConnectionRegisterImpl() = default;

  template <typename T, typename... Args>
  shared_service_client_type CreateServiceClient(NetworkManager const &tm, Args &&... args)
  {
    using Clock     = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;

    T connection(tm);
    connection.Connect(std::forward<Args>(args)...);

    // wait for the connection to be established
    Timepoint const start     = Clock::now();
    Timepoint const threshold = start + std::chrono::seconds{10};
    while (!connection.is_alive())
    {
      // termination condition
      if (Clock::now() >= threshold)
      {
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    shared_service_client_type service =
        std::make_shared<service_client_type>(connection.connection_pointer().lock(), tm);

    auto wptr = connection.connection_pointer();
    auto ptr  = wptr.lock();
    assert(ptr);

    Enter(wptr);
    ptr->SetConnectionManager(shared_from_this());

    {
      std::lock_guard<mutex::Mutex> lock(connections_lock_);
      AddService(ptr->handle(), service);
    }

    return service;
  }

  std::size_t size() const
  {
    std::lock_guard<mutex::Mutex> lock(connections_lock_);
    return connections_.size();
  }

  void OnClientEnter(callback_client_enter_type const &f)
  {
    on_client_enter_ = f;
  }
  void OnClientLeave(callback_client_enter_type const &f)
  {
    on_client_leave_ = f;
  }

  void Leave(connection_handle_type id) override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "ConnectionRegisterImpl::Leave");
    {
      RemoveService(id);

      std::lock_guard<mutex::Mutex> lock(connections_lock_);
      auto                          it = connections_.find(id);
      if (it != connections_.end())
      {
        connections_.erase(it);
      }

      auto it2 = details_.find(id);
      if (it2 != details_.end())
      {
        details_.erase(it2);
      }
    }

    SignalClientLeave(id);
  }

  void Enter(weak_connection_type const &wptr) override
  {
    connection_handle_type handle = connection_handle_type(-1);

    {
      auto ptr = wptr.lock();
      if (ptr)
      {
        std::lock_guard<mutex::Mutex> lock(connections_lock_);
        connections_[ptr->handle()] = ptr;
        details_[ptr->handle()]     = std::make_shared<LockableDetails>();
        handle                      = ptr->handle();
      }
    }

    if (handle != connection_handle_type(-1))
    {
      SignalClientEnter(handle);
    }
  }

  std::shared_ptr<LockableDetails> GetDetails(connection_handle_type const &i)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<mutex::Mutex> lock(details_lock_);
    if (details_.find(i) == details_.end())
    {
      return nullptr;
    }

    return details_[i];
  }

  shared_connection_type GetClient(connection_handle_type const &i)
  {
    std::lock_guard<mutex::Mutex> lock(connections_lock_);
    return connections_[i].lock();
  }

  void WithClientDetails(std::function<void(details_map_type const &)> fnc) const
  {
    std::lock_guard<mutex::Mutex> lock(details_lock_);
    fnc(details_);
  }

  void WithClientDetails(std::function<void(details_map_type &)> fnc)
  {
    std::lock_guard<mutex::Mutex> lock(details_lock_);
    fnc(details_);
  }

  void WithConnections(std::function<void(connection_map_type const &)> fnc)
  {
    std::lock_guard<mutex::Mutex> lock(connections_lock_);
    fnc(connections_);
  }

  void VisitConnections(std::function<void(connection_map_type::value_type const &)> f) const
  {
    std::list<connection_map_type::value_type> keys;
    {
      std::lock_guard<mutex::Mutex> lock(connections_lock_);
      for (auto &item : connections_)
      {
        keys.push_back(item);
      }
    }

    for (auto &item : keys)
    {
      auto                          k = item.first;
      std::lock_guard<mutex::Mutex> lock(connections_lock_);
      if (connections_.find(k) != connections_.end())
      {
        f(item);
      }
    }
  }

  void VisitConnections(
      std::function<void(connection_handle_type const &, shared_connection_type)> f) const
  {
    FETCH_LOG_WARN(LOGGING_NAME, "About to visit ", connections_.size(), " connections");
    std::list<connection_map_type::value_type> keys;
    {
      std::lock_guard<mutex::Mutex> lock(connections_lock_);
      for (auto &item : connections_)
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
        std::lock_guard<mutex::Mutex> lock(connections_lock_);
        if (connections_.find(k) != connections_.end())
        {
          f(k, v);
        }
      }
    }
  }

private:
  mutable mutex::Mutex connections_lock_{__LINE__, __FILE__};
  connection_map_type  connections_;

  mutable mutex::Mutex details_lock_{__LINE__, __FILE__};
  details_map_type     details_;

  void SignalClientLeave(connection_handle_type const &handle)
  {
    if (on_client_leave_)
    {
      on_client_leave_(handle);
    }
  }

  void SignalClientEnter(connection_handle_type const &handle)
  {
    if (on_client_enter_)
    {
      on_client_enter_(handle);
    }
  }

  generics::Callbacks<callback_client_enter_type> on_client_leave_;
  generics::Callbacks<callback_client_enter_type> on_client_enter_;
};

template <typename G>
class ConnectionRegister
{
public:
  using connection_handle_type             = typename AbstractConnection::connection_handle_type;
  using weak_connection_type               = std::weak_ptr<AbstractConnection>;
  using shared_connection_type             = std::shared_ptr<AbstractConnection>;
  using shared_implementation_pointer_type = std::shared_ptr<ConnectionRegisterImpl<G>>;
  using lockable_details_type              = typename ConnectionRegisterImpl<G>::LockableDetails;
  using shared_service_client_type         = std::shared_ptr<service::ServiceClient>;
  using weak_service_client_type           = std::weak_ptr<service::ServiceClient>;
  using service_map_type                   = AbstractConnectionRegister::service_map_type;
  using details_map_type                   = typename ConnectionRegisterImpl<G>::details_map_type;
  using callback_client_enter_type         = std::function<void(connection_handle_type)>;
  using callback_client_leave_type         = std::function<void(connection_handle_type)>;
  using connection_map_type = std::unordered_map<connection_handle_type, weak_connection_type>;
  ConnectionRegister()
  {
    ptr_ = std::make_shared<ConnectionRegisterImpl<G>>();
  }

  template <typename T, typename... Args>
  shared_service_client_type CreateServiceClient(NetworkManager const &tm, Args &&... args)
  {
    return ptr_->template CreateServiceClient<T, Args...>(tm, std::forward<Args>(args)...);
  }

  std::size_t size() const
  {
    return ptr_->size();
  }

  void OnClientEnter(callback_client_enter_type const &f)
  {
    return ptr_->OnClientEnter(f);
  }

  void OnClientLeave(callback_client_enter_type const &f)
  {
    return ptr_->OnClientLeave(f);
  }

  std::shared_ptr<lockable_details_type> GetDetails(connection_handle_type const &i)
  {
    return ptr_->GetDetails(i);
  }

  shared_service_client_type GetService(connection_handle_type &&i)
  {
    return ptr_->GetService(std::move(i));
  }

  shared_service_client_type GetService(connection_handle_type const &i)
  {
    return ptr_->GetService(i);
  }

  shared_connection_type GetClient(connection_handle_type const &i)
  {
    return ptr_->GetClient(i);
  }

  void WithServices(std::function<void(service_map_type const &)> const &f) const
  {
    ptr_->WithServices(f);
  }

  void VisitServiceClients(
      std::function<void(connection_handle_type const &, shared_service_client_type)> f) const
  {
    ptr_->VisitServiceClients(f);
  }

  void VisitServiceClients(std::function<void(service_map_type::value_type const &)> f) const
  {
    ptr_->VisitServiceClients(f);
  }

  void VisitConnections(
      std::function<void(connection_handle_type const &, shared_connection_type)> f) const
  {
    ptr_->VisitConnections(f);
  }

  void VisitConnections(std::function<void(connection_map_type::value_type const &)> f) const
  {
    ptr_->VisitConnections(f);
  }

  void WithClientDetails(std::function<void(details_map_type const &)> fnc) const
  {
    ptr_->WithClientDetails(fnc);
  }

  void WithClientDetails(std::function<void(details_map_type &)> fnc)
  {
    ptr_->WithClientDetails(fnc);
  }

  void WithConnections(std::function<void(connection_map_type const &)> fnc)
  {
    ptr_->WithConnections(fnc);
  }

  uint64_t number_of_services() const
  {
    return ptr_->number_of_services();
  }

  shared_implementation_pointer_type pointer()
  {
    return ptr_;
  }

private:
  shared_implementation_pointer_type ptr_;
};

}  // namespace network
}  // namespace fetch
