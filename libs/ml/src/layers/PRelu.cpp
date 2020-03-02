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

#include "math/tensor/tensor.hpp"
#include "ml/layers/PRelu.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/prelu_op.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
PRelu<TensorType>::PRelu(uint64_t in, std::string const &name, WeightsInit init_mode)
{
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

  std::string alpha =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Alpha", {});

  TensorType alpha_data(std::vector<SizeType>({in, 1}));
  fetch::ml::ops::Weights<TensorType>::Initialise(alpha_data, in, in, init_mode);

  this->SetInput(alpha, alpha_data);

  std::string output = this->template AddNode<fetch::ml::ops::PReluOp<TensorType>>(
      name + "_PReluOp", {input, alpha});

  this->AddInputNode(input);
  this->SetOutputNode(output);

  this->Compile();
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> PRelu<TensorType>::GetOpSaveableParams()
{
  // get base class saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  auto ret = std::make_shared<SPType>();

  // copy subgraph saveable params over
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  return ret;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class PRelu<math::Tensor<int8_t>>;
template class PRelu<math::Tensor<int16_t>>;
template class PRelu<math::Tensor<int32_t>>;
template class PRelu<math::Tensor<int64_t>>;
template class PRelu<math::Tensor<float>>;
template class PRelu<math::Tensor<double>>;
template class PRelu<math::Tensor<fixed_point::fp32_t>>;
template class PRelu<math::Tensor<fixed_point::fp64_t>>;
template class PRelu<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
