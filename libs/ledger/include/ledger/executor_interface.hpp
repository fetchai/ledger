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
  using TxDigest    = Transaction::TxDigest;
  using LaneIndex   = uint32_t;
  using LaneSet     = std::unordered_set<LaneIndex>;
  using TokenAmount = uint64_t;

  enum class Status
  {
    SUCCESS = 0,

    // Errors
    CHAIN_CODE_LOOKUP_FAILURE,
    CHAIN_CODE_EXEC_FAILURE,
    CONTRACT_NAME_PARSE_FAILURE,
    CONTRACT_LOOKUP_FAILURE,

    // Fatal Errors
    NOT_RUN,
    TX_LOOKUP_FAILURE,
    RESOURCE_FAILURE,
    INEXPLICABLE_FAILURE,
  };

  struct Result
  {
    Status      status;
    TokenAmount fee;
  };

  /// @name Executor Interface
  /// @{
  virtual Result Execute(TxDigest const &hash, LaneIndex log2_num_lanes, LaneSet lanes) = 0;
  /// @}

  virtual ~ExecutorInterface() = default;
};

inline char const *ToString(ExecutorInterface::Status status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case ExecutorInterface::Status::SUCCESS:
    text = "Success";
    break;
  case ExecutorInterface::Status::NOT_RUN:
    text = "Not Run";
    break;
  case ExecutorInterface::Status::TX_LOOKUP_FAILURE:
    text = "Tx Lookup Failure";
    break;
  case ExecutorInterface::Status::RESOURCE_FAILURE:
    text = "Resource Failure";
    break;
  case ExecutorInterface::Status::INEXPLICABLE_FAILURE:
    text = "Inexplicable Error";
    break;
  case ExecutorInterface::Status::CHAIN_CODE_LOOKUP_FAILURE:
    text = "Chain Code Lookup Failure";
    break;
  case ExecutorInterface::Status::CHAIN_CODE_EXEC_FAILURE:
    text = "Chain Code Execution Failure";
    break;
  case ExecutorInterface::Status::CONTRACT_NAME_PARSE_FAILURE:
    text = "Contract Name Parse Failure";
    break;
  case ExecutorInterface::Status::CONTRACT_LOOKUP_FAILURE:
    text = "Contract Lookup Failure";
    break;
  }

  return text;
}

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

template <typename T>
void Serialize(T &stream, ExecutorInterface::Result const &result)
{
  stream << result.status << result.fee;
}

template <typename T>
void Deserialize(T &stream, ExecutorInterface::Result &result)
{
  stream >> result.status >> result.fee;
}

}  // namespace ledger
}  // namespace fetch
