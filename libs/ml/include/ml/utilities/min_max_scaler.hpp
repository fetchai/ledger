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
  void Normalise(TensorType const &input_tensor, TensorType &output_tensor) override;
  void DeNormalise(TensorType const &input_tensor, TensorType &output_tensor) override;

private:
  TensorType x_min_;
  TensorType x_max_;
  TensorType x_range_;

  void InitialiseStatistics(SizeType tensor_shape);
};

/**
 * Calculate the min, max, and range for reference data
 * @tparam TensorType
 * @param reference_tensor
 */
template <typename TensorType>
void MinMaxScaler<TensorType>::SetScale(TensorType const &reference_tensor)
{
  SizeType batch_dim = reference_tensor.shape().size() - 1;
  InitialiseStatistics(reference_tensor.size() / reference_tensor.shape(batch_dim));

  // first loop computes min and max
  for (std::size_t i = 0; i < reference_tensor.shape(batch_dim); ++i)
  {
    auto x_min_it = x_min_.begin();
    auto x_max_it = x_max_.begin();
    auto ref_it   = reference_tensor.View(i).cbegin();
    while (x_min_it.is_valid())
    {
      if (*x_min_it > *ref_it)
      {
        *x_min_it = *ref_it;
      }

      if (*x_max_it < *ref_it)
      {
        *x_max_it = *ref_it;
      }

      ++ref_it;
      ++x_min_it;
      ++x_max_it;
    }
  }

  // second loop computes ranges
  auto x_min_it   = x_min_.begin();
  auto x_max_it   = x_max_.begin();
  auto x_range_it = x_range_.begin();
  while (x_min_it.is_valid())
  {
    if (*x_range_it < (*x_max_it - *x_min_it))
    {
      *x_range_it = (*x_max_it - *x_min_it);
    }

    ++x_min_it;
    ++x_max_it;
    ++x_range_it;
  }
}

/**
 * Normalise data according to previously set reference scale
 * @tparam TensorType
 * @param input_tensor
 * @param output_tensor
 */
template <typename TensorType>
void MinMaxScaler<TensorType>::Normalise(TensorType const &input_tensor, TensorType &output_tensor)
{
  output_tensor.Reshape(input_tensor.shape());
  SizeType batch_dim = input_tensor.shape().size() - 1;

  // apply normalisation to each feature according to scale -1, 1
  for (std::size_t i = 0; i < input_tensor.shape(batch_dim); ++i)
  {
    auto x_min_it   = x_min_.begin();
    auto x_range_it = x_range_.begin();
    auto in_it      = input_tensor.View(i).cbegin();
    auto ret_it     = output_tensor.View(i).begin();
    while (ret_it.is_valid())
    {
      *ret_it = (*in_it - *x_min_it) / (*x_range_it);

      ++in_it;
      ++ret_it;
      ++x_min_it;
      ++x_range_it;
    }
  }
}

/**
 * De normalise according to previously computed scale
 * @tparam TensorType
 * @param input_tensor
 * @param output_tensor
 */
template <typename TensorType>
void MinMaxScaler<TensorType>::DeNormalise(TensorType const &input_tensor,
                                           TensorType &      output_tensor)
{
  output_tensor.Reshape(input_tensor.shape());
  SizeType batch_dim = input_tensor.shape().size() - 1;

  // apply normalisation to each feature according to scale -1, 1
  for (std::size_t i = 0; i < input_tensor.shape(batch_dim); ++i)
  {
    auto x_min_it   = x_min_.begin();
    auto x_range_it = x_range_.begin();
    auto in_it      = input_tensor.View(i).cbegin();
    auto ret_it     = output_tensor.View(i).begin();
    while (ret_it.is_valid())
    {
      *ret_it = ((*in_it) * (*x_range_it)) + *x_min_it;

      ++in_it;
      ++ret_it;
      ++x_min_it;
      ++x_range_it;
    }
  }
}

/**
 * Initialise min, max, and range
 * @tparam TensorType
 * @param tensor_shape
 */
template <typename TensorType>
void MinMaxScaler<TensorType>::InitialiseStatistics(SizeType tensor_size)
{
  x_min_   = TensorType(tensor_size);
  x_max_   = TensorType(tensor_size);
  x_range_ = TensorType(tensor_size);

  // set min default to max
  x_min_.Fill(fetch::math::numeric_max<DataType>());

  // set max default to min
  x_max_.Fill(fetch::math::numeric_lowest<DataType>());

  // set range default to lowest
  x_range_.Fill(fetch::math::numeric_lowest<DataType>());
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
