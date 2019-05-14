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

#include "ml/ops/weights.hpp"
#include <set>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class AveragedEmbeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;

  AveragedEmbeddings(SizeType dataPoints, SizeType dimensions)
  {
    ArrayType weights = ArrayType({dataPoints, dimensions});
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  AveragedEmbeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~AveragedEmbeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    uint64_t valid_samples(0);
    bool     clear = true;
    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        if (clear)
        {
          output.Assign(this->output_->Slice(typename ArrayType::SizeType(i)));
          clear = false;
        }
        else
        {
          auto slice = this->output_->Slice(typename ArrayType::SizeType(i));
          auto it1   = slice.begin();
          auto end   = slice.end();
          auto it2   = output.begin();
          while (it2.is_valid())
          {
            *it2 += *it1;
            ++it1;
            ++it2;
          }
        }
        valid_samples++;
      }
    }
    output.InlineDivide(DataType(valid_samples));

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);

    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        updated_rows_.insert(typename ArrayType::SizeType(double(i)));
        auto gradientAccumulationSlice =
            this->gradient_accumulation_->Slice(typename ArrayType::SizeType(i));
        auto it1 = gradientAccumulationSlice.begin();
        auto end = gradientAccumulationSlice.end();
        auto it2 = error_signal.begin();
        while (it1 != end)
        {
          *it1 += *it2;
          ++it1;
          ++it2;
        }
      }
    }
    return {error_signal};
  }

  virtual void Step(typename T::Type learningRate)
  {
    for (auto const &r : updated_rows_)
    {
      auto gradientAccumulationSlice = this->gradient_accumulation_->Slice(r);
      auto outputSlice               = this->output_->Slice(r);
      auto it1                       = gradientAccumulationSlice.begin();
      auto end                       = gradientAccumulationSlice.end();
      auto it2                       = outputSlice.begin();
      while (it1 != end)
      {
        *it2 += (*it1 * learningRate);
        *it1 = 0;
        ++it1;
        ++it2;
      }
    }
    updated_rows_.clear();
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    (void)inputs;
    auto outputShape = this->output_->shape();
    outputShape[0]   = 1;
    return outputShape;
  }

private:
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
