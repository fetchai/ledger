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

#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/threading/Notification.hpp"
#include <memory>

template <typename EndpointType>
class EndpointPipe : public ISocketOwner
{
public:
  using message_type = typename EndpointType::message_type;
  using SendType     = typename EndpointType::message_type;

  explicit EndpointPipe(std::shared_ptr<EndpointType> endpoint)
    : endpoint(std::move(endpoint))
  {}

  ~EndpointPipe() override = default;

  virtual fetch::oef::base::Notification::NotificationBuilder send(SendType s)
  {
    return endpoint->send(s);
  }

  virtual bool connect(const Uri &uri, Core &core)
  {
    return endpoint->connect(uri, core);
  }

  virtual bool connected()
  {
    return endpoint->connected();
  }

  void go() override
  {
    endpoint->go();
  }

  ISocketOwner::Socket &socket() override
  {
    return endpoint->socket();
  }

  virtual void run_sending()
  {
    endpoint->run_sending();
  }

  virtual bool IsTXQFull()
  {
    return endpoint->IsTXQFull();
  }

  virtual void wake()
  {
    endpoint->wake();
  }

  std::size_t GetIdentifier() const
  {
    return endpoint->GetIdentifier();
  }

  virtual const std::string &GetRemoteId() const
  {
    return endpoint->GetRemoteId();
  }

  virtual const Uri &GetAddress() const
  {
    return endpoint->GetAddress();
  }

  virtual std::shared_ptr<EndpointType> GetEndpoint()
  {
    return endpoint;
  }

protected:
  std::shared_ptr<EndpointType> endpoint;

private:
  EndpointPipe(const EndpointPipe &other) = delete;
  EndpointPipe &operator=(const EndpointPipe &other)  = delete;
  bool          operator==(const EndpointPipe &other) = delete;
  bool          operator<(const EndpointPipe &other)  = delete;
};
