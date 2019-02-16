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

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "ledger/dag/dag.hpp"
#include "byte_array_wrapper.hpp"
namespace fetch
{
namespace modules
{

struct Item {
  byte_array::ConstByteArray contract;
  byte_array::ConstByteArray work_id;  
  byte_array::ConstByteArray owner;

  int64_t payload[4];
};

class ItemWrapper : public fetch::vm::Object
{
public:
  ItemWrapper()          = delete;
  virtual ~ItemWrapper() = default;

  static void Bind(vm::Module &module)
  {
    auto interface = module.CreateClassType<ItemWrapper>("Item");

    module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr<ItemWrapper>>(fetch::vm::TypeIds::IArray);      

    interface
      .CreateInstanceFunction("owner", &ItemWrapper::owner)
      .CreateInstanceFunction("payload", &ItemWrapper::payload);
  }

  ItemWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Item const &item)
    : fetch::vm::Object(vm, type_id)
    , item_(item)
  {}

  fetch::vm::Ptr< ByteArrayWrapper > owner()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(item_.owner);
  }

  int64_t payload(int32_t n)
  {
    if( (n < 0) || (n >= 4) )
    {
      this->vm_->RuntimeError("Index out of bounds");
      return 0;
    }

    return item_.payload[n];
  }

private:
  Item item_;
};


}
}