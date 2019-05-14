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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "crypto/sha256.hpp"
#include "ledger/upow/synergetic_base_types.hpp"

#include <limits>

namespace fetch {
namespace consensus {

struct Work
{
  using Identity     = byte_array::ConstByteArray;
  using ContractName = byte_array::ConstByteArray;
  using WorkId       = byte_array::ConstByteArray;
  using Digest       = byte_array::ConstByteArray;
  using ScoreType    = SynergeticScoreType;
  using BigUnsigned  = math::BigUnsigned;

  enum
  {
    WORK_NOT_VALID = uint64_t(-1)
  };

  /// Serialisable
  /// @{
  uint64_t    block_number{WORK_NOT_VALID};
  BigUnsigned nonce{};
  ScoreType   score = std::numeric_limits<ScoreType>::max();
  /// }

  // Used internally after deserialisation
  // This information is already stored in the DAG and hence
  // we don't want to store it again.
  ContractName contract_name{};
  Identity     miner{};

  operator bool()
  {
    return block_number != WORK_NOT_VALID;
  }

  /*
   * @brief computes the hashed nonce for work execution.
   */
  BigUnsigned operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(contract_name);
    hasher.Update(miner);
    hasher.Update(nonce);

    Digest digest = hasher.Final();
    hasher.Reset();
    hasher.Update(digest);

    return hasher.Final();
  }

  /*
   * @brief work can be prioritised based on
   */
  bool operator<(Work const &other) const
  {
    return score < other.score;
  }

  bool operator==(Work const &other) const
  {
    return (block_number == other.block_number) && (score == other.score) &&
           (nonce == other.nonce) && (contract_name == other.contract_name) &&
           (miner == other.miner);
  }

  bool operator!=(Work const &other) const
  {
    return !(*this == other);
  }
};

template <typename T>
void Serialize(T &serializer, Work const &work)
{
  serializer << work.block_number << work.nonce << work.score;
}

template <typename T>
void Deserialize(T &serializer, Work &work)
{
  serializer >> work.block_number >> work.nonce >> work.score;
}

}  // namespace consensus
}  // namespace fetch