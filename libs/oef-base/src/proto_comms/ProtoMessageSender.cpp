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
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include <google/protobuf/message.h>

ProtoMessageSender::consumed_needed_pair ProtoMessageSender::CheckForSpace(
    const mutable_buffers &data, IMessageWriter<TXType>::TXQ &txq)
{
  CharArrayBuffer chars(data);
  std::ostream    os(&chars);

  std::size_t consumed = 0;
  while (true)
  {
    {
      auto ep = endpoint.lock();
      if (ep == nullptr)
      {
        break;
      }
      if (!ep->IsTXQFull())
      {
        ep->wake();

        if (txq.empty())
        {
          break;
        }
      }
      Lock lock(mutex);

      auto     body_size = static_cast<uint32_t>(txq.front()->ByteSize());
      uint32_t head_size = sizeof(uint32_t);
      uint32_t mesg_size = body_size + head_size;
      if (chars.RemainingSpace() < mesg_size)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "out of space on write buffer.");
        break;
      }

      switch (endianness)
      {
      case fetch::oef::base::Endianness::DUNNO:
        throw std::invalid_argument("Refusing to send when endianness is DUNNO.");
        break;
      case fetch::oef::base::Endianness::LITTLE:
        // std::cout << "send little-endian size " << body_size << std::endl;
        chars.write_little_endian(body_size);
        break;
      case fetch::oef::base::Endianness::NETWORK:
        // std::cout << "send network-endian size" << body_size << std::endl;
        chars.write(body_size);
        break;
      case fetch::oef::base::Endianness::BAD:
        throw std::invalid_argument("Refusing to send when endianness is BAD.");
        break;
      }

      txq.front()->SerializeToOstream(&os);
      txq.pop_front();
      // std::cout << "Ready for sending! bytes=" << mesg_size << std::endl;
      // chars.diagnostic();

      consumed += mesg_size;
    }
  }
  return consumed_needed_pair(consumed, 0);
}
