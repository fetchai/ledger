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

#include "math/exceptions/exceptions.hpp"
#include "math/metrics/mean_square_error.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareErrorLoss : public Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpMeanSquareErrorSaveableParams<T>;
  using MyType        = MeanSquareErrorLoss<TensorType>;

  explicit MeanSquareErrorLoss(SPType const &sp)
    : Ops<T>(sp)
    , weightings_(sp.weightings)
  {}

  explicit MeanSquareErrorLoss(TensorType weightings = TensorType())
    : weightings_(std::move(weightings))
  {}

  ~MeanSquareErrorLoss() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret = std::make_shared<SPType>();

    ret->weightings = weightings_;
    return ret;
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType
    copyshare->weightings_ = weightings_.Copy();

    return copyshare;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());

    if (weightings_.size() == static_cast<SizeType>(0))
    {
      *(output.begin()) = fetch::math::MeanSquareError(*(inputs.at(0)), *(inputs.at(1)));
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
      fetch::math::Divide(output(0, 0), static_cast<DataType>(inputs.at(0)->size()), output(0, 0));
    }
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
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());

    TensorType return_signal(inputs.front()->shape());

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
        throw math::exceptions::WrongShape("input or weightings_shape invalid");
      }
    }

    fetch::math::Multiply(return_signal, static_cast<DataType>(2), return_signal);

    return {return_signal, return_signal};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    return {1, 1};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LOSS_MEAN_SQUARE_ERROR;
  }
  static constexpr char const *DESCRIPTOR = "MeanSquareErrorLoss";

private:
  TensorType weightings_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
