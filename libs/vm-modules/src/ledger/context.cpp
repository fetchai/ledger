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

#include "chain/transaction.hpp"
#include "vm/module.hpp"
#include "vm_modules/ledger/context.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

using namespace fetch::vm;

Context::Context(VM *vm, TypeId type_id, fetch::chain::Transaction const &tx,
                 BlockIndex block_index)
  : Object{vm, type_id}
  , transaction_{vm->CreateNewObject<vm_modules::ledger::Transaction>(tx)}
  , block_{vm->CreateNewObject<Block>(block_index)}
{}

void Context::Bind(vm::Module &module)
{
  module.CreateClassType<Context>("Context")
      .CreateMemberFunction("transaction", &Context::transaction)
      .CreateMemberFunction("block", &Context::block);
}

TransactionPtr Context::transaction() const
{
  return transaction_;
}

BlockPtr Context::block() const
{
  return block_;
}

void BindLedgerContext(vm::Module &module)
{
  Transfer::Bind(module);
  Transaction::Bind(module);
  Block::Bind(module);
  Context::Bind(module);
}

ContextPtr Context::Factory(vm::VM *vm, fetch::chain::Transaction const &tx, BlockIndex block_index)
{
  return vm->CreateNewObject<Context>(tx, block_index);
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
