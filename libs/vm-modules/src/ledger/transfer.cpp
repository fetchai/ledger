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

#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm_modules/ledger/transfer.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

using namespace fetch::vm;

Transfer::Transfer(VM *vm, TypeId type_id, AddressPtr to, NativeTokenAmount amount)
  : Object{vm, type_id}
  , address_{std::move(to)}
  , amount_{amount}
{}

void Transfer::Bind(Module &module)
{
  module.CreateClassType<Transfer>("Transfer")
      .CreateConstructor(&Transfer::Constructor)
      .CreateMemberFunction("to", &Transfer::to)
      .CreateMemberFunction("amount", &Transfer::amount);

  module.GetClassInterface<IArray>().CreateInstantiationType<GetManagedType<TransfersPtr>>();
}

TransferPtr Transfer::Constructor(VM *vm, TypeId type_id, AddressPtr const &to,
                                  NativeTokenAmount amount)
{
  return TransferPtr{new Transfer{vm, type_id, to, amount}};
}

AddressPtr Transfer::to() const
{
  return address_;
}

NativeTokenAmount Transfer::amount() const
{
  return amount_;
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
