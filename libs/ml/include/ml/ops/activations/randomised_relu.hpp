#pragma once
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

#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class RandomisedRelu : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using RNG           = fetch::random::LaggedFibonacciGenerator<>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpRandomisedReluSaveableParams<TensorType>;
  using MyType        = RandomisedRelu<TensorType>;

  RandomisedRelu(DataType const lower_bound, DataType const upper_bound,
                 SizeType const &random_seed = 25102015);

  explicit RandomisedRelu(SPType const &sp);

  ~RandomisedRelu() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_RANDOMISED_RELU;
  }
  static constexpr char const *DESCRIPTOR = "RandomisedRelu";

private:
  void UpdateRandomValue()
  {
    random_value_ = static_cast<DataType>(lower_bound_ +
                                          rng_.AsType<DataType>() * (upper_bound_ - lower_bound_));
  }

  DataType random_value_;
  DataType lower_bound_;
  DataType upper_bound_;
  DataType bounds_mean_;
  RNG      rng_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
