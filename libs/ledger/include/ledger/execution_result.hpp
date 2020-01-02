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

#include "ledger/consensus/stake_update_event.hpp"

#include <cstdint>

namespace fetch {
namespace ledger {

using TokenAmount = uint64_t;

enum class ExecutionStatusCategory
{
  SUCCESS = 0,
  BLOCK_INVALIDATING_ERROR,
  NORMAL_ERROR,
  INTERNAL_ERROR
};

enum class ContractExecutionStatus
{
  SUCCESS = 0,

  /// @name Block Invalidating Errors
  /// @{
  TX_LOOKUP_FAILURE,         ///< Unable to lookup the transaction contents
  TX_NOT_VALID_FOR_BLOCK,    ///< The transaction is in a block for which it is invalid
  TX_PERMISSION_DENIED,      ///< Evaluate if the transaction has permission to make the transfer
  TX_NOT_ENOUGH_CHARGE,      ///< Not enough charge available to make the required transfers
  TX_CHARGE_LIMIT_TOO_HIGH,  ///< The specified charge limit exceeds the allowed maximum
  /// @}

  /// @name General Errors
  /// @{
  INSUFFICIENT_AVAILABLE_FUNDS,  ///< The account holder did not have sufficent funds available
  CONTRACT_NAME_PARSE_FAILURE,   ///< The contract name could not be parsed
  CONTRACT_LOOKUP_FAILURE,       ///< The contract was not found
  ACTION_LOOKUP_FAILURE,         ///< The action on the contract was not found
  CONTRACT_EXECUTION_FAILURE,    ///< The contract action failed to execute succesffully
  TRANSFER_FAILURE,              ///< A transfer failed
  INSUFFICIENT_CHARGE,           ///< The transaction reached the charge limit
  /// @}

  /// @name Internal Errors
  /// @{
  NOT_RUN,               ///< Status result indicating that the contract has not been run
  INTERNAL_ERROR,        ///< Internal error when executing
  INEXPLICABLE_FAILURE,  ///< Catch all error
  /// @}
};

struct ContractExecutionResult
{
  // The status of the transaction
  ContractExecutionStatus status{ContractExecutionStatus::NOT_RUN};
  TokenAmount             charge{0};        ///< The number of units of charge
  TokenAmount             charge_rate{0};   ///< The cost of each unit of charge
  TokenAmount             charge_limit{0};  ///< Max. limit for units to charge defined by Tx sender
  TokenAmount             fee{0};           ///< The total fee claimed by the miner
  int64_t                 return_value{0};  ///< Return value from executed contract function
  StakeUpdateEvents stake_updates{};  ///< The stake updates that occured during this execution
};

constexpr char const *ToString(ContractExecutionStatus status)
{
  switch (status)
  {
  case ContractExecutionStatus::SUCCESS:
    return "Success";
  case ContractExecutionStatus::TX_LOOKUP_FAILURE:
    return "Tx Lookup Failure";
  case ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK:
    return "Tx Not Valid For Current Block";
  case ContractExecutionStatus::TX_PERMISSION_DENIED:
    return "Permission Denied";
  case ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE:
    return "Not Enough Charge";
  case ContractExecutionStatus::TX_CHARGE_LIMIT_TOO_HIGH:
    return "Charge Limit Too High";
  case ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS:
    return "Insufficient available funds";
  case ContractExecutionStatus::CONTRACT_NAME_PARSE_FAILURE:
    return "Contract Name Parse Failure";
  case ContractExecutionStatus::CONTRACT_LOOKUP_FAILURE:
    return "Contract Lookup Failure";
  case ContractExecutionStatus::ACTION_LOOKUP_FAILURE:
    return "Contract Action Lookup Failure";
  case ContractExecutionStatus::CONTRACT_EXECUTION_FAILURE:
    return "Contract Execution Failure";
  case ContractExecutionStatus::TRANSFER_FAILURE:
    return "Unable To Perform Transfer";
  case ContractExecutionStatus::INSUFFICIENT_CHARGE:
    return "Insufficient charge";
  case ContractExecutionStatus::NOT_RUN:
    return "Not Run";
  case ContractExecutionStatus::INTERNAL_ERROR:
    return "Internal Error";
  case ContractExecutionStatus::INEXPLICABLE_FAILURE:
    return "Inexplicable Error";
  }

  return "Unknown";
}

constexpr ExecutionStatusCategory Categorise(ContractExecutionStatus status)
{
  switch (status)
  {
  case ContractExecutionStatus::SUCCESS:
    return ExecutionStatusCategory::SUCCESS;

  case ContractExecutionStatus::TX_LOOKUP_FAILURE:
  case ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK:
  case ContractExecutionStatus::TX_PERMISSION_DENIED:
  case ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE:
  case ContractExecutionStatus::TX_CHARGE_LIMIT_TOO_HIGH:
    return ExecutionStatusCategory::BLOCK_INVALIDATING_ERROR;

  case ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS:
  case ContractExecutionStatus::CONTRACT_NAME_PARSE_FAILURE:
  case ContractExecutionStatus::CONTRACT_LOOKUP_FAILURE:
  case ContractExecutionStatus::ACTION_LOOKUP_FAILURE:
  case ContractExecutionStatus::CONTRACT_EXECUTION_FAILURE:
  case ContractExecutionStatus::TRANSFER_FAILURE:
  case ContractExecutionStatus::INSUFFICIENT_CHARGE:
    return ExecutionStatusCategory::NORMAL_ERROR;

  case ContractExecutionStatus::NOT_RUN:
  case ContractExecutionStatus::INTERNAL_ERROR:
  case ContractExecutionStatus::INEXPLICABLE_FAILURE:
    break;
  }

  return ExecutionStatusCategory::INTERNAL_ERROR;
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
