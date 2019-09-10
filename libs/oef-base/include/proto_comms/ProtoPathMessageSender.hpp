#pragma once

#include <vector>
#include <string>

#include "base/src/cpp/comms/IMessageWriter.hpp"
#include "base/src/cpp/comms/CharArrayBuffer.hpp"
#include "base/src/cpp/comms/Endianness.hpp"
#include "base/src/cpp/threading/Waitable.hpp"
#include "base/src/cpp/threading/Notification.hpp"
#include "fetch_teams/ledger/logger.hpp"

#include "base/src/cpp/comms/EndpointBase.hpp"

#include <list>

namespace google
{
  namespace protobuf
  {
    class Message;
  }
}

class Uri;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoPathMessageReader;

class ProtoPathMessageSender
  : public IMessageWriter <std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;
  using TXType               = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
  using consumed_needed_pair = IMessageWriter::consumed_needed_pair;
  using EndpointType         = ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>;

  static constexpr char const *LOGGING_NAME = "ProtoPathMessageSender";

  ProtoPathMessageSender(std::weak_ptr<EndpointType> endpoint)
  {
    this -> endpoint = endpoint;
  }
  virtual ~ProtoPathMessageSender()
  {
  }

  void setEndianness(Endianness newstate)
  {
  }

  virtual consumed_needed_pair checkForSpace(const mutable_buffers &data, IMessageWriter::TXQ& txq);
protected:
private:
  Mutex mutex;
  std::weak_ptr<EndpointType> endpoint;


  ProtoPathMessageSender(const ProtoPathMessageSender &other) = delete;
  ProtoPathMessageSender &operator=(const ProtoPathMessageSender &other) = delete;
  bool operator==(const ProtoPathMessageSender &other) = delete;
  bool operator<(const ProtoPathMessageSender &other) = delete;
};
