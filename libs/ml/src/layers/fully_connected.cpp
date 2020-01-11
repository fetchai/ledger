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
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

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
{
  // get correct name for the layer
  std::string name = GetName();

  // start to set up the structure
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

  // for non time distributed layer, flatten the input
  std::string flat_input = input;
  if (!time_distributed_)
  {
    flat_input =
        this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input});
  }

  std::string weights =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});
  std::string weights_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
      name + "_MatrixMultiply", {weights, flat_input});
  std::string bias =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Bias", {});
  std::string output = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
      name + "_Add", {weights_matmul, bias});

  output = fetch::ml::details::AddActivationNode<TensorType>(activation_type, this,
                                                             name + "_Activation", output);

  this->AddInputNode(input);
  this->SetOutputNode(output);
  this->SetRegularisation(fetch::ml::details::CreateRegulariser<TensorType>(regulariser),
                          regularisation_rate);

  // initialize weight with specified method
  TensorType weights_data(std::vector<SizeType>({out_size_, in_size_}));
  this->Initialise(weights_data, init_mode);
  this->SetInput(name + "_Weights", weights_data);

  // initialize bias with right shape and set to all zero
  TensorType bias_data;
  if (time_distributed_)
  {
    bias_data = TensorType(std::vector<SizeType>({out_size_, 1, 1}));
  }
  else
  {
    bias_data = TensorType(std::vector<SizeType>({out_size_, 1}));
  }
  this->SetInput(name + "_Bias", bias_data);
  this->Compile();
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
std::vector<math::SizeType> FullyConnected<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  if (!time_distributed_)
  {
    SizeType total_in_size = 1;
    for (std::size_t i = 0; i < inputs.front()->shape().size() - 1; i++)
    {
      total_in_size *= inputs.front()->shape(i);
    }
    assert(total_in_size == this->in_size_);
    return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
  }

  assert(inputs.front()->shape().size() == 3);
  assert(inputs.front()->shape(0) == in_size_);
  return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 2),
          inputs.front()->shape(inputs.front()->shape().size() - 1)};
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// template class FullyConnected<math::Tensor<int8_t>>;
// template class FullyConnected<math::Tensor<int16_t>>;
template class FullyConnected<math::Tensor<int32_t>>;
template class FullyConnected<math::Tensor<int64_t>>;
// template class FullyConnected<math::Tensor<uint8_t>>;
// template class FullyConnected<math::Tensor<uint16_t>>;
template class FullyConnected<math::Tensor<uint32_t>>;
template class FullyConnected<math::Tensor<uint64_t>>;
template class FullyConnected<math::Tensor<float>>;
template class FullyConnected<math::Tensor<double>>;
template class FullyConnected<math::Tensor<fixed_point::fp32_t>>;
template class FullyConnected<math::Tensor<fixed_point::fp64_t>>;
template class FullyConnected<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
