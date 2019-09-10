#pragma once

#include <vector>
#include <string>

#include "base/src/cpp/comms/IMessageWriter.hpp"
#include "base/src/cpp/comms/CharArrayBuffer.hpp"
#include "base/src/cpp/comms/Endianness.hpp"

#include "fetch_teams/ledger/logger.hpp"

template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoMessageReader;

class ProtoMessageSender
  : public IMessageWriter<std::shared_ptr<google::protobuf::Message>>
{
public:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<Mutex>;
  using consumed_needed_pair = IMessageWriter::consumed_needed_pair;
  using TXType = std::shared_ptr<google::protobuf::Message>;

  static constexpr char const *LOGGING_NAME = "ProtoMessageSender";

  ProtoMessageSender(std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> &endpoint)
  {
    this -> endpoint = endpoint;
  }
  virtual ~ProtoMessageSender()
  {
  }

  virtual consumed_needed_pair checkForSpace(const mutable_buffers &data, IMessageWriter<TXType>::TXQ& txq);
  void setEndianness(Endianness newstate) { endianness = newstate; }
protected:
private:
  Mutex mutex;
  Endianness endianness = DUNNO;
  std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> endpoint;

  ProtoMessageSender(const ProtoMessageSender &other) = delete;
  ProtoMessageSender &operator=(const ProtoMessageSender &other) = delete;
  bool operator==(const ProtoMessageSender &other) = delete;
  bool operator<(const ProtoMessageSender &other) = delete;
};
