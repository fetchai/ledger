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

#include "core/byte_array/byte_array.hpp"
#include "network/muddle/packet.hpp"

#include <cstring>  // memcpy

namespace fetch {
namespace muddle {

using byte_array::ByteArray;

/**
 * Convert the packet to a specified buffer
 *
 * @param packet The packet to convert to a binary stream
 * @param buffer The pointer to the output buffer to populate
 * @param length The length of the output buffer
 * @return true if successful, otherwise false
 */
bool Packet::ToBuffer(Packet const &packet, void *buffer, std::size_t length)
{
  bool success{false};

  auto *raw = reinterpret_cast<uint8_t *>(buffer);
  if (length >= packet.GetPacketSize())
  {
    std::size_t const payload_offset   = sizeof(packet.header_);
    std::size_t const signature_offset = payload_offset + packet.payload_.size();

    // copy header and the payload
    std::memcpy(raw, &packet.header_, sizeof(packet.header_));
    std::memcpy(raw + payload_offset, packet.payload_.pointer(), packet.payload_.size());

    // optionally add the signature if present
    if (packet.IsStamped())
    {
      std::memcpy(raw + signature_offset, packet.stamp_.pointer(), packet.stamp_.size());
    }

    success = true;
  }

  return success;
}

/**
 * Read in a packet from a specified packet buffer
 *
 * @param packet The packet to be populated
 * @param buffer The pointer to the input buffer
 * @param length THe length in bytes of the input buffer
 * @return true if successful, otherwise false
 */
bool Packet::FromBuffer(Packet &packet, void const *buffer, std::size_t length)
{
  auto const *raw = reinterpret_cast<uint8_t const *>(buffer);
  if (length < sizeof(packet.header_))
  {
    return false;
  }

  // read the header
  std::memcpy(&packet.header_, raw, sizeof(packet.header_));

  std::size_t payload_length = length - sizeof(packet.header_);
  if (packet.IsStamped())
  {
    if (payload_length < SIGNATURE_SIZE)
    {
      return false;
    }

    payload_length -= SIGNATURE_SIZE;
  }

  std::size_t const payload_offset = sizeof(packet.header_);

  // read in the payload
  ByteArray payload{};

  if (payload_length)
  {
    payload.Resize(payload_length);
    std::memcpy(payload.pointer(), raw + payload_offset, payload_length);
  }

  packet.payload_ = std::move(payload);

  // read in the signature
  if (packet.IsStamped())
  {
    std::size_t const signature_offset = sizeof(packet.header_) + payload_length;

    ByteArray signature{};
    signature.Resize(SIGNATURE_SIZE);
    std::memcpy(signature.pointer(), raw + signature_offset, SIGNATURE_SIZE);

    packet.stamp_ = std::move(signature);
  }

  return true;
}

}  // namespace muddle
}  // namespace fetch
