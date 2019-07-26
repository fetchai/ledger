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

#include "math/activation_functions/softmax.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Softmax : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit Softmax(SizeType axis = 0)
    : axis_(axis)
  {}
  ~Softmax() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax((*inputs.at(0)), output, axis_);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Copy();
    ArrayType t(error_signal.shape());
    this->Forward(inputs, t);
    return_signal.InlineMultiply(t);

    // 1D softmax
    if (inputs.front()->shape().size() == 1)
    {
      typename ArrayType::Type sum = return_signal.Sum();
      t.InlineMultiply(sum);
    }
    // 2D softmax
    else if (inputs.front()->shape().size() == 2)
    {
      ArrayType sum;
      sum = ReduceSum(return_signal, axis_);

      t.InlineMultiply(sum);
    }
    // N-D softmax
    else
    {
      if (axis_ == 0)
      {
        MultiplyBySumAxis0(t, return_signal);
      }
      else
      {
        MultiplyBySumAnyAxis(t, return_signal, axis_);
      }
    }

    return_signal.InlineSubtract(t);
    return {return_signal};
  }

  /**
   * Multiply each element of array1 by sum of values for each 1D slice of array2 in given axis
   * Implemented using Slices
   * @param array1
   * @param array2
   * @param axis
   */
  void MultiplyBySumAnyAxis(ArrayType &array1, ArrayType &array2, SizeType axis)
  {
    using DataType = typename ArrayType::Type;

    assert(array1.size() >= 2);
    assert(axis < array1.size());

    // Prepare Slice
    SizeType              reduced_size = array1.shape().size() - 1;
    std::vector<SizeType> offsets;
    offsets.resize(reduced_size);
    std::vector<SizeType> axes;
    axes.resize(reduced_size);

    SizeType j = 0;
    for (SizeType i = 0; i < reduced_size; i++)
    {
      if (j == axis)
      {
        j++;
      }

      axes.at(i)    = j;
      offsets.at(i) = 0;
      j++;
    }
    auto array1_slice = array1.Slice(offsets, axes);
    auto array2_slice = array2.Slice(offsets, axes);

    // Iterate over all possible offsets combinations
    DataType sum(0);
    bool     is_done = false;
    while (offsets.at(offsets.size() - 1) < array1.shape().at(axes.at(axes.size() - 1)))
    {
      // Prepare slice
      for (SizeType i = 0; i < offsets.size(); i++)
      {

        if (offsets.at(i) >= array1.shape().at(axes.at(i)))
        {
          if (i == offsets.size() - 1)
          {
            // Terminate counter otherwise it would overflow
            is_done = true;
            break;
          }

          offsets.at(i) = 0;
          array1_slice.ModifyRange(offsets.at(i), axes.at(i));
          array2_slice.ModifyRange(offsets.at(i), axes.at(i));

          offsets.at(i + 1)++;
        }
        else
        {
          // End of digit transmission
          array1_slice.ModifyRange(offsets.at(i), axes.at(i));
          array2_slice.ModifyRange(offsets.at(i), axes.at(i));
          break;
        }
      }
      if (is_done)
        break;

      // Do operation with slice

      // sum=sum(array2)
      sum      = static_cast<DataType>(0);
      auto it1 = array2_slice.cbegin();
      while (it1.is_valid())
      {
        sum += *it1;
        ++it1;
      }

      // array1=array1*sum(array2)
      auto it2 = array1_slice.begin();
      while (it2.is_valid())
      {
        *it2 = (*it2) * sum;
        ++it2;
      }

      // Next iteration step
      offsets.at(0)++;
    }
  }

  /**
   * Multiply each element of array1 by sum of values for each 1D slice of array2 along axis 0
   * Implementation using views
   * Implemented using Slices
   * @param array1
   * @param array2
   * @param axis
   */
  void MultiplyBySumAxis0(ArrayType &array1, ArrayType &array2)
  {
    using DataType = typename ArrayType::Type;

    assert(array1.size() >= 2);

    // Prepare Slice
    SizeType              reduced_size = array1.shape().size() - 1;
    std::vector<SizeType> offsets;
    offsets.resize(reduced_size);

    for (SizeType i = 0; i < reduced_size; i++)
    {
      offsets.at(i) = 0;
    }

    // Iterate over all possible offsets combinations
    DataType sum(0);
    bool     is_done = false;
    while (offsets.at(offsets.size() - 1) < array1.shape().at(array1.shape().size() - 1))
    {
      // Prepare slice
      for (SizeType i = 0; i < offsets.size(); i++)
      {

        if (offsets.at(i) >= array1.shape().at(i + 1))
        {
          if (i == offsets.size() - 1)
          {
            // Terminate counter otherwise it would overflow
            is_done = true;
            break;
          }

          offsets.at(i) = 0;
          offsets.at(i + 1)++;
        }
        else
        {
          // End of digit transmission
          break;
        }
      }
      if (is_done)
        break;

      // Do operation with slice
      auto array1_view = array1.View(offsets);
      auto array2_view = array2.View(offsets);

      // sum=sum(array2)
      sum      = static_cast<DataType>(0);
      auto it1 = array2_view.cbegin();
      while (it1.is_valid())
      {
        sum += *it1;
        ++it1;
      }

      // array1=array1*sum(array2)
      auto it2 = array1_view.begin();
      while (it2.is_valid())
      {
        *it2 = (*it2) * sum;
        ++it2;
      }

      // Next iteration step
      offsets.at(0)++;
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Softmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
