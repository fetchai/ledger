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
  using ConnectionHandleType    = typename AbstractConnection::ConnectionHandleType;
  using WeakConnectionType      = std::weak_ptr<AbstractConnection>;
  using SharedConnectionType    = std::shared_ptr<AbstractConnection>;
  using ServiceClientType       = service::ServiceClient;
  using SharedServiceClientType = std::shared_ptr<service::ServiceClient>;
  using WeakServiceClientType   = std::weak_ptr<service::ServiceClient>;
  using DetailsType             = G;

  static constexpr char const *LOGGING_NAME = "ConnectionRegisterImpl";

  struct LockableDetails final : public DetailsType, public Mutex
  {
    LockableDetails()
      : DetailsType()
      , Mutex{}
    {}
  };
  using DetailsMapType = std::unordered_map<ConnectionHandleType, std::shared_ptr<LockableDetails>>;
  using CallbackClientEnterType = std::function<void(ConnectionHandleType)>;
  using CallbackClientLeaveType = std::function<void(ConnectionHandleType)>;
  using ConnectionMapType       = std::unordered_map<ConnectionHandleType, WeakConnectionType>;

  ConnectionRegisterImpl()                                    = default;
  ConnectionRegisterImpl(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl(ConnectionRegisterImpl &&other)      = default;
  ConnectionRegisterImpl &operator=(ConnectionRegisterImpl const &other) = delete;
  ConnectionRegisterImpl &operator=(ConnectionRegisterImpl &&other) = default;

  virtual ~ConnectionRegisterImpl() = default;

  template <typename T, typename... Args>
  SharedServiceClientType CreateServiceClient(NetworkManager const &tm, Args &&... args)
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

    SharedServiceClientType service =
        std::make_shared<ServiceClientType>(connection.connection_pointer().lock(), tm);

    auto wptr = connection.connection_pointer();
    auto ptr  = wptr.lock();
    assert(ptr);

    Enter(wptr);
    ptr->SetConnectionManager(shared_from_this());

    {
      FETCH_LOCK(connections_lock_);
      AddService(ptr->handle(), service);
    }

    return service;
  }

  std::size_t size() const
  {
    FETCH_LOCK(connections_lock_);
    return connections_.size();
  }

  void OnClientEnter(CallbackClientEnterType const &f)
  {
    on_client_enter_ = f;
  }
  void OnClientLeave(CallbackClientEnterType const &f)
  {
    on_client_leave_ = f;
  }

  void Leave(ConnectionHandleType id) override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "ConnectionRegisterImpl::Leave");
    {
      RemoveService(id);

      FETCH_LOCK(connections_lock_);
      auto it = connections_.find(id);
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

  void Enter(WeakConnectionType const &wptr) override
  {
    ConnectionHandleType handle = ConnectionHandleType(-1);

    {
      auto ptr = wptr.lock();
      if (ptr)
      {
        FETCH_LOCK(connections_lock_);
        connections_[ptr->handle()] = ptr;
        details_[ptr->handle()]     = std::make_shared<LockableDetails>();
        handle                      = ptr->handle();
      }
    }

    if (handle != ConnectionHandleType(-1))
    {
      SignalClientEnter(handle);
    }
  }

  std::shared_ptr<LockableDetails> GetDetails(ConnectionHandleType const &i)
  {
    FETCH_LOCK(details_lock_);
    if (details_.find(i) == details_.end())
    {
      return nullptr;
    }

    return details_[i];
  }

  SharedConnectionType GetClient(ConnectionHandleType const &i)
  {
    FETCH_LOCK(connections_lock_);
    return connections_[i].lock();
  }

  void WithClientDetails(std::function<void(DetailsMapType const &)> fnc) const
  {
    FETCH_LOCK(details_lock_);
    fnc(details_);
  }

  void WithClientDetails(std::function<void(DetailsMapType &)> fnc)
  {
    FETCH_LOCK(details_lock_);
    fnc(details_);
  }

  void WithConnections(std::function<void(ConnectionMapType const &)> fnc)
  {
    FETCH_LOCK(connections_lock_);
    fnc(connections_);
  }

  void VisitConnections(std::function<void(ConnectionMapType::value_type const &)> f) const
  {
    std::list<ConnectionMapType::value_type> keys;
    {
      FETCH_LOCK(connections_lock_);
      for (auto &item : connections_)
      {
        keys.push_back(item);
      }
    }

    for (auto &item : keys)
    {
      auto k = item.first;
      FETCH_LOCK(connections_lock_);
      if (connections_.find(k) != connections_.end())
      {
        f(item);
      }
    }
  }

  void VisitConnections(
      std::function<void(ConnectionHandleType const &, SharedConnectionType)> f) const
  {
    FETCH_LOG_WARN(LOGGING_NAME, "About to visit ", connections_.size(), " connections");
    std::list<ConnectionMapType::value_type> keys;
    {
      FETCH_LOCK(connections_lock_);
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
        auto k = item.first;
        FETCH_LOCK(connections_lock_);
        if (connections_.find(k) != connections_.end())
        {
          f(k, v);
        }
      }
    }
  }

private:
  mutable Mutex     connections_lock_;
  ConnectionMapType connections_;

  mutable Mutex  details_lock_;
  DetailsMapType details_;

  void SignalClientLeave(ConnectionHandleType const &handle)
  {
    if (on_client_leave_)
    {
      on_client_leave_(handle);
    }
  }

  void SignalClientEnter(ConnectionHandleType const &handle)
  {
    if (on_client_enter_)
    {
      on_client_enter_(handle);
    }
  }

  generics::Callbacks<CallbackClientEnterType> on_client_leave_;
  generics::Callbacks<CallbackClientEnterType> on_client_enter_;
};

template <typename G>
class ConnectionRegister
{
public:
  using ConnectionHandleType            = typename AbstractConnection::ConnectionHandleType;
  using WeakConnectionType              = std::weak_ptr<AbstractConnection>;
  using SharedConnectionType            = std::shared_ptr<AbstractConnection>;
  using SharedImplementationPointerType = std::shared_ptr<ConnectionRegisterImpl<G>>;
  using LockableDetailsType             = typename ConnectionRegisterImpl<G>::LockableDetails;
  using SharedServiceClientType         = std::shared_ptr<service::ServiceClient>;
  using WeakServiceClientType           = std::weak_ptr<service::ServiceClient>;
  using ServiceMapType                  = AbstractConnectionRegister::ServiceMapType;
  using DetailsMapType                  = typename ConnectionRegisterImpl<G>::DetailsMapType;
  using CallbackClientEnterType         = std::function<void(ConnectionHandleType)>;
  using CallbackClientLeaveType         = std::function<void(ConnectionHandleType)>;
  using ConnectionMapType = std::unordered_map<ConnectionHandleType, WeakConnectionType>;
  ConnectionRegister()
  {
    ptr_ = std::make_shared<ConnectionRegisterImpl<G>>();
  }

  template <typename T, typename... Args>
  SharedServiceClientType CreateServiceClient(NetworkManager const &tm, Args &&... args)
  {
    return ptr_->template CreateServiceClient<T, Args...>(tm, std::forward<Args>(args)...);
  }

  std::size_t size() const
  {
    return ptr_->size();
  }

  void OnClientEnter(CallbackClientEnterType const &f)
  {
    return ptr_->OnClientEnter(f);
  }

  void OnClientLeave(CallbackClientEnterType const &f)
  {
    return ptr_->OnClientLeave(f);
  }

  std::shared_ptr<LockableDetailsType> GetDetails(ConnectionHandleType const &i)
  {
    return ptr_->GetDetails(i);
  }

  SharedServiceClientType GetService(ConnectionHandleType &&i)
  {
    return ptr_->GetService(std::move(i));
  }

  SharedServiceClientType GetService(ConnectionHandleType const &i)
  {
    return ptr_->GetService(i);
  }

  SharedConnectionType GetClient(ConnectionHandleType const &i)
  {
    return ptr_->GetClient(i);
  }

  void WithServices(std::function<void(ServiceMapType const &)> const &f) const
  {
    ptr_->WithServices(f);
  }

  void VisitServiceClients(
      std::function<void(ConnectionHandleType const &, SharedServiceClientType)> f) const
  {
    ptr_->VisitServiceClients(f);
  }

  void VisitServiceClients(std::function<void(ServiceMapType ::value_type const &)> f) const
  {
    ptr_->VisitServiceClients(f);
  }

  void VisitConnections(
      std::function<void(ConnectionHandleType const &, SharedConnectionType)> f) const
  {
    ptr_->VisitConnections(f);
  }

  void VisitConnections(std::function<void(ConnectionMapType::value_type const &)> f) const
  {
    ptr_->VisitConnections(f);
  }

  void WithClientDetails(std::function<void(DetailsMapType const &)> fnc) const
  {
    ptr_->WithClientDetails(fnc);
  }

  void WithClientDetails(std::function<void(DetailsMapType &)> fnc)
  {
    ptr_->WithClientDetails(fnc);
  }

  void WithConnections(std::function<void(ConnectionMapType const &)> fnc)
  {
    ptr_->WithConnections(fnc);
  }

  uint64_t number_of_services() const
  {
    return ptr_->number_of_services();
  }

  SharedImplementationPointerType pointer()
  {
    return ptr_;
  }

private:
  SharedImplementationPointerType ptr_;
};

}  // namespace network
}  // namespace fetch
