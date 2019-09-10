#pragma once

#include <string>
#include <vector>

#include "core/logging.hpp"
#include "oef-base/comms/CharArrayBuffer.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IMessageWriter.hpp"
#include "oef-base/threading/Notification.hpp"
#include "oef-base/threading/Waitable.hpp"

#include "oef-base/comms/EndpointBase.hpp"

#include <list>

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

class Uri;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoPathMessageReader;

class ProtoPathMessageSender
  : public IMessageWriter<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>
{
public:
  using Mutex                = std::mutex;
  using Lock                 = std::lock_guard<Mutex>;
  using TXType               = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using consumed_needed_pair = IMessageWriter::consumed_needed_pair;
  using EndpointType = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;

  static constexpr char const *LOGGING_NAME = "ProtoPathMessageSender";

  ProtoPathMessageSender(std::weak_ptr<EndpointType> endpoint)
  {
    this->endpoint = endpoint;
  }
  virtual ~ProtoPathMessageSender()
  {}

  void setEndianness(Endianness newstate)
  {}

  virtual consumed_needed_pair checkForSpace(const mutable_buffers &data, IMessageWriter::TXQ &txq);

protected:
private:
  Mutex                       mutex;
  std::weak_ptr<EndpointType> endpoint;

  ProtoPathMessageSender(const ProtoPathMessageSender &other) = delete;
  ProtoPathMessageSender &operator=(const ProtoPathMessageSender &other)  = delete;
  bool                    operator==(const ProtoPathMessageSender &other) = delete;
  bool                    operator<(const ProtoPathMessageSender &other)  = delete;
};
