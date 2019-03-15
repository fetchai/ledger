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
    this->SetData(ArrayType(std::vector<SizeType>({dataPoints, dimensions})));
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
          this->output_->Slice(typename ArrayType::SizeType(i)));
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
      this->gradientAccumulation_->Slice(typename ArrayType::SizeType(i))
          .Copy(errorSignal.Slice(j));
      j++;
    }
    return {};
  }

private:
  ArrayPtrType embeddings_output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
