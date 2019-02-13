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
  byte_array::ConstByteArray owner;
  int64_t type;
  int64_t id;
  int64_t value;  
};

class ItemWrapper : public fetch::vm::Object
{
public:
  ItemWrapper()          = delete;
  virtual ~ItemWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<ItemWrapper>("Item")
      .CreateInstanceFunction("owner", &ItemWrapper::owner)
      .CreateInstanceFunction("type", &ItemWrapper::type)      
      .CreateInstanceFunction("id", &ItemWrapper::id) 
      .CreateInstanceFunction("value", &ItemWrapper::value)      
      .CreateTypeConstuctor<>();
      // TODO: Does not work:
     //  .CreateTypeConstuctor<ByteArrayWrapper, ByteArrayWrapper, int64_t, int64_t, int64_t>();
  }

  ItemWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Item const &item)
    : fetch::vm::Object(vm, type_id)
    , item_(item)
    , vm_(vm)
  {}

  static fetch::vm::Ptr<ItemWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             ByteArrayWrapper contract, ByteArrayWrapper owner, 
                                             int64_t type, int64_t id, int64_t value)
  {
    return new ItemWrapper(vm, type_id, Item{contract.byte_array(), owner.byte_array(), type, id, value} );
  }

  static fetch::vm::Ptr<ItemWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new ItemWrapper(vm, type_id, Item{"hello", "world",2,3,4} );
  }

  fetch::vm::Ptr< ByteArrayWrapper > owner()
  {
    return vm_->NewObject<ByteArrayWrapper>(item_.owner);
  }

  int64_t type()
  {
    return item_.type;
  }

  int64_t id()
  {
    return item_.id;
  }

  int64_t value()
  {
    return item_.value;
  }
private:
  Item item_;
  fetch::vm::VM *vm_;
};


}
}