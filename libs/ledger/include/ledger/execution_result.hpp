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

#include <cstdint>

namespace fetch {
namespace ledger {

using TokenAmount = uint64_t;

enum class ContractExecutionStatus
{
  SUCCESS = 0,

  // Errors
  CHAIN_CODE_LOOKUP_FAILURE,
  CHAIN_CODE_EXEC_FAILURE,
  CONTRACT_NAME_PARSE_FAILURE,
  CONTRACT_LOOKUP_FAILURE,
  TX_NOT_VALID_FOR_BLOCK,
  INSUFFICIENT_AVAILABLE_FUNDS,
  TRANSFER_FAILURE,
  INSUFFICIENT_CHARGE,

  // Fatal Errors
  NOT_RUN,  // TODO(issue ): Seems to be the same as `INEXPLICABLE_FAILURE`
  TX_LOOKUP_FAILURE,
  RESOURCE_FAILURE,
  INEXPLICABLE_FAILURE,
};

struct ContractExecutionResult
{
  ContractExecutionStatus status{
      ContractExecutionStatus::NOT_RUN};  ///< The status of the transaction
  TokenAmount charge{0};                  ///< The number of units of charge
  TokenAmount charge_rate{0};             ///< The cost of each unit of charge
  TokenAmount charge_limit{0};            ///< Max. limit for units to charge defined by Tx sender
  TokenAmount fee{0};                     ///< The total fee claimed by the miner
  int64_t     return_value{0};            ///< Return value from executed contract function
};

constexpr char const *ToString(ContractExecutionStatus status)
{
  switch (status)
  {
  case ContractExecutionStatus::SUCCESS:
    return "Success";
  case ContractExecutionStatus::CHAIN_CODE_LOOKUP_FAILURE:
    return "Chain Code Lookup Failure";
  case ContractExecutionStatus::CHAIN_CODE_EXEC_FAILURE:
    return "Chain Code Execution Failure";
  case ContractExecutionStatus::CONTRACT_NAME_PARSE_FAILURE:
    return "Contract Name Parse Failure";
  case ContractExecutionStatus::CONTRACT_LOOKUP_FAILURE:
    return "Contract Lookup Failure";
  case ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK:
    return "Tx not valid for current block";
  case ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS:
    return "Insufficient available funds";
  case ContractExecutionStatus::TRANSFER_FAILURE:
    return "Unable to perform transfer";
  case ContractExecutionStatus::INSUFFICIENT_CHARGE:
    return "Insufficient charge";
  case ContractExecutionStatus::NOT_RUN:
    return "Not Run";
  case ContractExecutionStatus::TX_LOOKUP_FAILURE:
    return "Tx Lookup Failure";
  case ContractExecutionStatus::RESOURCE_FAILURE:
    return "Resource Failure";
  case ContractExecutionStatus::INEXPLICABLE_FAILURE:
    return "Inexplicable Error";
  }

  return "Unknown";
}

template <typename T>
void Serialize(T &stream, ContractExecutionStatus const &status)
{
  stream << static_cast<int>(status);
}

template <typename T>
void Deserialize(T &stream, ContractExecutionStatus &status)
{
  int raw_status{0};
  stream >> raw_status;
  status = static_cast<ContractExecutionStatus>(raw_status);
}

template <typename T>
void Serialize(T &stream, ContractExecutionResult const &result)
{
  stream.Append(result.status, result.charge, result.charge_rate, result.fee, result.return_value);
}

template <typename T>
void Deserialize(T &stream, ContractExecutionResult &result)
{
  stream >> result.status >> result.charge >> result.charge_rate >> result.fee >>
      result.return_value;
}

}  // namespace ledger
}  // namespace fetch
