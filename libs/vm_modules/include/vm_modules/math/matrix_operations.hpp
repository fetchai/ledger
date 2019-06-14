//#pragma once
////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
//#include "math/matrix_operations.hpp"
//#include "math/meta/math_type_traits.hpp"
//
//#include "vm_modules/math/tensor.hpp"
//
//namespace fetch {
//namespace vm_modules {
//namespace math {
//
//template <typename T>
//T VMArgMax(fetch::vm::VM *, T const & a)
//{
//  // get reference to incoming tensor
//  fetch::math::Tensor<float> & a_tensor = (*a).GetTensor();
//
//  // construct return tensor
//  T ret;
//  fetch::math::Tensor<float> & ret_tensor = (*ret).GetTensor();
////  ret_tensor({1});
////  ret_tensor.ResizeFromShape({1});
//
//  FETCH_UNUSED(a_tensor);
//  FETCH_UNUSED(ret_tensor);
////  ret_tensor = fetch::math::Tensor<float>(a_tensor.shape());
//
//  // do argmax
////  fetch::math::ArgMax(a_tensor, ret_tensor);
//
//  // return result
//  return ret;
//
//}
//
//static void CreateArgMax(fetch::vm::Module &module)
//{
//  module.CreateFreeFunction<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>("ArgMax", &VMArgMax<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>);
//}
//
//
//inline void CreateArgMax(std::shared_ptr<vm::Module> module)
//{
//  CreateArgMax(*module.get());
//}
//
//}  // namespace math
//}  // namespace vm_modules
//}  // namespace fetch
