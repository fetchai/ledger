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

#include "math/exceptions/exceptions.hpp"
#include "math/matrix_operations.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CategoricalAccuracy : public Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpCategoricalAccuracySaveableParams<T>;
  using MyType        = CategoricalAccuracy<TensorType>;

  explicit CategoricalAccuracy(SPType const &sp)
    : Ops<T>(sp)
    , weightings_(sp.weightings)
  {
    weights_sum_ = fetch::math::Sum(weightings_);
  }

  explicit CategoricalAccuracy(TensorType weightings = TensorType())
    : weightings_(std::move(weightings))
  {
    weights_sum_ = fetch::math::Sum(weightings_);
  }

  ~CategoricalAccuracy() override = default;

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
    copyshare->weightings_  = weightings_.Copy();
    copyshare->weights_sum_ = weights_sum_;

    return copyshare;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());

    auto num_correct = DataType{0};

    TensorType test_results = fetch::math::ArgMax(*inputs.at(0));
    TensorType ground_truth = fetch::math::ArgMax(*inputs.at(1));

    auto it1 = test_results.cbegin();
    auto it2 = ground_truth.cbegin();

    SizeType data_size = test_results.size();

    if (weightings_.size() == SizeType{0})
    {
      while (it1.is_valid())
      {
        if (*it1 == *it2)
        {
          ++num_correct;
        }
        ++it1;
        ++it2;
      }
      num_correct = fetch::math::Divide(num_correct, static_cast<DataType>(data_size));
    }
    // rescale according to weights
    // weighting is a batch_size vector (one weight per data point)
    else if (weightings_.shape() == std::vector<SizeType>{data_size})
    {
      auto w_it = weightings_.cbegin();
      while (it1.is_valid())
      {
        if (*it1 == *it2)
        {
          num_correct += *w_it;
        }
        ++it1;
        ++it2;
        ++w_it;
      }
      num_correct = fetch::math::Divide(num_correct, weights_sum_);
    }
    else
    {
      throw math::exceptions::WrongShape("input or weightings_ invalid");
    }

    // divide by number of elements
    output(0, 0) = num_correct;
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(error_signal);
    FETCH_UNUSED(inputs);

    throw fetch::ml::exceptions::NotImplemented();
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    return {1, 1};
  }

  static constexpr OpType OpCode()
  {
    return OpType::METRIC_CATEGORICAL_ACCURACY;
  }
  static constexpr char const *DESCRIPTOR = "Categorical Accuracy";

private:
  TensorType weightings_;
  DataType   weights_sum_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
