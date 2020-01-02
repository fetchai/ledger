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

#include <utility>

#include "logging/logging.hpp"
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

  explicit ProtoPathMessageReader(std::weak_ptr<EndpointType> endpoint)
  {
    this->endpoint = std::move(endpoint);
  }
  ~ProtoPathMessageReader() override = default;

  void SetEndianness(fetch::oef::base::Endianness /*newstate*/)
  {}

  consumed_needed_pair initial() override;
  consumed_needed_pair CheckForMessage(const buffers &data) override;

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
