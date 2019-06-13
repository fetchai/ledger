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
//#include "ml/layers/fully_connected.hpp"
//#include "ml/ops/activation.hpp"
//
//#include "vm/module.hpp"
//
//namespace fetch {
//namespace vm_modules {
//namespace ml {
//
//template <typename CostType>
//class OptimiserWrapper : public fetch::vm::Object, public fetch::ml::AdamOptimiser<fetch::math::Tensor<float>, CostType>
//{
//public:
//  OptimiserWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
//    : fetch::vm::Object(vm, type_id)
//    , fetch::ml::Graph<fetch::math::Tensor<float>>()
//  {}
//
//  static fetch::vm::Ptr<OptimiserWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
//  {
//    return new OptimiserWrapper(vm, type_id);
//  }
//
//  void SetInput(fetch::vm::Ptr<fetch::vm::String> const &name,
//                fetch::vm::Ptr<TensorWrapper> const &    input)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::SetInput(name->str, *input);
//  }
//
//  fetch::vm::Ptr<TensorWrapper> Evaluate(fetch::vm::Ptr<fetch::vm::String> const &name)
//  {
//    fetch::math::Tensor<float> t =
//        fetch::ml::Graph<fetch::math::Tensor<float>>::Evaluate(name->str);
//    fetch::vm::Ptr<TensorWrapper> ret = this->vm_->CreateNewObject<TensorWrapper>(t.shape());
//    (*ret).Copy(t);
//    return ret;
//  }
//
//  void Backpropagate(fetch::vm::Ptr<fetch::vm::String> const &name,
//                     fetch::vm::Ptr<TensorWrapper> const &    dt)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::BackPropagate(name->str, *dt);
//  }
//
//  void Step(float lr) override
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::Step(lr);
//  }
//
//  void AddPlaceholder(fetch::vm::Ptr<fetch::vm::String> const &name)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<
//        fetch::ml::ops::PlaceHolder<fetch::math::Tensor<float>>>(name->str, {});
//  }
//
//  void AddFullyConnected(fetch::vm::Ptr<fetch::vm::String> const &name,
//                         fetch::vm::Ptr<fetch::vm::String> const &inputName, int in, int out)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<
//        fetch::ml::layers::FullyConnected<fetch::math::Tensor<float>>>(
//        name->str, {inputName->str}, std::size_t(in), std::size_t(out));
//  }
//
//  void AddRelu(fetch::vm::Ptr<fetch::vm::String> const &name,
//               fetch::vm::Ptr<fetch::vm::String> const &inputName)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<
//        fetch::ml::ops::Relu<fetch::math::Tensor<float>>>(name->str, {inputName->str});
//  }
//
//  void AddSoftmax(fetch::vm::Ptr<fetch::vm::String> const &name,
//                  fetch::vm::Ptr<fetch::vm::String> const &inputName)
//  {
//    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<
//        fetch::ml::ops::Softmax<fetch::math::Tensor<float>>>(name->str, {inputName->str});
//  }
//};
//
//inline void CreateGraph(fetch::vm::Module &module)
//{
//  module.CreateClassType<OptimiserWrapper>("Graph")
//      .CreateConstuctor<>()
//      .CreateMemberFunction("SetInput", &OptimiserWrapper::SetInput)
//      .CreateMemberFunction("Evaluate", &OptimiserWrapper::Evaluate)
//      .CreateMemberFunction("Backpropagate", &OptimiserWrapper::Backpropagate)
//      .CreateMemberFunction("Step", &OptimiserWrapper::Step)
//      .CreateMemberFunction("AddPlaceholder", &OptimiserWrapper::AddPlaceholder)
//      .CreateMemberFunction("AddFullyConnected", &OptimiserWrapper::AddFullyConnected)
//      .CreateMemberFunction("AddRelu", &OptimiserWrapper::AddRelu)
//      .CreateMemberFunction("AddSoftmax", &OptimiserWrapper::AddSoftmax);
//}
//
//}  // namespace ml
//}  // namespace vm_modules
//}  // namespace fetch
