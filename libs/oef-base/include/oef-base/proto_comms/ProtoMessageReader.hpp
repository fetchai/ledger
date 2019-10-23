#pragma once

#include "logging/logging.hpp"
#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-messages/fetch_protobuf.hpp"


template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoMessageSender;

class ProtoMessageReader : public IMessageReader

{
public:
  using CompleteNotification = std::function<void(ConstCharArrayBuffer buffer)>;
  using ErrorNotification =
      std::function<void(unsigned long id, int error_code, const std::string &message)>;
  using TXType = std::shared_ptr<google::protobuf::Message>;

  static constexpr char const *LOGGING_NAME = "ProtoMessageReader";

  ProtoMessageReader(
      std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> &endpoint)
    : endpoint(endpoint)
  {}
  virtual ~ProtoMessageReader()
  {}

  consumed_needed_pair initial();
  consumed_needed_pair CheckForMessage(const buffers &data);

  CompleteNotification onComplete;
  ErrorNotification    onError;

  void SetEndianness(Endianness newstate);

protected:
private:
  std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> endpoint;

  void setDetectedEndianness(Endianness newstate);

  Endianness endianness = DUNNO;

  ProtoMessageReader(const ProtoMessageReader &other) = delete;
  ProtoMessageReader &operator=(const ProtoMessageReader &other)  = delete;
  bool                operator==(const ProtoMessageReader &other) = delete;
  bool                operator<(const ProtoMessageReader &other)  = delete;
};
