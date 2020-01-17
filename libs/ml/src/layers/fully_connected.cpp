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
  : in_size_(in)
  , out_size_(out)
  , time_distributed_(time_distributed)
  , regulariser_(regulariser)
  , regularisation_rate_(regularisation_rate)
  , init_mode_(init_mode)
{
  // get correct name for the layer
  std::string name = GetName();

  // start to set up the structure
  input_ = this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});
  // for non time distributed layer, flatten the input
  flattened_input_ = input_;
  if (!time_distributed_)
  {
    flattened_input_ =
        this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input_});
  }

  weights_ = this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});

  std::string weights_matmul_ = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
      name + "_MatrixMultiply", {weights_, flattened_input_});

  bias_ = this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Bias", {});

  std::string add_ = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
      name + "_Add", {weights_matmul_, bias_});

  output_ = fetch::ml::details::AddActivationNode<TensorType>(activation_type, this,
                                                              name + "_Activation", add_);

  this->AddInputNode(input_);
  this->SetOutputNode(output_);

  // If inputs count is known, the initialisation can be completed immediately.
  if (in_size_ != AUTODETECT_INPUT_SHAPE)
  {
    if (time_distributed_)
    {
      this->batch_input_shapes_ = {{in_size_, 1, 1}};
    }
    else
    {
      this->batch_input_shapes_ = {{in_size_, 1}};
    }
    this->ComputeBatchOutputShape(this->batch_input_shapes_);
    CompleteInitialisation();
  }
}

template <typename TensorType>
void FullyConnected<TensorType>::CompleteInitialisation()
{
  if (is_initialised_)
  {
    return;
  }

  assert(!this->batch_input_shapes_.empty());
  assert(!this->batch_output_shape_.empty());
  FETCH_LOG_INFO(Descriptor(), "-- Completing FullyConnected initialisation ... --");
  this->nodes_.at(input_)->SetBatchInputShapes(this->batch_input_shapes_);
  this->nodes_.at(input_)->SetBatchOutputShape(this->batch_input_shapes_.front());

  if (in_size_ == AUTODETECT_INPUT_SHAPE)
  {
    if (time_distributed_)
    {
      math::SizeVector const &first_input_shape = this->batch_input_shapes_.front();
      // An input size of a time-distributed layer is equal to a first dimension in
      // the input shape.
      in_size_ = first_input_shape.front();
    }
    else
    {
      // An input size of a non-time-distributed layer is equal to total elements
      // in input tensor, e.g. equal to a flattened input output size.
      this->nodes_.at(flattened_input_)
          ->GetOp()
          ->ComputeBatchOutputShape(this->batch_input_shapes_);
      in_size_ = this->nodes_.at(flattened_input_)->BatchOutputShape().front();
    }
  }
  out_size_ = this->batch_output_shape_.front();

  // At this point we know everything necessary to directly assign shapes to
  // leaf nodes such as Weights and Bias.
  this->nodes_.at(weights_)->SetBatchOutputShape({out_size_, in_size_});
  this->nodes_.at(bias_)->SetBatchOutputShape(this->batch_output_shape_);

  // initialize weight with specified method.
  TensorType weights_data(std::vector<SizeType>({out_size_, in_size_}));
  fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, in_size_, out_size_, init_mode_);
  this->SetInput(weights_, weights_data);

  // initialize bias with right shape and set to all zero.
  TensorType bias_data;
  if (time_distributed_)
  {
    bias_data = TensorType(std::vector<SizeType>({out_size_, 1, 1}));
  }
  else
  {
    bias_data = TensorType(std::vector<SizeType>({out_size_, 1}));
  }
  this->SetInput(bias_, bias_data);

  this->SetRegularisation(fetch::ml::details::CreateRegulariser<TensorType>(regulariser_),
                          regularisation_rate_);
  this->Compile();
  FETCH_LOG_INFO(Descriptor(), "-- FullyConnected initialisation completed. --");
  is_initialised_ = true;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> FullyConnected<TensorType>::MakeSharedCopy(
    OpPtrType me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);  // used for compatability

  auto copyshare = std::make_shared<FullyConnected<TensorType>>();

  copyshare->time_distributed_ = time_distributed_;
  copyshare->in_size_          = in_size_;
  copyshare->out_size_         = out_size_;

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
  ret->in_size          = in_size_;
  ret->out_size         = out_size_;
  ret->time_distributed = time_distributed_;

  return ret;
}

template <typename TensorType>
void FullyConnected<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  in_size_          = sp.in_size;
  out_size_         = sp.out_size;
  time_distributed_ = sp.time_distributed;
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
    assert((this->in_size_ == AUTODETECT_INPUT_SHAPE) || (total_in_size == this->in_size_));
    return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
  }

  assert(inputs.front()->shape().size() == 3);
  assert(inputs.front()->shape(0) == in_size_);
  return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 2),
          inputs.front()->shape(inputs.front()->shape().size() - 1)};
}

template <typename TensorType>
math::SizeVector FullyConnected<TensorType>::ComputeBatchOutputShape(
    const std::vector<math::SizeVector> &input_shapes)
{
  if (time_distributed_)
  {
    assert((this->in_size_ == AUTODETECT_INPUT_SHAPE) || (input_shapes.front().at(0) == in_size_));

    this->SetBatchInputShapes(input_shapes);
    if (input_shapes.front().size() == 3)
    {
      this->SetBatchOutputShape({this->out_size_, input_shapes.front().at(1), 1});
    }
    else
    {
      this->SetBatchOutputShape({this->out_size_, 1, 1});
    }
    return this->batch_output_shape_;
  }

  this->SetBatchInputShapes(input_shapes);
  this->SetBatchOutputShape({this->out_size_, 1});
  return this->batch_output_shape_;
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
