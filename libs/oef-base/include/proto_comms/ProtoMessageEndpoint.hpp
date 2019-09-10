#pragma once

#include <memory>

#include "base/src/cpp/comms/Endpoint.hpp"

class ProtoMessageReader;
class ProtoMessageSender;

#include "base/src/cpp/comms/Endianness.hpp"
#include "base/src/cpp/comms/Endpoint.hpp"
#include "base/src/cpp/comms/EndpointPipe.hpp"
#include "base/src/cpp/proto_comms/ProtoMessageReader.hpp"
#include "base/src/cpp/proto_comms/ProtoPathMessageReader.hpp"
#include "base/src/cpp/threading/Notification.hpp"

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

  void setEndianness(Endianness newstate);

  void setup(std::shared_ptr<ProtoMessageEndpoint> &myself);

  void setOnStartHandler(typename EndpointType::StartNotification handler)
  {
    endpoint->onStart = std::move(handler);
  }

  void setOnCompleteHandler(typename Reader::CompleteNotification handler)
  {
    protoMessageReader->onComplete = std::move(handler);
  }

  void setOnPeerErrorHandler(typename Reader::ErrorNotification handler)
  {
    protoMessageReader->onError = std::move(handler);
  }

  void setOnErrorHandler(typename EndpointType::ErrorNotification handler)
  {
    endpoint->onError = std::move(handler);
  }

  void setOnEofHandler(typename EndpointType::EofNotification handler)
  {
    endpoint->onEof = std::move(handler);
  }

  void setOnProtoErrorHandler(typename EndpointType::ProtoErrorNotification handler)
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
