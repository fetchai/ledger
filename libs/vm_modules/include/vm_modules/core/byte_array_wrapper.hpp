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

#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {

class ByteArrayWrapper : public fetch::vm::Object
{
public:
  using ElementType = uint8_t;
  using TemplateParameter = vm::TemplateParameter;
  using AnyInteger = vm::AnyInteger;

  ByteArrayWrapper()          = delete;
  virtual ~ByteArrayWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<ByteArrayWrapper>("Buffer")
        .CreateConstuctor<int32_t>()
        .CreateMemberFunction("copy", &ByteArrayWrapper::Copy)
        .__EnableIndexOperator__<int32_t, uint8_t>();
  }

  ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   byte_array::ByteArray const &bytearray)
    : fetch::vm::Object(vm, type_id)
    , byte_array_(bytearray)
    , vm_(vm)
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

  TemplateParameter GetIndexedValue(AnyInteger const &index)
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      return TemplateParameter(*ptr, element_type_id_);
    }
    // Not found
    return TemplateParameter();
  }

  void SetIndexedValue(AnyInteger const &index, TemplateParameter const &value)
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      *ptr         = value.Get<ElementType>();
    }
  }

  ElementType *Find(AnyInteger const &index)
  {
    size_t i;
    if (GetNonNegativeInteger(index, i) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }

    if (i >= byte_array_.size())
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    return &byte_array_[i];
  }
/*
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
*/
  byte_array::ByteArray byte_array()
  {
    return byte_array_;
  }

private:
  byte_array::ByteArray byte_array_;
  fetch::vm::VM *       vm_;
};

}  // namespace vm_modules
}  // namespace fetch
