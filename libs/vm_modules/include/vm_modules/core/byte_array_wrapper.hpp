#pragma once

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "vm/vm.hpp"


namespace fetch
{
namespace modules
{


class ByteArrayWrapper : public fetch::vm::Object
{
public:
  using ElementType = uint8_t;

  ByteArrayWrapper()          = delete;
  virtual ~ByteArrayWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<ByteArrayWrapper>("ByteArray")
      .CreateTypeConstuctor<int32_t>()
      .CreateInstanceFunction("copy", &ByteArrayWrapper::Copy)
      .EnableIndexOperator<uint8_t, int32_t>();
  }

  ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray const &bytearray)
    : fetch::vm::Object(vm, type_id)
    , byte_array_(bytearray)
    , vm_(vm)
  {}

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             int32_t n)
  {
    return new ByteArrayWrapper(vm, type_id, byte_array::ByteArray(std::size_t(n) ) );
  }

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             byte_array::ByteArray bytearray)
  {
    return new ByteArrayWrapper(vm, type_id, bytearray );
  }

  fetch::vm::Ptr<ByteArrayWrapper> Copy()
  {
    return vm_->CreateNewObject< ByteArrayWrapper >(byte_array_.Copy());
  }

  ElementType *Find()
  {
    vm::Variant &positionv = Pop();
    size_t   position;
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
      *ptr         = top.Move<ElementType>();
    }
  }

  byte_array::ByteArray byte_array()
  {
    return byte_array_;
  }
private:
  byte_array::ByteArray byte_array_;
  fetch::vm::VM *vm_;
};

}
}