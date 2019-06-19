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
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Weights<T>::VecTensorType;

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

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    ASSERT(this->output_);
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    if (!this->embeddings_output_ ||
        this->embeddings_output_->shape()[0] != inputs.front().get().shape()[0] ||
        this->embeddings_output_->shape()[1] != this->output_->shape()[1])
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(std::vector<SizeType>(
          {inputs.front().get().shape(0), this->output_->shape()[1], batch_size}));
    }

    for (SizeType n{0}; n < batch_size; n++)
    {
      for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
      {

        SizeType e = static_cast<SizeType>(inputs.front().get().At(i, n));

        for (SizeType j{0}; j < this->embeddings_output_->shape().at(1); j++)
        {
          this->embeddings_output_->At(i, j, n) = this->output_->At(e, j);
        }
      }
    }

    output = *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    for (SizeType n{0}; n < batch_size; n++)
    {
      for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
      {
        SizeType e = static_cast<SizeType>(inputs.front().get().At(i, n));
        updated_rows_.insert(e);

        for (SizeType j{0}; j < this->gradient_accumulation_->shape().at(1); j++)
        {
          this->gradient_accumulation_->At(e, j) += error_signal.At(i, j, n);
        }
      }

      // std::cout<<"EMBINPUT"<< n <<":"<<std::endl<<inputs.front().get().ToString()<<std::endl;
      // std::cout<<this<<"-Gradient"<< n
      // <<":"<<std::endl<<this->gradient_accumulation_->ToString()<<std::endl;
    }

    using SizeType = typename ArrayType::SizeType;
    for (SizeType i{0}; i < error_signal.shape().at(2); i++)
    {
      // std::cout<<"EMBERR("<<i<<")"<<std::endl<<error_signal.Slice(i,2).Copy().Squeeze().ToString()<<std::endl;
    }

    return {ArrayType(error_signal.shape())};
  }

  virtual void Step(typename T::Type learning_rate)
  {
    ArrayType embedding_slice;

    for (auto const &r : updated_rows_)
    {
      // get the relevant slice from gradients and embeddings
      auto grad_slice = this->gradient_accumulation_->Slice(r);
      auto out_slice  = this->output_->Slice(r);

      embedding_slice = out_slice.Copy();

      // multiply accumulated gradients by learning rate, then subtract from current embeddings
      embedding_slice.InlineSubtract(grad_slice.Copy().InlineMultiply(learning_rate));

      // zero out gradients and assign new embeddings values
      grad_slice.Assign(ArrayType::Zeroes(embedding_slice.shape()));
      out_slice.Assign(embedding_slice);
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
