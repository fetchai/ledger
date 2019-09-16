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

#include "core/byte_array/decoders.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/state_dict.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using SizeType       = fetch::math::SizeType;
using MathTensorType = fetch::math::Tensor<VMGraph::DataType>;
using VMTensorType   = fetch::vm_modules::math::VMTensor;
using VMPtrString    = Ptr<String>;

VMGraph::VMGraph(VM *vm, TypeId type_id)
  : Object(vm, type_id)
  , graph_()
{}

Ptr<VMGraph> VMGraph::Constructor(VM *vm, TypeId type_id)
{
  return new VMGraph(vm, type_id);
}

void VMGraph::SetInput(VMPtrString const &name, Ptr<VMTensorType> const &input)
{
  graph_.SetInput(name->str, (*input).GetTensor());
}

Ptr<VMTensorType> VMGraph::Evaluate(VMPtrString const &name)
{
  MathTensorType    t   = graph_.Evaluate(name->str);
  Ptr<VMTensorType> ret = this->vm_->CreateNewObject<math::VMTensor>(t.shape());
  (*ret).Copy(t);
  return ret;
}

void VMGraph::BackPropagate(VMPtrString const &name)
{
  graph_.BackPropagate(name->str);
}

void VMGraph::Step(DataType const &lr)
{
  auto grads = graph_.GetGradients();
  for (auto &grad : grads)
  {
    grad *= static_cast<DataType>(-lr);
  }
  graph_.ApplyGradients(grads);
}

void VMGraph::AddPlaceholder(VMPtrString const &name)
{
  graph_.AddNode<fetch::ml::ops::PlaceHolder<MathTensorType>>(name->str, {});
}

void VMGraph::AddFullyConnected(VMPtrString const &name, VMPtrString const &input_name, int in,
                                int out)
{
  graph_.AddNode<fetch::ml::layers::FullyConnected<MathTensorType>>(
      name->str, {input_name->str}, std::size_t(in), std::size_t(out));
}

void VMGraph::AddConv1D(VMPtrString const &name, VMPtrString const &input_name, int filters,
                        int in_channels, int kernel_size, int stride_size)
{
  graph_.AddNode<fetch::ml::layers::Convolution1D<MathTensorType>>(
      name->str, {input_name->str}, static_cast<SizeType>(filters),
      static_cast<SizeType>(in_channels), static_cast<SizeType>(kernel_size),
      static_cast<SizeType>(stride_size));
}

void VMGraph::AddRelu(VMPtrString const &name, VMPtrString const &input_name)
{
  graph_.AddNode<fetch::ml::ops::Relu<MathTensorType>>(name->str, {input_name->str});
}

void VMGraph::AddSoftmax(VMPtrString const &name, VMPtrString const &input_name)
{
  graph_.AddNode<fetch::ml::ops::Softmax<fetch::math::Tensor<DataType>>>(name->str,
                                                                         {input_name->str});
}

void VMGraph::AddCrossEntropyLoss(VMPtrString const &name, VMPtrString const &input_name,
                                  VMPtrString const &label_name)
{
  graph_.AddNode<fetch::ml::ops::CrossEntropyLoss<fetch::math::Tensor<DataType>>>(
      name->str, {input_name->str, label_name->str});
}

void VMGraph::AddMeanSquareErrorLoss(VMPtrString const &name, VMPtrString const &input_name,
                                     VMPtrString const &label_name)
{
  graph_.AddNode<fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<DataType>>>(
      name->str, {input_name->str, label_name->str});
}

void VMGraph::AddDropout(VMPtrString const &name, VMPtrString const &input_name,
                         DataType const &prob)
{
  graph_.AddNode<fetch::ml::ops::Dropout<MathTensorType>>(name->str, {input_name->str}, prob);
}

void VMGraph::LoadStateDict(Ptr<VMStateDict> const &sd)
{
  graph_.LoadStateDict(sd->state_dict_);
}

Ptr<VMStateDict> VMGraph::StateDict()
{
  Ptr<VMStateDict> ret = this->vm_->CreateNewObject<VMStateDict>(graph_.StateDict());
  return ret;
}

void VMGraph::Bind(Module &module)
{
  module.CreateClassType<VMGraph>("Graph")
      .CreateConstructor(&VMGraph::Constructor)
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) -> Ptr<VMGraph> { return new VMGraph(vm, type_id); })
      .CreateMemberFunction("setInput", &VMGraph::SetInput)
      .CreateMemberFunction("evaluate", &VMGraph::Evaluate)
      .CreateMemberFunction("backPropagate", &VMGraph::BackPropagate)
      .CreateMemberFunction("step", &VMGraph::Step)
      .CreateMemberFunction("addPlaceholder", &VMGraph::AddPlaceholder)
      .CreateMemberFunction("addFullyConnected", &VMGraph::AddFullyConnected)
      .CreateMemberFunction("addConv1D", &VMGraph::AddConv1D)
      .CreateMemberFunction("addRelu", &VMGraph::AddRelu)
      .CreateMemberFunction("addSoftmax", &VMGraph::AddSoftmax)
      .CreateMemberFunction("addDropout", &VMGraph::AddDropout)
      .CreateMemberFunction("addCrossEntropyLoss", &VMGraph::AddCrossEntropyLoss)
      .CreateMemberFunction("addMeanSquareErrorLoss", &VMGraph::AddMeanSquareErrorLoss)
      .CreateMemberFunction("loadStateDict", &VMGraph::LoadStateDict)
      .CreateMemberFunction("stateDict", &VMGraph::StateDict)
      .CreateMemberFunction("serializeToString", &VMGraph::SerializeToString)
      .CreateMemberFunction("deserializeFromString", &VMGraph::DeserializeFromString);
}

VMGraph::GraphType &VMGraph::GetGraph()
{
  return graph_;
}

bool VMGraph::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << graph_.GetGraphSaveableParams();
  return true;
}

bool VMGraph::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  fetch::ml::GraphSaveableParams<fetch::math::Tensor<fetch::vm_modules::math::DataType>> gsp;
  buffer >> gsp;

  auto vm_graph  = std::make_shared<fetch::vm_modules::ml::VMGraph>(this->vm_, this->type_id_);
  auto graph_ptr = std::make_shared<fetch::ml::Graph<MathTensorType>>(vm_graph->GetGraph());
  fetch::ml::utilities::BuildGraph<MathTensorType>(gsp, graph_ptr);

  vm_graph->GetGraph() = *graph_ptr;
  *this                = *vm_graph;

  return true;
}

fetch::vm::Ptr<fetch::vm::String> VMGraph::SerializeToString()
{
  serializers::MsgPackSerializer b;
  SerializeTo(b);
  auto byte_array_data = b.data().ToBase64();
  return {new fetch::vm::String(vm_, static_cast<std::string>(byte_array_data))};
}

fetch::vm::Ptr<VMGraph> VMGraph::DeserializeFromString(
    fetch::vm::Ptr<fetch::vm::String> const &graph_string)
{
  byte_array::ConstByteArray b(graph_string->str);
  b = byte_array::FromBase64(b);
  MsgPackSerializer buffer(b);
  DeserializeFrom(buffer);

  auto vm_graph        = fetch::vm::Ptr<VMGraph>(new VMGraph(vm_, type_id_));
  vm_graph->GetGraph() = graph_;

  return vm_graph;
}
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
