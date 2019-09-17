#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
      std::function<void(unsigned long id, int error_code, std::string const &message)>;
  using TXType = std::shared_ptr<google::protobuf::Message>;

  static constexpr char const *LOGGING_NAME = "ProtoMessageReader";

  /// @{
  ProtoMessageReader(
      std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> &endpoint)
    : endpoint(endpoint)
  {}
  ProtoMessageReader(ProtoMessageReader const &other) = delete;
  virtual ~ProtoMessageReader()                       = default;
  /// @}

  /// @{
  ProtoMessageReader &operator=(ProtoMessageReader const &other)  = delete;
  bool                operator==(ProtoMessageReader const &other) = delete;
  bool                operator<(ProtoMessageReader const &other)  = delete;

  /// @}

  consumed_needed_pair initial();
  consumed_needed_pair CheckForMessage(const buffers &data);

  // TODO: Should be private variable with forward functions
  CompleteNotification onComplete;
  ErrorNotification    onError;

  void SetEndianness(Endianness newstate);

private:
  std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> endpoint;

  void setDetectedEndianness(Endianness newstate);

  Endianness endianness = DUNNO;
};
