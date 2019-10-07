#pragma once

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

  virtual ~EndpointPipe()
  {}

  virtual Notification::NotificationBuilder send(SendType s)
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

  virtual void go() override
  {
    endpoint->go();
  }

  virtual ISocketOwner::Socket &socket() override
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

  std::size_t GetIdentifier(void) const
  {
    return endpoint->GetIdentifier();
  }

  virtual const std::string &GetRemoteId(void) const
  {
    return endpoint->GetRemoteId();
  }

  std::shared_ptr<EndpointType> GetEndpoint()
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
