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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/weights.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class FullyConnected : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  FullyConnected(SizeType in, SizeType out,
                 details::ActivationType activation_type = details::ActivationType::NOTHING,
                 fetch::ml::details::RegularisationType regulariser =
                     fetch::ml::details::RegularisationType::NONE,
                 DataType           regularisation_rate = static_cast<DataType>(0),
                 std::string const &name = "FC", WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
    : in_size_(in)
    , out_size_(out)
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    std::string flat_input =
        this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_Flatten", {input});
    std::string weights =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weights", {});
    std::string weights_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_MatrixMultiply", {weights, flat_input});
    std::string bias =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Bias", {});
    std::string output = this->template AddNode<fetch::ml::ops::Add<ArrayType>>(
        name + "_Add", {weights_matmul, bias});

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);
    this->SetRegularisation(fetch::ml::details::CreateRegulariser<T>(regulariser),
                            regularisation_rate);

    ArrayType weights_data(std::vector<SizeType>({out, in}));
    this->Initialise(weights_data, init_mode);
    this->SetInput(weights, weights_data);

    ArrayType bias_data(std::vector<SizeType>({out, 1}));
    this->SetInput(bias, bias_data);
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &) const
  {
    return {this->out_size_, 1};
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";

private:
  SizeType in_size_;
  SizeType out_size_;

  void Initialise(ArrayType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, in_size_, out_size_, init_mode);
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
