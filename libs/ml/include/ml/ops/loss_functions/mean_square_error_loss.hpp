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

#include "math/metrics/mean_square_error.hpp"
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

  explicit MeanSquareErrorLoss(ArrayType const &weightings = ArrayType())
    : weightings_(weightings)
  {}
  virtual ~MeanSquareErrorLoss() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());

    if (weightings_.size() == static_cast<SizeType>(0))
    {
      output(0, 0) = fetch::math::MeanSquareError((*inputs.at(0)), (*inputs.at(1)));
    }
    // rescale according to weights
    else
    {
      SizeType data_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);

      auto it1 = inputs.at(0)->cbegin();
      auto it2 = inputs.at(1)->cbegin();

      // weighting is scalar
      if (weightings_.shape().size() == static_cast<SizeType>(1))
      {
        while (it1.is_valid())
        {
          DataType d = (*it1) - (*it2);
          output(0, 0) += (d * d) * weightings_(0);
          ++it1;
          ++it2;
        }
      }
      // weighting tensor is same shape as input (one weight for every parameter)
      else if (weightings_.shape() == inputs.at(0)->shape())
      {
        auto w_it = weightings_.cbegin();
        while (it1.is_valid())
        {
          DataType d = (*it1) - (*it2);
          output(0, 0) += (d * d) * (*w_it);

          ++it1;
          ++it2;
          ++w_it;
        }
      }
      // weighting is a batch_size vector (one weight per data point)
      else if (weightings_.shape() == std::vector<SizeType>{data_size})
      {
        SizeType data_count = 0;
        SizeType data_stride;
        fetch::math::Divide(inputs.at(0)->size(), weightings_.size(), data_stride);

        auto w_it = weightings_.cbegin();
        while (it1.is_valid())
        {
          DataType d = (*it1) - (*it2);
          output(0, 0) += (d * d) * (*w_it);
          ++it1;
          ++it2;

          ++data_count;
          if (data_count == data_stride)
          {
            data_count = 0;
            ++w_it;
          }
        }
      }

      // divide by number of elements
      fetch::math::Divide(output(0, 0), static_cast<DataType>(inputs.at(0)->size()),
                          output(0, 0));
    }

    // division by 2 allows us to cancel out with a 2 in the derivative for optimisation
    fetch::math::Divide(output(0, 0), static_cast<DataType>(2), output(0, 0));
  }

  /**
   * Gradients for Mean Square Error Loss would usually be of the form:
   * grad[0] = 2 * err * (in[0] - in[1]) / data_size
   * grad[1] = -2 * err * (in[0] - in[1]) / data_size
   *
   * However we make a few alterations:
   * 1. we ignore the gradient for the ground truth (i.e. grad[1]),
   * 2. we drop the 2 since we divide by 2 in forward pass,
   * 3. we must incorporate the weightings,
   *
   * so the modified gradient is computed as:
   * grad[0] = (err * (in[0] - in[1])  * weighting) / data_size
   * grad[1] = grad[0] -- SHOULD NOT BE USED
   *
   * @param inputs vector of input_tensor and ground_truth tensor (order is important)
   * @param error_signal error_signal passed back from next layer - since there should not be a next
   * layer the calling function is required to set this to a tensor of size 1 and value 1
   * @return
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());

    ArrayType return_signal(inputs.front()->shape());

    SizeType data_size = inputs.at(0)->shape(inputs.at(0)->shape().size() - 1);
    auto     count     = static_cast<DataType>(data_size);

    // backprop update rule varies depending on shape of weightings
    auto a_it = inputs.at(0)->cbegin();
    auto b_it = inputs.at(1)->cbegin();
    auto r_it = return_signal.begin();

    // no weighting
    if (weightings_.size() == static_cast<SizeType>(0))
    {
      while (r_it.is_valid())
      {
        *r_it = ((*a_it) - (*b_it)) / count;
        ++a_it;
        ++b_it;
        ++r_it;
      }
    }
    else
    {
      // weighting is scalar
      if (weightings_.shape().size() == static_cast<SizeType>(1))
      {
        auto weight_over_count = weightings_(0) / count;
        while (r_it.is_valid())
        {
          *r_it = ((*a_it) - (*b_it)) * weight_over_count;
          ++a_it;
          ++b_it;
          ++r_it;
        }
      }
      // weighting tensor is same shape as input (one weight for every parameter)
      else if (weightings_.shape() == inputs.at(0)->shape())
      {
        auto w_it = weightings_.cbegin();
        while (r_it.is_valid())
        {
          *r_it = (((*a_it) - (*b_it)) * (*w_it)) / (count);

          ++a_it;
          ++b_it;
          ++r_it;
          ++w_it;
        }
      }
      // weighting is a batch_size vector (one weight per data point)
      else if (weightings_.shape() == std::vector<SizeType>{data_size})
      {
        auto     w_it       = weightings_.cbegin();
        SizeType data_count = 0;
        SizeType data_stride;
        fetch::math::Divide(inputs.at(0)->size(), weightings_.size(), data_stride);
        while (r_it.is_valid())
        {
          *r_it = (((*a_it) - (*b_it)) * (*w_it)) / (count);
          ++a_it;
          ++b_it;
          ++r_it;

          // update weight value once per data point
          ++data_count;
          if (data_count == data_stride)
          {
            data_count = 0;
            ++w_it;
          }
        }
      }
      else
      {
        throw std::runtime_error("input or weightings_shape invalid");
      }
    }

    return {return_signal, return_signal};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    return {1, 1};
  }

  static constexpr char const *DESCRIPTOR = "MeanSquareErrorLoss";

private:
  ArrayType weightings_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
