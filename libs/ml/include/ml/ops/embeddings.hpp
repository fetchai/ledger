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

#include <cassert>
#include <memory>
#include <set>
#include <vector>

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
  using SPType        = WeightsSaveableParams<ArrayType>;

  Embeddings(SizeType dimensions, SizeType data_points)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dimensions, data_points}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dimensions, data_points);
    this->SetData(weights);
  }

  explicit Embeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  explicit Embeddings(SPType const &sp)
  {
    this->output_ = sp.output;
  }

  ~Embeddings() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    SPType tp{};
    tp.output     = this->output_;
    tp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(tp);
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape().at(1);

    // test embeddings_output_ not null ptr
    if (!this->embeddings_output_)
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(std::vector<SizeType>(
          {this->output_->shape().at(0), inputs.front().get().shape(0), batch_size}));
    }
    // test embeddings_output_ batch size has changed
    else if (this->embeddings_output_->shape().at(2) != batch_size)
    {
      this->embeddings_output_->Reshape({this->embeddings_output_->shape().at(0),
                                         this->embeddings_output_->shape().at(1), batch_size});
    }

    assert(this->embeddings_output_->shape().at(0) == this->output_->shape().at(0));
    assert(this->embeddings_output_->shape().at(1) == inputs.front().get().shape().at(0));
    assert(this->embeddings_output_->shape().at(2) == batch_size);

    ArrayType transposed_input = inputs.front().get().Transpose();
    auto      e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        trailing_indices1.at(0) = i;
        trailing_indices1.at(1) = n;
        auto embedding_view     = this->embeddings_output_->View(trailing_indices1);
        trailing_indices2.at(0) = static_cast<SizeType>(*e_it);
        auto output_view        = this->output_->View(trailing_indices2);

        embedding_view.Assign(output_view);
        ++e_it;
      }
    }

    output = *this->embeddings_output_;
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
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
        auto error_view         = error_signal.View(trailing_indices1);
        trailing_indices2.at(0) = static_cast<SizeType>(*e_it);
        auto gradient_view      = this->gradient_accumulation_->View(trailing_indices2);

        auto error_view_it    = error_view.cbegin();
        auto gradient_view_it = gradient_view.begin();
        while (error_view_it.is_valid())
        {
          *gradient_view_it += *error_view_it;
          ++error_view_it;
          ++gradient_view_it;
        }
        ++e_it;
      }
    }

    return {ArrayType(error_signal.shape())};
  }

  void Step(typename T::Type learning_rate) override
  {
    for (auto const &r : updated_rows_)
    {
      // get the relevant view from gradients and embeddings
      auto grad_view = this->gradient_accumulation_->View(r);
      auto out_view  = this->output_->View(r);

      auto out_it  = out_view.begin();
      auto grad_it = grad_view.begin();

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

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape = {
        this->output_->shape().at(0), inputs.front().get().shape(0), inputs.front().get().shape(1)};
    return output_shape;
  }

  static constexpr char const *DESCRIPTOR = "Embedding";

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
  std::vector<SizeType>                  trailing_indices1 = {0, 0};
  std::vector<SizeType>                  trailing_indices2 = {0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
