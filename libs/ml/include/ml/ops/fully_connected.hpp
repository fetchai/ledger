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

#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class FullyConnected : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  FullyConnected(size_t in, size_t out, std::string const &name = "FC")
  {
    this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_Flatten",
                                                               {name + "_Input"});
    this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weights", {});
    this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_MatrixMultiply", {name + "_Flatten", name + "_Weights"});

    this->AddInputNodes(name + "_Input");
    this->SetOutputNode(name + "_MatrixMultiply");

    ArrayPtrType weights = std::make_shared<ArrayType>(std::vector<size_t>({in, out}));

    this->SetInput(name + "_Weights", weights);
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
