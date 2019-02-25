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

#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {

class ExecutorInterface
{
public:
  using TxDigest  = Transaction::TxDigest;
  using LaneIndex = uint32_t;
  using LaneSet   = std::unordered_set<LaneIndex>;

  enum class Status
  {
    SUCCESS = 0,

    // Errors
    NOT_RUN,
    TX_LOOKUP_FAILURE,
    RESOURCE_FAILURE,
    INEXPLICABLE_FAILURE,
    CHAIN_CODE_LOOKUP_FAILURE,
    CHAIN_CODE_EXEC_FAILURE
  };

  /// @name Executor Interface
  /// @{
  virtual Status Execute(TxDigest const &hash, std::size_t slice, LaneSet const &lanes) = 0;
  /// @}

  virtual ~ExecutorInterface() = default;
};

template <typename T>
void Serialize(T &stream, ExecutorInterface::Status const &status)
{
  stream << static_cast<int>(status);
}

template <typename T>
void Deserialize(T &stream, ExecutorInterface::Status &status)
{
  int raw_status{0};
  stream >> raw_status;
  status = static_cast<ExecutorInterface::Status>(raw_status);
}

}  // namespace ledger
}  // namespace fetch
