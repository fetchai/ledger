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

#include "math/ml/loss_functions/mean_square_error.hpp"
#include "ml/ops/loss_functions/criterion.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareError : public Criterion<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MeanSquareError()          = default;
  virtual ~MeanSquareError() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].shape() == inputs[1].shape());

    typename ArrayType::Type ret = fetch::math::MeanSquareError(inputs[0], inputs[1]);
    return ret;
  }

  virtual ArrayType Backward(std::vector<ArrayType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].shape() == inputs[1].shape());

    ArrayType return_signal(inputs.front().shape());
    fetch::math::Subtract(inputs[0], inputs[1], return_signal);
    fetch::math::Multiply(return_signal, DataType{2}, return_signal);
    fetch::math::Divide(return_signal, static_cast<DataType>(inputs.at(0).shape().at(0)),
                        return_signal);

    return return_signal;
  }

  static constexpr char const *DESCRIPTOR = "MeanSquareError";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
