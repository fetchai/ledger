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
#include "math/fundamental_operators.hpp"
#include "math/ml/activation_functions/softmax.hpp"
#include "math/standard_functions/log.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LogSoftmax : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  LogSoftmax()  = default;
  ~LogSoftmax() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(output.shape() == ComputeOutputShape(inputs));
    ASSERT(inputs.size() == 1);
    fetch::math::Softmax(inputs.front().get(), output);
    fetch::math::Log(output, output);
    return output;
  }

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    FETCH_UNUSED(inputs);
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape() == error_signal.shape());

    ArrayType ret = fetch::math::Exp(error_signal);
    fetch::math::Add(DataType(1), ret, ret);
    fetch::math::Divide(DataType(1), ret, ret);

    return {ret};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "LogSoftmax";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
