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

#include "vectorise/uint/uint.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm/module.hpp"

// This is does not really say much about what Fetch.AI is and how it empowers the community

namespace fetch {
namespace vm_modules {

class UInt256Wrapper : public fetch::vm::Object
{
public:
  using SizeType = uint64_t;

  template <typename T>
  using Ptr    = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  UInt256Wrapper()          = delete;
  virtual ~UInt256Wrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<UInt256Wrapper>("UInt256")
        .CreateConstuctor<uint64_t>()     
        .CreateConstuctor<Ptr<vm::String>>()
        .CreateConstuctor<Ptr<ByteArrayWrapper>>()
//        .CreateMemberFunction("toBuffer", &UInt256Wrapper::ToBuffer)
        .CreateMemberFunction("increase", &UInt256Wrapper::Increase)
        .CreateMemberFunction("lessThan", &UInt256Wrapper::LessThan)
        .CreateMemberFunction("logValue", &UInt256Wrapper::LogValue)
        .CreateMemberFunction("toFloat64", &UInt256Wrapper::ToFloat64)
        .CreateMemberFunction("toInt32", &UInt256Wrapper::ToInt32)
        .CreateMemberFunction("size", &UInt256Wrapper::size);
  }

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray const& data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}


  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      fetch::vm::Ptr<ByteArrayWrapper> const &ba)
  {
    try
    {
      return new UInt256Wrapper(vm, type_id, ba->byte_array());
    }
    catch(std::runtime_error const &e)
    {
      vm->RuntimeError(e.what());
    }
    return nullptr;
  }

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      fetch::vm::Ptr<vm::String> const &ba)
  {
    try 
    {
      return new UInt256Wrapper(vm, type_id, ba->str);
    }
    catch(std::runtime_error const &e)
    {
      vm->RuntimeError(e.what());
    }
    return nullptr;
  }

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                      uint64_t const &val)
  {
    try 
    {
      return new UInt256Wrapper(vm, type_id, val);
    }
    catch(std::runtime_error const &e)
    {
      vm->RuntimeError(e.what());
    }
    return nullptr;
  }

  double ToFloat64()
  {
    return ToDouble(number_);
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
    return Log(number_);
  }

  /*
  fetch::vm::Ptr<ByteArrayWrapper> ToBuffer()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(number_.Copy());
  }
  */

  bool LessThan(Ptr<UInt256Wrapper> const &other)
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

  vectorise::UInt<256> const& number() const { return number_; }
private:
  vectorise::UInt<256> number_;
};


}  // namespace vm_modules
}  // namespace fetch
