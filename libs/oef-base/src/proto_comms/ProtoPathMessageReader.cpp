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

#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/transport.hpp"

static Counter bytes_consumed_counter("mt-core.comms.protopath.read.bytes_consumed");
static Counter bytes_requested_counter("mt-core.comms.protopath.read.bytes_requested");
static Counter messages_handled_counter("mt-core.comms.protopath.read.messages_handled");

ProtoPathMessageReader::consumed_needed_pair ProtoPathMessageReader::initial()
{
  return consumed_needed_pair(0, 1);
}

ProtoPathMessageReader::consumed_needed_pair ProtoPathMessageReader::CheckForMessage(
    const buffers &data)
{
  FETCH_LOG_INFO(LOGGING_NAME, "CheckForMessage");

  std::string s;

  std::size_t consumed = 0;
  std::size_t needed   = 1;

  ConstCharArrayBuffer chars(data);
  std::istream         is(&chars);

  while (true)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "CheckForMessage in ", chars.RemainingData(),
                   " bytes. Current: ", chars.current, ", size:", chars.size);
    // chars.diagnostic();

    uint32_t leader_head_size  = sizeof(uint32_t);
    uint32_t payload_head_size = sizeof(uint32_t);

    uint32_t head_size = leader_head_size + payload_head_size;

    if (chars.RemainingData() < head_size)
    {
      needed = static_cast<std::size_t>(head_size - chars.RemainingData());
      break;
    }

    uint32_t leader_size_u32;
    uint32_t payload_size_u32;

    chars.read(leader_size_u32);
    chars.read(payload_size_u32);

    std::size_t leader_size  = leader_size_u32;
    std::size_t payload_size = payload_size_u32;

    std::size_t body_size = leader_size + payload_size;

    if (body_size > 10000)  // TODO(kll)
    {
      throw std::invalid_argument(std::string("Proto deserialisation refuses incoming ") +
                                  std::to_string(body_size) + " bytes message header.");
      break;
    }

    auto remaining_size = static_cast<std::size_t>(chars.RemainingData());
    if (remaining_size < body_size)
    {
      needed = static_cast<std::size_t>(body_size - remaining_size);
      break;
    }

    TransportHeader leader;

    auto header_chars =
        ConstCharArrayBuffer(chars, static_cast<uint32_t>(chars.current + leader_size));
    std::istream h_is(&header_chars);
    if (!leader.ParseFromIstream(&h_is))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse header!");
      throw std::invalid_argument("Proto deserialization refuses incoming invalid leader message!");
    }
    chars.advance(leader_size);

    consumed += head_size;
    consumed += body_size;

    bytes_consumed_counter += head_size;
    bytes_consumed_counter += body_size;

    if (!leader.status().success())
    {
      std::string msg;
      for (auto &n : leader.status().narrative())
      {
        msg += n;
      }
      int error_code = leader.status().error_code();
      if (error_code == 0)
      {
        error_code = 132;
      }
      FETCH_LOG_WARN(LOGGING_NAME, "Got error message from search: error_code=", error_code,
                     ", message=\"", msg, "\"");
      Counter("mt-core.comms.protopath.read.error_response")++;
      if (onError)
      {
        onError(leader.id(), error_code, msg);
      }
    }
    else
    {
      if (onComplete)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "+++++++++++++++++ onComplete.");
        messages_handled_counter++;
        Uri uri(leader.uri());
        if (!uri.valid && !leader.uri().empty())
        {
          uri.path = leader.uri();
        }
        onComplete(
            leader.status().success(), leader.id(), std::move(uri),
            ConstCharArrayBuffer(chars, static_cast<uint32_t>(chars.current + payload_size)));
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "+++++++++++++++++ No onComplete handler set.");
      }
    }

    chars.advance(payload_size);
  }
  bytes_requested_counter += needed;
  return consumed_needed_pair(consumed, needed);
}
