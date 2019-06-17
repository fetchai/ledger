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
//#include "vm_modules/ml/tensor.hpp"
//
//#include "ml/optimisation/adam_optimiser.hpp"
//
//#include "vm/module.hpp"
//
// namespace fetch {
// namespace vm_modules {
// namespace ml {
//
// template <typename CostType>
// class AdamOptimiser : public fetch::vm::Object
//{
// public:
//  AdamOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
//    : fetch::vm::Object(vm, type_id)
//    , fetch::ml::Graph<fetch::math::Tensor<float>>()
//  {}
//
//  static fetch::vm::Ptr<AdamOptimiser> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
//  {
//    return new AdamOptimiser(vm, type_id);
//  }
//
//  DataType Run(std::vector<ArrayType> const &data, ArrayType const &labels, SizeType batch_size)
//  {
//    optimiser_.Run();
//  }
//  DataType Run(fetch::ml::DataLoader<ArrayType, ArrayType> &loader, SizeType batch_size = 0,
//               SizeType subset_size = fetch::math::numeric_max<SizeType>());
//
// private:
//  fetch::ml::AdamOptimiser<fetch::math::Tensor<float>, CostType> optimiser_;
//};
//
// inline void CreateGraph(fetch::vm::Module &module)
//{
//  module.CreateClassType<AdamOptimiser>("Graph")
//      .CreateConstuctor<>()
//      .CreateMemberFunction("SetInput", &AdamOptimiser::SetInput)
//      .CreateMemberFunction("Evaluate", &AdamOptimiser::Evaluate)
//      .CreateMemberFunction("Backpropagate", &AdamOptimiser::Backpropagate)
//      .CreateMemberFunction("Step", &AdamOptimiser::Step)
//      .CreateMemberFunction("AddPlaceholder", &AdamOptimiser::AddPlaceholder)
//      .CreateMemberFunction("AddFullyConnected", &AdamOptimiser::AddFullyConnected)
//      .CreateMemberFunction("AddRelu", &AdamOptimiser::AddRelu)
//      .CreateMemberFunction("AddSoftmax", &AdamOptimiser::AddSoftmax);
//}
//
//}  // namespace ml
//}  // namespace vm_modules
//}  // namespace fetch
