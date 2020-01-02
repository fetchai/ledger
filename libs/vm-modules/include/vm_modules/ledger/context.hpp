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

#include "vm_modules/ledger/block.hpp"
#include "vm_modules/ledger/transaction.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

class Context : public vm::Object
{
public:
  Context(vm::VM *vm, vm::TypeId type_id, fetch::chain::Transaction const &tx,
          BlockIndex block_index);

  TransactionPtr transaction() const;
  BlockPtr       block() const;

  static void       Bind(fetch::vm::Module &module);
  static ContextPtr Factory(vm::VM *vm, fetch::chain::Transaction const &tx,
                            BlockIndex block_index);

private:
  TransactionPtr transaction_;
  BlockPtr       block_;
};

void BindLedgerContext(vm::Module &module);

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
