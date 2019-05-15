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

#include "math/bignumber.hpp"

#include "vm_modules/core/byte_array_wrapper.hpp"

#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {

// TODO: Make templated
class BigNumberWrapper : public fetch::vm::Object
{
public:
  using SizeType = uint64_t;

  template <typename T>
  using Ptr    = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  BigNumberWrapper()          = delete;
  virtual ~BigNumberWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<BigNumberWrapper>("BigUInt")
        .CreateConstuctor<Ptr<ByteArrayWrapper>>()
        .CreateMemberFunction("toBuffer", &BigNumberWrapper::ToBuffer)
        .CreateMemberFunction("increase", &BigNumberWrapper::Increase)
        .CreateMemberFunction("lessThan", &BigNumberWrapper::LessThan)
        .CreateMemberFunction("logValue", &BigNumberWrapper::LogValue)
        .CreateMemberFunction("toFloat64", &BigNumberWrapper::ToFloat64)
        .CreateMemberFunction("toInt32", &BigNumberWrapper::ToInt32)
        .CreateMemberFunction("size", &BigNumberWrapper::size);
  }

  BigNumberWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  static fetch::vm::Ptr<BigNumberWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      fetch::vm::Ptr<ByteArrayWrapper> const &ba)
  {
    return new BigNumberWrapper(vm, type_id, ba->byte_array());
  }

  double ToFloat64()
  {
    return math::ToDouble(number_);
  }

  int32_t ToInt32()
  {
    union
    {
      uint8_t bytes[4];
      int32_t value;
    } x;
    x.bytes[0] = number_[0];
    x.bytes[1] = number_[1];
    x.bytes[2] = number_[2];
    x.bytes[3] = number_[3];
    return x.value;
  }

  double LogValue()
  {
    return math::Log(number_);
  }

  fetch::vm::Ptr<ByteArrayWrapper> ToBuffer()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(number_.Copy());
  }

  bool LessThan(Ptr<BigNumberWrapper> const &other)
  {
    return number_ < other->number_;
  }

  void Increase()
  {
    ++number_;
  }

  SizeType size() const
  {
    return number_.size();
  }

private:
  math::BigUnsigned number_;
};

}  // namespace vm_modules
}  // namespace fetch
