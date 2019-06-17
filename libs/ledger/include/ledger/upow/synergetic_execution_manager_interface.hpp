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

namespace fetch {
namespace ledger {

class Block;

class SynergeticExecutionManagerInterface
{
public:
  // Construction / Destruction
  SynergeticExecutionManagerInterface()          = default;
  virtual ~SynergeticExecutionManagerInterface() = default;

  enum ExecStatus
  {
    SUCCESS,
    CONTRACT_NAME_PARSE_FAILURE,
    INVALID_CONTRACT_ADDRESS,
    INVALID_NODE,
    INVALID_BLOCK,
    CONTRACT_REGISTRATION_FAILED
  };

  /// @name Synergetic Execution Manager Interface
  /// @{
  virtual ExecStatus PrepareWorkQueue(Block const &current, Block const &previous)     = 0;
  virtual bool       ValidateWorkAndUpdateState(uint64_t block, std::size_t num_lanes) = 0;
  /// @}
};

inline char const *ToString(SynergeticExecutionManagerInterface::ExecStatus status)
{
  using ExecStatus = SynergeticExecutionManagerInterface::ExecStatus;

  char const *text = "Unknown";

  switch (status)
  {
  case ExecStatus::SUCCESS:
    text = "Success";
    break;
  case ExecStatus::CONTRACT_NAME_PARSE_FAILURE:
    text = "Contract name parse failure";
    break;
  case ExecStatus::INVALID_CONTRACT_ADDRESS:
    text = "Invalid contract address";
    break;
  case ExecStatus::INVALID_NODE:
    text = "Invalid node";
    break;
  case ExecStatus::INVALID_BLOCK:
    text = "Invalid block";
    break;
  case ExecStatus::CONTRACT_REGISTRATION_FAILED:
    text = "Contract registration failed";
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch
