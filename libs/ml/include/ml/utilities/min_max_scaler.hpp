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

#include "core/serializers/base_types.hpp"
#include "math/base_types.hpp"
#include "ml/utilities/scaler.hpp"

namespace fetch {
namespace ml {
namespace utilities {

template <typename TensorType>
class MinMaxScaler : public Scaler<TensorType>
{
public:
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = fetch::math::SizeVector;

  MinMaxScaler() = default;

  void SetScale(TensorType const &reference_tensor) override;
  void SetScale(DataType const &min_val, DataType const &max_val) override;
  void Normalise(TensorType const &input_tensor, TensorType &output_tensor) override;
  void DeNormalise(TensorType const &input_tensor, TensorType &output_tensor) override;

  DataType x_min_   = fetch::math::numeric_max<DataType>();
  DataType x_max_   = fetch::math::numeric_lowest<DataType>();
  DataType x_range_ = fetch::math::numeric_max<DataType>();
};

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
