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

#include "core/macros.hpp"
#include "core/random/lfg.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Dropout : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using RNG          = fetch::random::LaggedFibonacciGenerator<>;

  Dropout(DataType const probability, SizeType const &random_seed = 25102015)
    : probability_(probability)
  {
    ASSERT(probability >= 0.0 && probability <= 1.0);
    rng_.Seed(random_seed);
    drop_values_ = ArrayType{0};
  }

  virtual ~Dropout() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));

    if (!this->is_training_)
    {
      output.Copy(inputs.front().get());
      return output;
    }

    if (drop_values_.shape() != output.shape())
    {
      drop_values_ = ArrayType(inputs.front().get().shape());
    }
    UpdateRandomValues();

    fetch::math::Multiply(inputs.front().get(), drop_values_, output);

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(errorSignal.shape() == inputs.front().get().shape());
    ASSERT(drop_values_.shape() == inputs.front().get().shape());

    ArrayType returnSignal{errorSignal.shape()};

    // gradient of dropout is 1.0 for enabled neurons and 0.0 for disabled
    // multiply by errorSignal (chain rule)
    if (this->is_training_)
    {
      fetch::math::Multiply(errorSignal, drop_values_, returnSignal);
    }
    else
    {
      returnSignal.Copy(errorSignal);
    }

    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "Dropout";

private:
  void UpdateRandomValues()
  {
    DataType zero{0};
    DataType one{1};

    auto it = drop_values_.begin();
    while (it.is_valid())
    {
      if (DataType(rng_.AsDouble()) <= probability_)
      {
        *it = one;
      }
      else
      {
        *it = zero;
      }
      ++it;
    }
  }

  ArrayType drop_values_;
  DataType  probability_;
  RNG       rng_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
