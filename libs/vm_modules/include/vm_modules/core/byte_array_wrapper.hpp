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

<<<<<<< HEAD
#include "vm/module.hpp"
=======
#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "vm/vm.hpp"
>>>>>>> feature/dag_v2

namespace fetch {
namespace vm_modules {

class ByteArrayWrapper : public fetch::vm::Object
{
public:
<<<<<<< HEAD
=======
  using ElementType = uint8_t;

>>>>>>> feature/dag_v2
  ByteArrayWrapper()          = delete;
  virtual ~ByteArrayWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<ByteArrayWrapper>("Buffer")
<<<<<<< HEAD
        .CreateConstuctor<int32_t>()
        .CreateMemberFunction("copy", &ByteArrayWrapper::Copy);
=======
        .CreateTypeConstuctor<int32_t>()
        .CreateInstanceFunction("copy", &ByteArrayWrapper::Copy)
        .EnableIndexOperator<uint8_t, int32_t>();
>>>>>>> feature/dag_v2
  }

  ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   byte_array::ByteArray const &bytearray)
    : fetch::vm::Object(vm, type_id)
    , byte_array_(bytearray)
<<<<<<< HEAD
=======
    , vm_(vm)
>>>>>>> feature/dag_v2
  {}

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      int32_t n)
  {
    return new ByteArrayWrapper(vm, type_id, byte_array::ByteArray(std::size_t(n)));
  }

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      byte_array::ByteArray bytearray)
  {
    return new ByteArrayWrapper(vm, type_id, bytearray);
  }

  fetch::vm::Ptr<ByteArrayWrapper> Copy()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(byte_array_.Copy());
  }

<<<<<<< HEAD
=======
  ElementType *Find()
  {
    vm::Variant &positionv = Pop();
    size_t       position;
    if (GetNonNegativeInteger(positionv, position) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }
    positionv.Reset();
    if (position >= byte_array_.size())
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    return &byte_array_[position];
  }

  void *FindElement() override
  {
    return Find();
  }

  void PushElement(fetch::vm::TypeId element_type_id) override
  {
    ElementType *ptr = Find();
    if (ptr)
    {
      vm::Variant &top = Push();
      top.Construct(*ptr, element_type_id);
    }
  }

  void PopToElement() override
  {
    ElementType *ptr = Find();
    if (ptr)
    {
      vm::Variant &top = Pop();
      *ptr             = top.Move<ElementType>();
    }
  }

>>>>>>> feature/dag_v2
  byte_array::ByteArray byte_array()
  {
    return byte_array_;
  }

private:
  byte_array::ByteArray byte_array_;
<<<<<<< HEAD
};

}  // namespace vm_modules
}  // namespace fetch
=======
  fetch::vm::VM *       vm_;
};

}  // namespace vm_modules
}  // namespace fetch
>>>>>>> feature/dag_v2
