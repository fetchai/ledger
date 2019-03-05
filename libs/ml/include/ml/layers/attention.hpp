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

#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Attention : public Layer<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Attention(std::uint64_t in, std::uint64_t out, std::string const &name = "Att")
    : Layer<T>(in, out)
  {
    this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Query", {});
    this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Key", {});
    this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Value", {});

    this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenQuery",
                                                               {name + "_Query"});
    this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenKey",
                                                               {name + "_Key"});
    this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenValue",
                                                               {name + "_Value"});

    this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_QKMatrixMultiply", {name + "_FlattenQuery", name + "_FlattenKey"});
    this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name + "_Softmax",
                                                               name + "_QKMatrixMultiply");
    this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_AVMatrixMultiply", {name + "_Softmax", name + "_Value"});

    this->AddInputNodes({name + "_Query", name + "_Key", name + "_Value"});
    this->SetOutputNode(name + "_AVMatrixMultiply");

    ArrayPtrType weights =
        std::make_shared<ArrayType>(std::vector<std::uint64_t>({this->in_size, this->out_size}));
    this->InitialiseWeights(weights);
    this->SetInput(name + "_Weights", weights);

    ArrayPtrType bias = std::make_shared<ArrayType>(std::vector<std::uint64_t>({1, out}));
    this->SetInput(name + "_Bias", bias);
  }

  static constexpr char const *DESCRIPTOR = "Attention";

private:
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
