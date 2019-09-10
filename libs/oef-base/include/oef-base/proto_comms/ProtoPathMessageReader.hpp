#pragma once

#include "core/logging.hpp"
#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-base/utils/Uri.hpp"

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoPathMessageSender;

class ProtoPathMessageReader : public IMessageReader

{
public:
  using CompleteNotification =
      std::function<void(bool success, unsigned long id, Uri, ConstCharArrayBuffer buffer)>;
  using ErrorNotification =
      std::function<void(unsigned long id, int error_code, const std::string &message)>;
  using TXType       = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using EndpointType = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;

  static constexpr char const *LOGGING_NAME = "ProtoPathMessageReader";

  ProtoPathMessageReader(std::weak_ptr<EndpointType> endpoint)
  {
    this->endpoint = endpoint;
  }
  virtual ~ProtoPathMessageReader()
  {}

  void setEndianness(Endianness newstate)
  {}

  consumed_needed_pair initial();
  consumed_needed_pair checkForMessage(const buffers &data);

  CompleteNotification onComplete;
  ErrorNotification    onError;

protected:
private:
  std::weak_ptr<EndpointType> endpoint;

  ProtoPathMessageReader(const ProtoPathMessageReader &other) = delete;
  ProtoPathMessageReader &operator=(const ProtoPathMessageReader &other)  = delete;
  bool                    operator==(const ProtoPathMessageReader &other) = delete;
  bool                    operator<(const ProtoPathMessageReader &other)  = delete;
};
