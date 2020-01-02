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

#include "chain/address.hpp"
#include "crypto/identity.hpp"

#include <type_traits>

/**
 * Generate a random address based on a specified RNG
 *
 * @tparam RNG The RNG type
 * @param rng The reference to the RNG to be used
 * @return The generated address
 */
template <typename RNG>
fetch::chain::Address GenerateRandomAddress(RNG &&rng)
{
  using Address = fetch::chain::Address;
  using RngWord = typename std::decay_t<RNG>::RandomType;

  static constexpr std::size_t ADDRESS_WORD_SIZE = Address::RAW_LENGTH / sizeof(RngWord);
  static_assert((Address::RAW_LENGTH % sizeof(RngWord)) == 0, "");

  Address::RawAddress raw_address{};
  auto *              raw = reinterpret_cast<RngWord *>(raw_address.data());

  for (std::size_t i = 0; i < ADDRESS_WORD_SIZE; ++i)
  {
    raw[i] = rng();
  }

  return Address{raw_address};
}

/**
 * Generate a random identity based on a specified RNG
 *
 * @tparam RNG The RNG type
 * @param rng The reference to the RNG to be used
 * @return The generated identity
 */
template <typename RNG>
fetch::crypto::Identity GenerateRandomIdentity(RNG &&rng)
{
  static constexpr std::size_t IDENTITY_BYTES = 64;

  fetch::byte_array::ByteArray array{};
  array.Resize(IDENTITY_BYTES);

  for (std::size_t i = 0; i < IDENTITY_BYTES; ++i)
  {
    array[i] = uint8_t(rng());
  }

  return fetch::crypto::Identity{array};
}
