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

#include "ml/layers/layer.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

#include "ml/meta/ml_type_traits.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class FullyConnected : public Layer<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  FullyConnected(std::uint64_t in, std::uint64_t out, std::string const &name = "FC",
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
    : Layer<T>(in, out)
  {

    this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_Flatten",
                                                               {name + "_Input"});
    this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weights", {});
    this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_MatrixMultiply", {name + "_Flatten", name + "_Weights"});
    this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Bias", {});
    this->template AddNode<fetch::ml::ops::Add<ArrayType>>(
        name + "_Add", {name + "_MatrixMultiply", name + "_Bias"});

    this->AddInputNodes(name + "_Input");
    this->SetOutputNode(name + "_Add");

    ArrayPtrType weights = std::make_shared<ArrayType>(std::vector<std::uint64_t>({in, out}));
    this->Initialise(weights, init_mode);
    this->SetInput(name + "_Weights", weights);

    ArrayPtrType bias = std::make_shared<ArrayType>(std::vector<std::uint64_t>({1, out}));
    this->SetInput(name + "_Bias", bias);
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
