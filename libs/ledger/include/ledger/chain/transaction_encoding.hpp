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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace ledger {
namespace detail {

template <typename T>
meta::IfIsUnsignedInteger<T, uint64_t> ToU64(T value)
{
  return value;
}

template <typename T>
meta::IfIsSignedInteger<T, uint64_t> ToU64(T value)
{
  return static_cast<uint64_t>(std::abs(value));
}

template <typename T>
meta::IfIsInteger<T, fetch::byte_array::ConstByteArray> EncodeInteger(T value)
{
  using fetch::byte_array::ByteArray;

  bool const is_signed = meta::IsSignedInteger<T> && (value < 0);

  ByteArray encoded;
  if (!is_signed && (value <= T{0x7f}))
  {
    encoded.Resize(1);
    encoded[0] = static_cast<uint8_t>(value & 0x7f);
  }
  else
  {
    uint64_t const abs_value = ToU64(value);

    if (is_signed && (abs_value <= 0x1F))
    {
      encoded.Resize(1);
      encoded[0] = 0xE0 | static_cast<uint8_t>(abs_value);
    }
    else
    {
      // determine the corresponding number of bytes required to encode this value
      uint64_t const bits_to_encode  = 64ull - platform::CountLeadingZeroes64(abs_value);
      uint64_t const bytes_to_encode = (bits_to_encode + 7u) >> 3u;

      uint8_t log2_bytes_required{3};
      for (uint64_t limit = 4u; limit; limit >>= 1u)
      {
        if (bytes_to_encode <= limit)
        {
          --log2_bytes_required;
        }
      }

      // calculate the actual number of bytes that will be required
      std::size_t const bytes_required = 1u << log2_bytes_required;

      // resize the buffer and encode the value
      encoded.Resize(bytes_required + 1u);

      // write out the header
      encoded[0] = static_cast<uint8_t>((is_signed) ? 0xD0u : 0xC0u) |
                   static_cast<uint8_t>(log2_bytes_required & 0xFu);

      std::size_t i = 1;
      for (std::size_t index = bytes_required - 1u;; --index)
      {
        // configure the mask to
        uint64_t const offset = index << 3u;
        uint64_t const mask   = 0xFFull << offset;

        // mask of the byte we are interested and store into place
        encoded[i++] = static_cast<uint8_t>((abs_value & mask) >> offset);

        // exit the loop once we have finished the zero'th index
        if (index == 0)
        {
          break;
        }
      }
    }
  }

  return {encoded};
}

template <typename T>
meta::IfIsUnsignedInteger<T, T> Negate(T value)
{
  return value;
}

template <typename T>
meta::IfIsSignedInteger<T, T> Negate(T value)
{
  return static_cast<T>(-value);
}

template <typename T>
meta::IfIsInteger<T, T> DecodeInteger(fetch::serializers::ByteArrayBuffer &buffer)
{
  // determine the traits of the output type
  constexpr bool        output_is_signed   = meta::IsSignedInteger<T>;
  constexpr std::size_t output_length      = sizeof(T);
  constexpr std::size_t output_log2_length = meta::Log2(output_length);

  T output_value{0};

  uint8_t initial_byte{0};
  buffer.ReadBytes(&initial_byte, 1);

  if ((initial_byte & 0x80u) == 0)
  {
    output_value = static_cast<T>(initial_byte & 0x7Fu);
  }
  else
  {
    uint8_t const type = (initial_byte >> 5u) & 0x3u;

    if (type == 3u)
    {
      // ensure the output type is of the correct type
      if (!output_is_signed)
      {
        throw std::runtime_error("Unable to extract signed value into unsigned value");
      }

      output_value = Negate(static_cast<T>(initial_byte & 0x1fu));
    }
    else
    {
      uint8_t const signed_flag       = (initial_byte >> 4u) & 0x1u;
      uint8_t const log2_value_length = (initial_byte & 0xfu);

      // size checks
      bool const output_is_too_small = (log2_value_length > output_log2_length);

      // in the case where U64::Max is (potentially) encoded but the output format is I64
      bool const unsigned_value_too_large =
          (!signed_flag) && output_is_signed && (log2_value_length == output_log2_length);

      // ensure that the output is of the correct size for this value
      if (output_is_too_small || unsigned_value_too_large)
      {
        throw std::runtime_error("Output is not large enough to extract the encoded value");
      }
      else if (signed_flag && !output_is_signed)
      {
        throw std::runtime_error("Unable to extract signed value into unsigned value");
      }

      uint64_t partial_value{0};
      for (std::size_t index = (1u << log2_value_length) - 1u;; --index)
      {
        // read the next byte
        uint8_t encoded_byte{0};
        buffer.ReadBytes(&encoded_byte, 1);

        // build up the partial value
        partial_value |= static_cast<uint64_t>(encoded_byte) << (index * 8u);

        // exit the loop once we have finished
        if (index == 0)
        {
          break;
        }
      }

      output_value = static_cast<T>(partial_value);

      if (output_is_signed && signed_flag)
      {
        output_value = Negate(output_value);
      }
    }
  }

  return output_value;
}

}  // namespace detail
}  // namespace ledger
}  // namespace fetch
