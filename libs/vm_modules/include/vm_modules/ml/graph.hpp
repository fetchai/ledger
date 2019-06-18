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

#include "vm_modules/ml/tensor.hpp"

#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"

#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
class GraphWrapper : public fetch::vm::Object, public fetch::ml::Graph<fetch::math::Tensor<float>>
{
  using TensorType = fetch::math::Tensor<float>;
public:
  GraphWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
    , fetch::ml::Graph<TensorType>()
  {}

  static fetch::vm::Ptr<GraphWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new GraphWrapper(vm, type_id);
  }

  void SetInput(fetch::vm::Ptr<fetch::vm::String> const &name,
                fetch::vm::Ptr<TensorWrapper> const &    input)
  {
    fetch::ml::Graph<TensorType>::SetInput(name->str, *input);
  }

  fetch::vm::Ptr<TensorWrapper> Evaluate(fetch::vm::Ptr<fetch::vm::String> const &name)
  {
    TensorType t =
        fetch::ml::Graph<TensorType>::Evaluate(name->str);
    fetch::vm::Ptr<TensorWrapper> ret = this->vm_->CreateNewObject<TensorWrapper>(t.shape());
    (*ret).Copy(t);
    return ret;
  }

  void Backpropagate(fetch::vm::Ptr<fetch::vm::String> const &name,
                     fetch::vm::Ptr<TensorWrapper> const &    dt)
  {
    fetch::ml::Graph<TensorType>::BackPropagate(name->str, *dt);
  }

  void Step(float lr) override
  {
    fetch::ml::Graph<TensorType>::Step(lr);
  }

  void AddPlaceholder(fetch::vm::Ptr<fetch::vm::String> const &name)
  {
    fetch::ml::Graph<TensorType>::AddNode<
        fetch::ml::ops::PlaceHolder<TensorType>>(name->str, {});
  }

  void AddFullyConnected(fetch::vm::Ptr<fetch::vm::String> const &name,
                         fetch::vm::Ptr<fetch::vm::String> const &inputName, int in, int out)
  {
    fetch::ml::Graph<TensorType>::AddNode<
        fetch::ml::layers::FullyConnected<TensorType>>(
        name->str, {inputName->str}, std::size_t(in), std::size_t(out));
  }

  void AddRelu(fetch::vm::Ptr<fetch::vm::String> const &name,
               fetch::vm::Ptr<fetch::vm::String> const &inputName)
  {
    fetch::ml::Graph<TensorType>::AddNode<
        fetch::ml::ops::Relu<TensorType>>(name->str, {inputName->str});
  }

  void AddSoftmax(fetch::vm::Ptr<fetch::vm::String> const &name,
                  fetch::vm::Ptr<fetch::vm::String> const &inputName)
  {
    fetch::ml::Graph<TensorType>::AddNode<
        fetch::ml::ops::Softmax<TensorType>>(name->str, {inputName->str});
  }

  void AddDropout(fetch::vm::Ptr<fetch::vm::String> const &name,
                  fetch::vm::Ptr<fetch::vm::String> const &input_name,
                  float const & prob)
  {
    fetch::ml::Graph<TensorType>::AddNode<
        fetch::ml::ops::Dropout<TensorType>>(name->str, {input_name->str}, prob);
  }


};

inline void CreateGraph(fetch::vm::Module &module)
{
  module.CreateClassType<GraphWrapper>("Graph")
      .CreateConstuctor<>()
      .CreateMemberFunction("SetInput", &GraphWrapper::SetInput)
      .CreateMemberFunction("Evaluate", &GraphWrapper::Evaluate)
      .CreateMemberFunction("Backpropagate", &GraphWrapper::Backpropagate)
      .CreateMemberFunction("Step", &GraphWrapper::Step)
      .CreateMemberFunction("AddPlaceholder", &GraphWrapper::AddPlaceholder)
      .CreateMemberFunction("AddFullyConnected", &GraphWrapper::AddFullyConnected)
      .CreateMemberFunction("AddRelu", &GraphWrapper::AddRelu)
      .CreateMemberFunction("AddSoftmax", &GraphWrapper::AddSoftmax)
      .CreateMemberFunction("AddDropout", &GraphWrapper::AddDropout);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
