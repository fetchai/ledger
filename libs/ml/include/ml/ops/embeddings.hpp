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

#include <set>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Embeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;

  Embeddings(SizeType dataPoints, SizeType dimensions)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dataPoints, dimensions}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  Embeddings(SizeType dataPoints, SizeType dimensions, ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~Embeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    ASSERT(this->output_);
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 1);

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
      this->embeddings_output_->Slice(j).Copy(
          this->output_->Slice(typename ArrayType::SizeType(double(i))));
      j++;
    }
    return *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 1);

    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      updated_rows_.insert(typename ArrayType::SizeType(double(i)));
      this->gradient_accumulation_->Slice(typename ArrayType::SizeType(i))
          .Copy(errorSignal.Slice(j));
      j++;
    }
    return {ArrayType(errorSignal.shape())};
  }

  virtual void Step(typename T::Type learningRate)
  {
    std::cout << "updated_rows_.size(): " << updated_rows_.size() << std::endl;
//    std::cout << "updated_rows_.at(0): " << updated_rows_.at(0) << std::endl;
    for (auto const &r : updated_rows_)
    {
      std::cout << "r: " << r << std::endl;
      ArrayType gradient_accumulation_slice = this->gradient_accumulation_->Slice(r);
      std::cout << "gradient_accumulation_slice.ToString(): " << gradient_accumulation_slice.ToString() << std::endl;
      ArrayType output_slice               = this->output_->Slice(r);
      std::cout << "output_slice.ToString(): " << output_slice.ToString() << std::endl;
      gradient_accumulation_slice.InlineMultiply(-learningRate);
      output_slice.InlineAdd(gradient_accumulation_slice);
      gradient_accumulation_slice.Fill(typename T::Type(0));
    }
    updated_rows_.clear();
  }

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
