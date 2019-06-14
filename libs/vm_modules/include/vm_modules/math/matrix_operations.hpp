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

#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"

#include "vm_modules/math/tensor.hpp"

namespace fetch {
namespace vm_modules {
namespace math {

/**
 * method for taking the argmax of a tensor
 */
//template <typename T>
//T VMArgMax(fetch::vm::VM * /*vm*/, T const & /*a*/)

template <typename T>
T VMArgMax(T const & a)
{

  //  // Construct return tensor (get shape from input tensor)
  //  std::vector<std::uint64_t> tensor_shape = (*a).shape();
  //  fetch::vm::TypeId type_id = ((*a).GetTypeId());
  //  fetch::vm_modules::math::VMTensor ret(vm, type_id, tensor_shape);//, (*a).GetTypeId(),
  //  (*a).shape());
  //
  //  // do the argmax
  //  fetch::math::Tensor<float> a_tensor = (*a).GetTensor();
  //  fetch::math::Tensor<float> ret_tensor = ret.GetTensor();
  //
  //  fetch::math::ArgMax(a_tensor, ret_tensor);
  //
  //  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ret_ptr =
  //  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>(&ret); return ret_ptr;


//  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ret_ptr;
//  return ret_ptr;
  return a;
}

static void CreateArgMax(fetch::vm::Module &module)
{
//  module.CreateFreeFunction<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>(
//      "ArgMax", &VMArgMax<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>);
  module.CreateFreeFunction<int32_t >("ArgMax", &VMArgMax<int32_t>);
}

inline void CreateArgMax(std::shared_ptr<vm::Module> module)
{
  CreateArgMax(*module.get());
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
