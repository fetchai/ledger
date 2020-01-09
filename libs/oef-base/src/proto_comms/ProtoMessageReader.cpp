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

#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"

ProtoMessageReader::consumed_needed_pair ProtoMessageReader::initial()
{
  return consumed_needed_pair(0, 1);
}

ProtoMessageReader::consumed_needed_pair ProtoMessageReader::CheckForMessage(const buffers &data)
{
  // std::cout << "ProtoMessageReader::CheckForMessage" << std::endl;

  std::string s;

  uint32_t consumed = 0;
  uint32_t needed   = 1;

  ConstCharArrayBuffer chars(data);
  std::istream         is(&chars);

  while (true)
  {
    // std::cout << "CheckForMessage in " << chars.RemainingData() << " bytes." << std::endl;
    // chars.diagnostic();

    uint32_t body_size_u32 = 0;
    uint32_t body_size;
    uint32_t head_size = sizeof(uint32_t);

    if (chars.RemainingData() < head_size)
    {
      needed = static_cast<uint32_t>(head_size - chars.RemainingData());
      break;
    }

    switch (endianness)
    {
    case fetch::oef::base::Endianness::BAD:
      throw std::invalid_argument("Read attempted while endianness determined to be BAD.");
    case fetch::oef::base::Endianness::DUNNO:
      chars.read_little_endian(body_size_u32);
      if (body_size_u32 < 65535)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "detected LITTLE ENDIAN CONNECTION");
        setDetectedEndianness(fetch::oef::base::Endianness::LITTLE);
      }
      else
      {
        chars.read(body_size_u32);
        if (body_size_u32 < 65535)
        {
          FETCH_LOG_INFO(LOGGING_NAME, "detected NETWORK/BIG ENDIAN CONNECTION");
          setDetectedEndianness(fetch::oef::base::Endianness::NETWORK);
        }
        else
        {
          throw std::invalid_argument("Could not determine endianness.");
        }
      }
      break;
    case fetch::oef::base::Endianness::LITTLE:
      chars.read_little_endian(body_size_u32);
      break;
    case fetch::oef::base::Endianness::NETWORK:
      chars.read(body_size_u32);
      break;
    }

    body_size = body_size_u32;

    if (body_size > 10000)  // TODO(kll)
    {
      throw std::invalid_argument(std::string("Proto deserialisation refuses incoming ") +
                                  std::to_string(body_size) + " bytes message header.");
      break;
    }

    auto crmd = static_cast<uint32_t>(chars.RemainingData());

    if (crmd < body_size)
    {
      needed = body_size - crmd;
      break;
    }

    consumed += head_size;
    consumed += body_size;

    if (onComplete)
    {
      // std::cout << "MESSAGE = " << head_size << "+" << body_size << " bytes" << std::endl;
      onComplete(ConstCharArrayBuffer(chars, chars.current + body_size));
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No onComplete handler set.");
    }
    chars.advance(body_size);
  }
  return consumed_needed_pair(consumed, needed);
}

void ProtoMessageReader::setDetectedEndianness(fetch::oef::base::Endianness newstate)
{
  if (auto endpoint_sp = endpoint.lock())
  {
    endpoint_sp->SetEndianness(newstate);
  }
}

void ProtoMessageReader::SetEndianness(fetch::oef::base::Endianness newstate)
{
  endianness = newstate;
}
