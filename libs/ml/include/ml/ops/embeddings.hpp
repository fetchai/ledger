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
class Embeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType      = T;
  using DataType       = typename ArrayType::Type;
  using ArrayPtrType   = std::shared_ptr<ArrayType>;
  using SizeType       = typename ArrayType::SizeType;
  using ConstSliceType = typename ArrayType::ConstSliceType;

  Embeddings(SizeType dataPoints, SizeType dimensions)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dataPoints, dimensions}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  Embeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~Embeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    ASSERT(this->output_);
    ASSERT(inputs.size() == 1);
    ASSERT(
        (inputs.front().get().shape().size() == 1) ||
        ((inputs.front().get().shape().size() == 2) && (inputs.front().get().shape().at(1) == 1)));

    if (!this->embeddings_output_ ||
        this->embeddings_output_->shape()[0] != inputs.front().get().size() ||
        this->embeddings_output_->shape()[1] != this->output_->shape()[1])
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(
          std::vector<SizeType>({inputs.front().get().size(), this->output_->shape()[1]}));
    }
    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      auto tmp  = this->embeddings_output_->Slice(j);
      auto tmp2 = this->output_->Slice(typename ArrayType::SizeType(i));
      tmp.Assign(tmp2);
      j++;
    }
    return *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(
        (inputs.front().get().shape().size() == 1) ||
        ((inputs.front().get().shape().size() == 2) && (inputs.front().get().shape().at(1) == 1)));

    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      updated_rows_.insert(typename ArrayType::SizeType(double(i)));
      this->gradient_accumulation_->Slice(typename ArrayType::SizeType(double(i)))
          .Assign(errorSignal.Slice(j));
      j++;
    }
    return {ArrayType(errorSignal.shape())};
  }

//  virtual void Step(typename T::Type learningRate)
//  {
//    for (auto const &r : updated_rows_)
//    {
//      auto gradient_accumulation_slice = this->gradient_accumulation_->Slice(r).Tensor();
//      auto output_slice                = this->output_->Slice(r).Tensor();
//
//      gradient_accumulation_slice.InlineMultiply(-learningRate);
//      output_slice.InlineAdd(gradient_accumulation_slice);
//      gradient_accumulation_slice.Fill(typename T::Type(0));
//    }
//    updated_rows_.clear();
//  }
  virtual void Step(typename T::Type learningRate)
  {
    if (updated_rows_.size() > 0)
    {
      ArrayType gradient_accumulation_slice{this->gradient_accumulation_->Slice(0).Copy()};
      ArrayType output_slice{this->output_->Slice(0).Copy()};

      for (auto const &r : updated_rows_)
      {
        gradient_accumulation_slice = this->gradient_accumulation_->Slice(r).Copy();
        output_slice = this->output_->Slice(r).Copy();

        gradient_accumulation_slice.InlineMultiply(-learningRate);
        output_slice.InlineAdd(gradient_accumulation_slice);

        this->gradient_accumulation_->Slice(r).Assign(gradient_accumulation_slice);
        this->output_->Slice(r).Assign(output_slice);
      }
      updated_rows_.clear();
    }
  }

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
