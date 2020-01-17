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

#include "ml/layers/fully_connected.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"

#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
FullyConnected<TensorType>::FullyConnected(SizeType in, SizeType out,
                                           details::ActivationType       activation_type,
                                           fetch::ml::RegularisationType regulariser,
                                           DataType regularisation_rate, WeightsInit init_mode,
                                           bool time_distributed)
  : total_inputs_(in)
  , total_outputs_(out)
  , time_distributed_(time_distributed)
  , init_mode_(init_mode)
{
  using namespace fetch::ml::ops;
  using namespace fetch::ml::details;

  // get correct name for the layer
  std::string const name = GetName();

  // start to set up the structure
  std::string const input_name =
      this->template AddNode<PlaceHolder<TensorType>>(name + "_Input", {});
  // for non time distributed layer, flatten the input
  std::string flattened_input_name = input_name;
  if (!time_distributed_)
  {
    flattened_input_name =
        this->template AddNode<Flatten<TensorType>>(name + "_Flatten", {input_name});
  }

  weights_name_ = this->template AddNode<Weights<TensorType>>(name + "_Weights", {});

  std::string const weights_matmul_name = this->template AddNode<MatrixMultiply<TensorType>>(
      name + "_MatrixMultiply", {weights_name_, flattened_input_name});

  bias_name_ = this->template AddNode<Weights<TensorType>>(name + "_Bias", {});

  std::string const add =
      this->template AddNode<Add<TensorType>>(name + "_Add", {weights_matmul_name, bias_name_});

  std::string const output_name =
      AddActivationNode<TensorType>(activation_type, this, name + "_Activation", add);

  this->SetRegularisation(CreateRegulariser<TensorType>(regulariser), regularisation_rate);

  this->AddInputNode(input_name);
  this->SetOutputNode(output_name);

  // If inputs count is known, the initialisation can be completed immediately.
  if (total_inputs_ != AUTODETECT_INPUTS_COUNT)
  {
    if (time_distributed_)
    {
      this->batch_input_shapes_ = {{total_inputs_, 1, 1}};
    }
    else
    {
      this->batch_input_shapes_ = {{total_inputs_, 1}};
    }
    this->ComputeBatchOutputShape(this->batch_input_shapes_);
    CompleteConstruction();
  }
}

template <typename TensorType>
void FullyConnected<TensorType>::CompleteConstruction()
{
  if (is_initialised_)
  {
    return;
  }

  assert(!this->batch_input_shapes_.empty());
  assert(!this->batch_output_shape_.empty());
  assert(this->input_node_names_.size() == 1);  // Only 1 input node is allowed
  assert(total_outputs_ = this->batch_output_shape_.front());
  FETCH_LOG_INFO(Descriptor(), "-- Completing FullyConnected initialisation ... --");

  NodePtrType input_node = this->nodes_.at(this->input_node_names_.front());
  input_node->SetBatchInputShapes(this->batch_input_shapes_);
  input_node->SetBatchOutputShape(this->batch_input_shapes_.front());

  if (total_inputs_ == AUTODETECT_INPUTS_COUNT)
  {
    if (time_distributed_)
    {
      // An input size of a time-distributed layer is equal to a first dimension in
      // the input shape.
      total_inputs_ = this->batch_input_shapes_.front().at(0);
    }
    else
    {
      // An input size of a non-time-distributed layer is equal to total elements
      // in input tensor, e.g. equal to a flattened input output size.
      NodePtrType flatten_node = FindNodeByOpCode(OpType::OP_FLATTEN);
      total_inputs_ =
          flatten_node->GetOp()->ComputeBatchOutputShape(this->batch_input_shapes_).at(0);
    }
  }

  math::SizeVector const weights_shape = {total_outputs_, total_inputs_};
  // At this point we know everything necessary to directly assign shapes to
  // leaf nodes such as Weights and Bias.
  this->nodes_.at(weights_name_)->SetBatchOutputShape(weights_shape);
  this->nodes_.at(bias_name_)->SetBatchOutputShape(this->batch_output_shape_);

  // initialize weight with specified method.
  TensorType weights_data(weights_shape);
  fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, total_inputs_, total_outputs_,
                                                  init_mode_);
  this->SetInput(weights_name_, weights_data);

  TensorType bias_data = TensorType(this->batch_output_shape_);

  this->SetInput(bias_name_, bias_data);

  this->Compile();

  FETCH_LOG_INFO(Descriptor(), "-- FullyConnected initialisation completed. --");
  is_initialised_ = true;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> FullyConnected<TensorType>::MakeSharedCopy(
    OpPtrType me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);  // used for compatibility

  auto copyshare = std::make_shared<FullyConnected<TensorType>>();

  copyshare->time_distributed_   = time_distributed_;
  copyshare->total_inputs_       = total_inputs_;
  copyshare->total_outputs_      = total_outputs_;
  copyshare->batch_output_shape_ = this->batch_output_shape_;
  copyshare->batch_input_shapes_ = this->batch_input_shapes_;
  copyshare->is_initialised_     = is_initialised_;
  copyshare->weights_name_       = weights_name_;
  copyshare->bias_name_          = bias_name_;
  copyshare->init_mode_          = init_mode_;

  SubGraph<TensorType>::InsertSharedCopy(copyshare);

  return copyshare;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> FullyConnected<TensorType>::GetOpSaveableParams()
{
  auto ret = std::make_shared<SPType>();
  // get base class saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  // assign base class saveable params to ret
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->total_inputs_    = total_inputs_;
  ret->total_outputs_   = total_outputs_;
  ret->time_distributed = time_distributed_;
  ret->is_initialised   = is_initialised_;
  ret->weights_name     = weights_name_;
  ret->bias_name        = bias_name_;
  ret->init_mode        = static_cast<int>(init_mode_);

  return ret;
}

template <typename TensorType>
void FullyConnected<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  total_inputs_     = sp.total_inputs_;
  total_outputs_    = sp.total_outputs_;
  time_distributed_ = sp.time_distributed;
  is_initialised_   = sp.is_initialised;
  weights_name_     = sp.weights_name;
  bias_name_        = sp.bias_name;
  init_mode_        = static_cast<WeightsInit>(sp.init_mode);
}

template <typename TensorType>
math::SizeVector FullyConnected<TensorType>::ComputeOutputShape(VecTensorType const &inputs) const
{
  if (!time_distributed_)
  {
    SizeType total_in_size = 1;
    for (std::size_t i = 0; i < inputs.front()->shape().size() - 1; i++)
    {
      total_in_size *= inputs.front()->shape(i);
    }
    assert((this->total_inputs_ == AUTODETECT_INPUTS_COUNT) ||
           (total_in_size == this->total_inputs_));
    return {this->total_outputs_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
  }

  assert(inputs.front()->shape().size() == 3);
  assert(inputs.front()->shape(0) == total_inputs_);
  return {this->total_outputs_, inputs.front()->shape(inputs.front()->shape().size() - 2),
          inputs.front()->shape(inputs.front()->shape().size() - 1)};
}

template <typename TensorType>
math::SizeVector FullyConnected<TensorType>::ComputeBatchOutputShape(
    const std::vector<math::SizeVector> &input_shapes)
{
  if (!time_distributed_)
  {
    this->SetBatchInputShapes(input_shapes);
    this->SetBatchOutputShape({this->total_outputs_, 1});
    return this->batch_output_shape_;
  }

  assert((this->total_inputs_ == AUTODETECT_INPUTS_COUNT) ||
         (input_shapes.front().at(0) == total_inputs_));

  this->SetBatchInputShapes(input_shapes);
  if (input_shapes.front().size() == 3)
  {
    this->SetBatchOutputShape({this->total_outputs_, input_shapes.front().at(1), 1});
  }
  else
  {
    this->SetBatchOutputShape({this->total_outputs_, 1, 1});
  }
  return this->batch_output_shape_;
}

template <class TensorType>
std::string FullyConnected<TensorType>::GetName()
{
  if (time_distributed_)
  {
    return std::string("TimeDistributed_") + DESCRIPTOR;
  }
  return DESCRIPTOR;
}

template <class TensorType>
std::shared_ptr<fetch::ml::Node<TensorType>> FullyConnected<TensorType>::FindNodeByOpCode(
    OpType code)
{
  for (const auto &node_name_and_ptr : this->nodes_)
  {
    NodePtrType candidate = node_name_and_ptr.second;
    if (candidate->OperationType() == code)
    {
      return candidate;
    }
  }
  return nullptr;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class FullyConnected<math::Tensor<int8_t>>;
template class FullyConnected<math::Tensor<int16_t>>;
template class FullyConnected<math::Tensor<int32_t>>;
template class FullyConnected<math::Tensor<int64_t>>;
template class FullyConnected<math::Tensor<float>>;
template class FullyConnected<math::Tensor<double>>;
template class FullyConnected<math::Tensor<fixed_point::fp32_t>>;
template class FullyConnected<math::Tensor<fixed_point::fp64_t>>;
template class FullyConnected<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
