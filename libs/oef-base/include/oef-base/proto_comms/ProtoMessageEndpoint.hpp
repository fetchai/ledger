#pragma once

#include <memory>

#include "oef-base/comms/Endpoint.hpp"

class ProtoMessageReader;
class ProtoMessageSender;

#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/comms/EndpointPipe.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/threading/Notification.hpp"

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

template <typename TXType, typename Reader = ProtoMessageReader,
          typename Sender = ProtoMessageSender>
class ProtoMessageEndpoint : public EndpointPipe<EndpointBase<TXType>>
{
public:
  using EndpointType = EndpointBase<TXType>;
  using typename EndpointPipe<EndpointBase<TXType>>::message_type;
  using EndpointPipe<EndpointBase<TXType>>::endpoint;

  ProtoMessageEndpoint(std::shared_ptr<EndpointType> endpoint);

  virtual ~ProtoMessageEndpoint()
  {}

  void SetEndianness(Endianness newstate);

  void setup(std::shared_ptr<ProtoMessageEndpoint> &myself);

  void SetOnStartHandler(typename EndpointType::StartNotification handler)
  {
    endpoint->onStart = std::move(handler);
  }

  void SetOnCompleteHandler(typename Reader::CompleteNotification handler)
  {
    protoMessageReader->onComplete = std::move(handler);
  }

  void SetOnPeerErrorHandler(typename Reader::ErrorNotification handler)
  {
    protoMessageReader->onError = std::move(handler);
  }

  void setOnErrorHandler(typename EndpointType::ErrorNotification handler)
  {
    endpoint->onError = std::move(handler);
  }

  void SetOnEofHandler(typename EndpointType::EofNotification handler)
  {
    endpoint->onEof = std::move(handler);
  }

  void SetOnProtoErrorHandler(typename EndpointType::ProtoErrorNotification handler)
  {
    endpoint->onProtoError = std::move(handler);
  }

protected:
  std::shared_ptr<Reader> protoMessageReader;
  std::shared_ptr<Sender> protoMessageSender;

private:
  ProtoMessageEndpoint(const ProtoMessageEndpoint &other) = delete;
  ProtoMessageEndpoint &operator=(const ProtoMessageEndpoint &other)  = delete;
  bool                  operator==(const ProtoMessageEndpoint &other) = delete;
  bool                  operator<(const ProtoMessageEndpoint &other)  = delete;
};
