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

#include "ledger/upow/synergetic_contract_factory.hpp"
#include "ledger/upow/synergetic_executor_interface.hpp"

namespace fetch {
namespace ledger {

class SynergeticExecutor : public SynergeticExecutorInterface
{
public:

  // Construction / Destruction
  explicit SynergeticExecutor(StorageInterface &storage);
  SynergeticExecutor(SynergeticExecutor const &) = delete;
  SynergeticExecutor(SynergeticExecutor &&) = delete;
  ~SynergeticExecutor() override = default;

  /// @name Synergetic Executor Interface
  /// @{
  void Verify(WorkQueue &solutions) override;
  /// @}

  // Operators
  SynergeticExecutor &operator=(SynergeticExecutor const &) = delete;
  SynergeticExecutor &operator=(SynergeticExecutor &&) = delete;

private:

  SynergeticContractFactory factory_;
};

} // namespace ledger
} // namespace fetch
