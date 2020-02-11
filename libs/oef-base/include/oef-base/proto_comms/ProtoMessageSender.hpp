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

#include <memory>
#include <string>
#include <vector>

#include "oef-base/comms/CharArrayBuffer.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IMessageWriter.hpp"

#include "logging/logging.hpp"

template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class ProtoMessageReader;

class ProtoMessageSender : public IMessageWriter<std::shared_ptr<google::protobuf::Message>>
{
public:
  using Mutex                = std::mutex;
  using Lock                 = std::lock_guard<Mutex>;
  using consumed_needed_pair = IMessageWriter::consumed_needed_pair;
  using TXType               = std::shared_ptr<google::protobuf::Message>;

  static constexpr char const *LOGGING_NAME = "ProtoMessageSender";

  explicit ProtoMessageSender(
      std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> &endpoint)
  {
    this->endpoint = endpoint;
  }
  ~ProtoMessageSender() override = default;

  consumed_needed_pair CheckForSpace(const mutable_buffers &      data,
                                     IMessageWriter<TXType>::TXQ &txq) override;
  void                 SetEndianness(fetch::oef::base::Endianness newstate)
  {
    endianness = newstate;
  }

protected:
private:
  Mutex                        mutex;
  fetch::oef::base::Endianness endianness = fetch::oef::base::Endianness::DUNNO;
  std::weak_ptr<ProtoMessageEndpoint<TXType, ProtoMessageReader, ProtoMessageSender>> endpoint;

  ProtoMessageSender(const ProtoMessageSender &other) = delete;
  ProtoMessageSender &operator=(const ProtoMessageSender &other)  = delete;
  bool                operator==(const ProtoMessageSender &other) = delete;
  bool                operator<(const ProtoMessageSender &other)  = delete;
};
