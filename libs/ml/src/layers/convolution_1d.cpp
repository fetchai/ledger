//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/charge_estimation/constants.hpp"
#include "ml/charge_estimation/layers/constants.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/placeholder.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
Convolution1D<TensorType>::Convolution1D(SizeType const output_channels,
                                         SizeType const input_channels, SizeType const kernel_size,
                                         SizeType const                stride_size,
                                         details::ActivationType const activation_type,
                                         std::string const &name, WeightsInit const init_mode,
                                         SizeType const seed)
  : kernel_size_{kernel_size}
  , input_channels_{input_channels}
  , output_channels_{output_channels}
  , stride_size_{stride_size}
  , init_mode_{init_mode}
  , seed_{seed}
{
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});
  weights_ = this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});
  std::string output = this->template AddNode<fetch::ml::ops::Convolution1D<TensorType>>(
      name + "_Conv1D", {input, weights_}, stride_size_);

  output = fetch::ml::details::AddActivationNode<TensorType>(activation_type, this,
                                                             name + "_Activation", output);

  this->GetNode(weights_)->SetBatchOutputShape(
      {output_channels_, input_channels_, kernel_size_, 1});

  this->AddInputNode(input);
  this->SetOutputNode(output);
}

template <typename TensorType>
void Convolution1D<TensorType>::CompleteShapeDeduction()
{
  if (is_initialised_)
  {
    return;
  }
  using NodePtrType = std::shared_ptr<fetch::ml::Node<TensorType>>;
  FETCH_LOG_INFO(Descriptor(), "-- Finalising Convolution2D initialisation ... --");

  assert(!this->batch_input_shapes_.empty());
  assert(!this->batch_output_shape_.empty());
  assert(this->input_node_names_.size() == 1);  // Only 1 input node is allowed
  assert(output_channels_ == this->batch_output_shape_.front());

  NodePtrType input_node = this->nodes_.at(this->input_node_names_.front());
  input_node->SetBatchInputShapes(this->batch_input_shapes_);
  input_node->SetBatchOutputShape(this->batch_input_shapes_.front());

  FETCH_LOG_INFO(Descriptor(), "-- Convolution1D initialisation completed. --");
  is_initialised_ = true;
}

template <typename TensorType>
void Convolution1D<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  kernel_size_     = sp.kernel_size;
  input_channels_  = sp.input_channels;
  output_channels_ = sp.output_channels;
  stride_size_     = sp.stride_size;
  weights_         = sp.weights_name;
  seed_            = sp.seed;
  init_mode_       = static_cast<WeightsInit>(sp.init_mode);
}

template <class TensorType>
std::shared_ptr<OpsSaveableParams> Convolution1D<TensorType>::GetOpSaveableParams()
{
  // get all base classes saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  auto ret = std::make_shared<SPType>();

  // copy subgraph saveable params over
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->kernel_size     = kernel_size_;
  ret->input_channels  = input_channels_;
  ret->output_channels = output_channels_;
  ret->stride_size     = stride_size_;
  ret->weights_name    = weights_;
  ret->seed            = seed_;
  ret->init_mode       = static_cast<std::uint8_t>(init_mode_);

  return ret;
}

template <class TensorType>
std::vector<fetch::math::SizeType> Convolution1D<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  std::vector<SizeType> weights_data{output_channels_, input_channels_, kernel_size_, 1};
  return fetch::ml::ops::Convolution1D<TensorType>(stride_size_)
      .ComputeOutputShape({inputs.at(0), weights_data});
}

template <typename TensorType>
void Convolution1D<TensorType>::Compile()
{
  SubGraph<TensorType>::Compile();

  TensorType weights_data(
      std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, 1}});
  fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, 1, 1, init_mode_, seed_);
  this->SetInput(weights_, weights_data);
}

template <class TensorType>
OperationsCount Convolution1D<TensorType>::ChargeBackward() const
{
  return Graph<TensorType>::ChargeBackward(this->output_node_name_);
}

template <typename TensorType>
OperationsCount Convolution1D<TensorType>::ChargeCompile()
{
  OperationsCount op_cnt{charge_estimation::FUNCTION_CALL_COST};

  // Construct weights and bias tensors
  std::vector<SizeType> weights_data_shape({output_channels_, input_channels_, kernel_size_, 1});
  op_cnt += fetch::ml::ops::Weights<TensorType>::ChargeInitialise(weights_data_shape);

  // SetInput weights
  auto weights_dataholder =
      std::dynamic_pointer_cast<ops::DataHolder<TensorType>>(this->nodes_.at(weights_)->GetOp());
  op_cnt += weights_dataholder->ChargeSetData(weights_data_shape);

  // ResetGraphCache for weights and biases
  op_cnt += charge_estimation::layers::CONV_1D_CHARGE_COMPILE_PER_NODE * this->nodes_.size();

  op_cnt += Graph<TensorType>::ChargeCompile();
  return op_cnt;
}

template <typename TensorType>
OperationsCount Convolution1D<TensorType>::ChargeCompile()
{
  OperationsCount op_cnt{charge_estimation::FUNCTION_CALL_COST};

  // Construct weights and bias tensors
  std::vector<SizeType> weights_data_shape({output_channels_, input_channels_, kernel_size_, 1});
  op_cnt += fetch::ml::ops::Weights<TensorType>::ChargeInitialise(weights_data_shape);

  // SetInput weights
  auto weights_dataholder =
      std::dynamic_pointer_cast<ops::DataHolder<TensorType>>(this->nodes_.at(weights_)->GetOp());
  op_cnt += weights_dataholder->ChargeSetData(weights_data_shape);

  // ResetGraphCache for weights and biases
  op_cnt += charge_estimation::layers::CONV_1D_CHARGE_COMPILE_PER_NODE * this->nodes_.size();

  op_cnt += Graph<TensorType>::ChargeCompile();
  return op_cnt;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Convolution1D<math::Tensor<int8_t>>;
template class Convolution1D<math::Tensor<int16_t>>;
template class Convolution1D<math::Tensor<int32_t>>;
template class Convolution1D<math::Tensor<int64_t>>;
template class Convolution1D<math::Tensor<float>>;
template class Convolution1D<math::Tensor<double>>;
template class Convolution1D<math::Tensor<fixed_point::fp32_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp64_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
