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

#include <string>
#include <utility>
#include <vector>

#include "logging/logging.hpp"
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

  explicit ProtoPathMessageSender(std::weak_ptr<EndpointType> endpoint)
  {
    this->endpoint = std::move(endpoint);
  }
  ~ProtoPathMessageSender() override = default;

  void SetEndianness(fetch::oef::base::Endianness /*newstate*/)
  {}

  consumed_needed_pair CheckForSpace(const mutable_buffers &data,
                                     IMessageWriter::TXQ &  txq) override;

protected:
private:
  Mutex                       mutex;
  std::weak_ptr<EndpointType> endpoint;

  ProtoPathMessageSender(const ProtoPathMessageSender &other) = delete;
  ProtoPathMessageSender &operator=(const ProtoPathMessageSender &other)  = delete;
  bool                    operator==(const ProtoPathMessageSender &other) = delete;
  bool                    operator<(const ProtoPathMessageSender &other)  = delete;
};
