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
namespace ml {
class TensorWrapper : public fetch::vm::Object, public fetch::math::Tensor<float>
{

public:
  using ArrayType = fetch::math::Tensor<float>;
  using SizeType  = ArrayType::SizeType;

  TensorWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                std::vector<std::uint64_t> const &shape)
    : fetch::vm::Object(vm, type_id)
    , ArrayType(shape)
  {}

  static fetch::vm::Ptr<TensorWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                   fetch::vm::Ptr<fetch::vm::Array<SizeType>> shape)
  {
    return {new TensorWrapper(vm, type_id, shape->elements)};
  }
/* TODO: this is broken by design
  void SetAt(uint64_t index, float value)
  {
    (*this).At(index) = value;
  }
*/
  /*
  fetch::vm::Ptr<fetch::vm::String> ToString()
  {
    return new fetch::vm::String(vm_, (*this).Tensor::ToString());
  }
  */
};

inline void CreateTensor(std::shared_ptr<fetch::vm::Module> module)
{
  module->CreateClassType<TensorWrapper>("Tensor")
      .CreateTypeConstuctor<fetch::vm::Ptr<fetch::vm::Array<TensorWrapper::SizeType>>>();
//      .CreateInstanceFunction("SetAt", &TensorWrapper::SetAt)
//      .CreateInstanceFunction("ToString", &TensorWrapper::ToString);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
