#pragma once

#include "base/src/cpp/comms/IMessageReader.hpp"
#include "base/src/cpp/comms/ConstCharArrayBuffer.hpp"
#include "base/src/cpp/comms/Endianness.hpp"
#include "fetch_teams/ledger/logger.hpp"

#include <google/protobuf/message.h>

template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoMessageSender;

class ProtoMessageReader : public IMessageReader

{
public:
  using CompleteNotification = std::function<void (ConstCharArrayBuffer buffer)>;
  using ErrorNotification    = std::function<void(unsigned long id, int error_code, const std::string& message)>;
  using TXType    = std::shared_ptr<google::protobuf::Message>;

  static constexpr char const *LOGGING_NAME = "ProtoMessageReader";


  ProtoMessageReader(std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> &endpoint)
    : endpoint(endpoint)
  {
  }
  virtual ~ProtoMessageReader()
  {
  }

  consumed_needed_pair initial();
  consumed_needed_pair checkForMessage(const buffers &data);

  CompleteNotification onComplete;
  ErrorNotification onError;

  void setEndianness(Endianness newstate);
protected:
private:
  std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> endpoint;

  void setDetectedEndianness(Endianness newstate);

  Endianness endianness = DUNNO;

  ProtoMessageReader(const ProtoMessageReader &other) = delete;
  ProtoMessageReader &operator=(const ProtoMessageReader &other) = delete;
  bool operator==(const ProtoMessageReader &other) = delete;
  bool operator<(const ProtoMessageReader &other) = delete;
};
