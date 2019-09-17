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

#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"

ProtoMessageReader::consumed_needed_pair ProtoMessageReader::initial()
{
  return consumed_needed_pair(0, 1);
}

ProtoMessageReader::consumed_needed_pair ProtoMessageReader::checkForMessage(const buffers &data)
{
  // std::cout << "ProtoMessageReader::checkForMessage" << std::endl;

  std::string s;

  std::size_t consumed = 0;
  std::size_t needed   = 1;

  ConstCharArrayBuffer chars(data);
  std::istream         is(&chars);

  while (true)
  {
    // std::cout << "checkForMessage in " << chars.remainingData() << " bytes." << std::endl;
    // chars.diagnostic();

    uint32_t    body_size_u32;
    std::size_t body_size;
    uint32_t    head_size = sizeof(uint32_t);

    if (chars.remainingData() < head_size)
    {
      needed =
          static_cast<std::size_t>(head_size) - static_cast<std::size_t>(chars.remainingData());
      break;
    }

    switch (endianness)
    {
    case BAD:
      throw std::invalid_argument("Read attempted while endianness determined to be BAD.");
    case DUNNO:
      chars.read_little_endian(body_size_u32);
      if (body_size_u32 < 65535)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "detected LITTLE ENDIAN CONNECTION");
        setDetectedEndianness(LITTLE);
      }
      else
      {
        chars.read(body_size_u32);
        if (body_size_u32 < 65535)
        {
          FETCH_LOG_INFO(LOGGING_NAME, "detected NETWORK/BIG ENDIAN CONNECTION");
          setDetectedEndianness(NETWORK);
        }
        else
        {
          throw std::invalid_argument("Could not determine endianness.");
        }
      }
      break;
    case LITTLE:
      chars.read_little_endian(body_size_u32);
      break;
    case NETWORK:
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

    if (static_cast<std::size_t>(chars.remainingData()) < static_cast<std::size_t>(body_size))
    {
      needed =
          static_cast<std::size_t>(body_size) - static_cast<std::size_t>(chars.remainingData());
      break;
    }

    consumed += head_size;
    consumed += body_size;

    if (onComplete)
    {
      // std::cout << "MESSAGE = " << head_size << "+" << body_size << " bytes" << std::endl;
      onComplete(ConstCharArrayBuffer(
          chars, static_cast<std::size_t>(chars.current) + static_cast<std::size_t>(body_size)));
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No onComplete handler set.");
    }
    chars.advance(static_cast<int32_t>(body_size));
  }
  return consumed_needed_pair(consumed, needed);
}

void ProtoMessageReader::setDetectedEndianness(Endianness newstate)
{
  if (auto endpoint_sp = endpoint.lock())
  {
    endpoint_sp->setEndianness(newstate);
  }
}

void ProtoMessageReader::setEndianness(Endianness newstate)
{
  endianness = newstate;
}
