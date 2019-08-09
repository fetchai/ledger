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
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Weights<T>::VecTensorType;
  using SPType        = OpEmbeddingsSaveableParams<TensorType>;

  Embeddings(SizeType dimensions, SizeType data_points)
  {
    TensorType weights = TensorType(std::vector<SizeType>({dimensions, data_points}));
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, dimensions, data_points);
    this->SetData(weights);
  }

  explicit Embeddings(TensorType const &weights)
  {
    this->SetData(weights);
  }

  explicit Embeddings(SPType const &sp)
    : Weights<T>(sp)
  {
    embeddings_output_ = sp.embeddings_output;
    updated_rows_ =
        std::set<typename TensorType::SizeType>(sp.updated_rows.begin(), sp.updated_rows.begin());
    trailing_indices1_ = sp.trailing_indices1;
    trailing_indices2_ = sp.trailing_indices2;
  }

  ~Embeddings() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp   = std::make_shared<SPType>();
    auto w_sp = Weights<T>::GetOpSaveableParams();

    auto cast_sp = std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(sp);
    *cast_sp     = *(std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(w_sp));

    sp->embeddings_output = embeddings_output_;
    std::copy(updated_rows_.begin(), updated_rows_.end(), std::back_inserter(sp->updated_rows));
    sp->trailing_indices1 = trailing_indices1_;
    sp->trailing_indices2 = trailing_indices2_;

    return sp;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(inputs.front()->shape().size() == 2);

    SizeType batch_size = inputs.front()->shape().at(1);

    // test embeddings_output_ not null ptr
    if (!this->embeddings_output_)
    {
      this->embeddings_output_ = std::make_shared<TensorType>(std::vector<SizeType>(
          {this->output_->shape().at(0), inputs.front()->shape(0), batch_size}));
    }
    // test embeddings_output_ batch size has changed
    else if (this->embeddings_output_->shape().at(2) != batch_size)
    {
      this->embeddings_output_->Reshape({this->embeddings_output_->shape().at(0),
                                         this->embeddings_output_->shape().at(1), batch_size});
    }

    assert(this->embeddings_output_->shape().at(0) == this->output_->shape().at(0));
    assert(this->embeddings_output_->shape().at(1) == inputs.front()->shape().at(0));
    assert(this->embeddings_output_->shape().at(2) == batch_size);

    TensorType transposed_input = inputs.front()->Transpose();
    auto       e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front()->shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        trailing_indices1_.at(0) = i;
        trailing_indices1_.at(1) = n;
        auto embedding_view      = this->embeddings_output_->View(trailing_indices1_);
        trailing_indices2_.at(0) = static_cast<SizeType>(*e_it);
        auto output_view         = this->output_->View(trailing_indices2_);

        embedding_view.Assign(output_view);
        ++e_it;
      }
    }

    output = *this->embeddings_output_;
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape().size() == 2);

    SizeType batch_size = inputs.front()->shape(1);

    TensorType transposed_input = inputs.front()->Transpose();
    auto       e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front()->shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {

        trailing_indices1_.at(0) = i;
        trailing_indices1_.at(1) = n;
        auto error_view          = error_signal.View(trailing_indices1_);
        trailing_indices2_.at(0) = static_cast<SizeType>(*e_it);
        auto gradient_view       = this->gradient_accumulation_->View(trailing_indices2_);

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

    return {TensorType(error_signal.shape())};
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
    std::vector<SizeType> output_shape = {this->output_->shape().at(0), inputs.front()->shape(0),
                                          inputs.front()->shape(1)};
    return output_shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_EMBEDDINGS;
  }
  static constexpr char const *DESCRIPTOR = "Embedding";

private:
  ArrayPtrType                            embeddings_output_;
  std::set<typename TensorType::SizeType> updated_rows_;
  std::vector<SizeType>                   trailing_indices1_ = {0, 0};
  std::vector<SizeType>                   trailing_indices2_ = {0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
