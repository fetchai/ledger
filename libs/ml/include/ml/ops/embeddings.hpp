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

  Embeddings(SizeType dimensions, SizeType data_points)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dimensions, data_points}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dimensions, data_points);
    this->SetData(weights);
  }

  Embeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~Embeddings() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    if (!this->embeddings_output_ ||
        this->embeddings_output_->shape().at(1) != inputs.front().get().shape().at(0) ||
        this->embeddings_output_->shape().at(0) != this->output_->shape().at(0))
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(std::vector<SizeType>(
          {this->output_->shape().at(0), inputs.front().get().shape(0), batch_size}));
    }

    ArrayType transposed_input = inputs.front().get().Transpose();
    auto      e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        trailing_indices1.at(0) = i;
        trailing_indices1.at(1) = n;
        auto embedding_slice    = this->embeddings_output_->View(trailing_indices1);
        trailing_indices2.at(0) = static_cast<SizeType>(*e_it);
        auto output_slice       = this->output_->View(trailing_indices2);

        embedding_slice.Assign(output_slice);
        ++e_it;
      }
    }

    output = *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    ArrayType transposed_input = inputs.front().get().Transpose();
    auto      e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {

        trailing_indices1.at(0) = i;
        trailing_indices1.at(1) = n;
        auto error_slice        = error_signal.View(trailing_indices1);
        trailing_indices2.at(0) = static_cast<SizeType>(*e_it);
        auto gradient_slice     = this->gradient_accumulation_->View(trailing_indices2);

        auto error_slice_it    = error_slice.cbegin();
        auto gradient_slice_it = gradient_slice.begin();
        while (error_slice_it.is_valid())
        {
          *gradient_slice_it += *error_slice_it;
          ++error_slice_it;
          ++gradient_slice_it;
        }
        ++e_it;
      }
    }

    return {ArrayType(error_signal.shape())};
  }

  virtual void Step(typename T::Type learning_rate)
  {
    for (auto const &r : updated_rows_)
    {
      // get the relevant slice from gradients and embeddings
      auto grad_slice = this->gradient_accumulation_->Slice(r, 1);
      auto out_slice  = this->output_->Slice(r, 1);

      auto out_it  = out_slice.begin();
      auto grad_it = grad_slice.begin();

      while (out_it.is_valid())
      {
        *out_it  = *out_it - (*grad_it * learning_rate);
        *grad_it = 0;
        ++out_it;
        ++grad_it;
      }
    }
    updated_rows_.clear();
  }

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
  std::vector<SizeType>                  trailing_indices1 = {0, 0};
  std::vector<SizeType>                  trailing_indices2 = {0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
