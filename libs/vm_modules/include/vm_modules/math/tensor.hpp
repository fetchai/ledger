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

#include "math/tensor.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {
namespace math {
class VMTensor : public fetch::vm::Object
{

public:
  using ArrayType = fetch::math::Tensor<float>;
  using SizeType  = ArrayType::SizeType;

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                std::vector<std::uint64_t> const &shape)
    : fetch::vm::Object(vm, type_id)
    , tensor_(shape)
  {
  }

  static fetch::vm::Ptr<VMTensor> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                   fetch::vm::Ptr<fetch::vm::Array<SizeType>> shape)
  {
    return {new VMTensor(vm, type_id, shape->elements)};
  }

  void SetAt(uint64_t index, float value)
  {
    tensor_.At(index) = value;
  }

  void Copy(ArrayType const &other)
  {
    tensor_.Copy(other);
  }

  fetch::vm::Ptr<fetch::vm::String> ToString()
  {
    return new fetch::vm::String(vm_, tensor_.ToString());
  }

  fetch::vm::Ptr<fetch::vm::Array<VMTensor::SizeType>> shape()
  {
    return new fetch::vm::Array<VMTensor::SizeType>(tensor_.shape());
  }

  ArrayType const & GetTensor()
  {
    return tensor_;
  }

private:
  ArrayType tensor_;

};

inline void CreateTensor(fetch::vm::Module &module)
{
  module.CreateClassType<VMTensor>("Tensor")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm::Array<VMTensor::SizeType>>>()
      .CreateMemberFunction("SetAt", &VMTensor::SetAt)
      .CreateMemberFunction("ToString", &VMTensor::ToString);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
