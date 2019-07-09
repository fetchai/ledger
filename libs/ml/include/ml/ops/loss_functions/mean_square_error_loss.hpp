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

#include <cassert>
#include <memory>
#include <vector>

#include "math/ml/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareErrorLoss : public Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  MeanSquareErrorLoss()          = default;
  virtual ~MeanSquareErrorLoss() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape() == inputs.at(1).get().shape());

    // division by 2 allows us to cancel out with a 2 in the derivative for optimisation
    output(0, 0) = fetch::math::MeanSquareError(inputs.at(0).get(), inputs.at(1).get()) / 2;
  }

  // grad[0]=2*err*(in[0]-in[1])/mean_size, grad[1]=-2*err*(in[0]-in[1])/mean_size,
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape() == inputs.at(1).get().shape());

    ArrayType return_signal(inputs.front().get().shape());
    DataType  count = static_cast<DataType>(inputs.at(0).get().size());

    auto a_it = inputs.at(0).get().cbegin();
    auto b_it = inputs.at(1).get().cbegin();
    auto r_it = return_signal.begin();

    while (r_it.is_valid())
    {
      *r_it = ((*a_it) - (*b_it)) / count;
      ++a_it;
      ++b_it;
      ++r_it;
    }

    return {return_signal, return_signal};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    return {1, 1};
  }

  static constexpr char const *DESCRIPTOR = "MeanSquareErrorLoss";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
