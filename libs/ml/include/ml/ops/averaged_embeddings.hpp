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

#include "core/assert.hpp"
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
    ArrayType weights = ArrayType(std::vector<SizeType>({dataPoints, dimensions}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  virtual ~AveragedEmbeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(this->output_);
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 1);
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));

    auto shape = this->output_->shape();
    shape[0]   = 1;
    if (!this->embeddings_output_ || this->embeddings_output_->shape() != shape)
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(shape);
    }
    uint64_t valid_samples(0);
    this->embeddings_output_->Fill(0);
    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        auto it1 = this->embeddings_output_->begin();
        auto it2 = this->output_->Slice(typename ArrayType::SizeType(i)).begin();
        while (it1.is_valid())
        {
          *it1 += *it2;
          ++it1;
          ++it2;
        }
        valid_samples++;
      }
    }
    this->embeddings_output_->InlineDivide(DataType(valid_samples));
    return *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 1);

    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        updated_rows_.insert(typename ArrayType::SizeType(double(i)));

        auto it1 = this->gradient_accumulation_->Slice(typename ArrayType::SizeType(i)).begin();
        auto it2 = errorSignal.begin();
        while (it1.is_valid())
        {
          *it1 += *it2;
          ++it1;
          ++it2;
        }
      }
    }
    return {ArrayType(errorSignal.shape())};
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
      std::vector<std::reference_wrapper<ArrayType const>> const & /*inputs*/) const
  {
    std::vector<typename ArrayType::SizeType> outputShape = this->output_->shape();
    ;
    outputShape[0] = 1;
    return outputShape;
  }

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
