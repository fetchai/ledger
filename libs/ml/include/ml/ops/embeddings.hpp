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
  using SizeType      = fetch::math::SizeType;
  using SizeSet       = std::unordered_set<SizeType>;
  using VecTensorType = typename Weights<T>::VecTensorType;
  using SPType        = OpEmbeddingsSaveableParams<TensorType>;
  using MyType        = Embeddings<TensorType>;

  Embeddings(SizeType dimensions, SizeType data_points)
  {
    TensorType weights = TensorType(std::vector<SizeType>({dimensions, data_points}));
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, dimensions, data_points);
    Weights<T>::SetData(weights);
  }

  explicit Embeddings(TensorType const &weights)
  {
    Weights<T>::SetData(weights);
  }

  explicit Embeddings(SPType const &sp)
    : Weights<T>(sp)
  {
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

    sp->trailing_indices1 = trailing_indices1_;
    sp->trailing_indices2 = trailing_indices2_;

    return sp;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(this->data_);
    assert(inputs.size() == 1);
    assert(inputs.front()->shape().size() == 2);

    SizeType batch_size = inputs.front()->shape().at(1);

    assert(output.shape().at(0) == this->data_->shape().at(0));
    assert(output.shape().at(1) == inputs.front()->shape().at(0));
    assert(output.shape().at(2) == batch_size);

    TensorType transposed_input = inputs.front()->Transpose();
    auto       e_it             = transposed_input.begin();

    for (SizeType i{0}; i < inputs.front()->shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        trailing_indices1_.at(0) = i;
        trailing_indices1_.at(1) = n;
        auto embedding_view      = output.View(trailing_indices1_);
        trailing_indices2_.at(0) = static_cast<SizeType>(*e_it);
        auto output_view         = this->data_->View(trailing_indices2_);

        embedding_view.Assign(output_view);
        ++e_it;
      }
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape().size() == 2);

    if (!this->value_frozen_)
    {
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

          // Mark update
          this->updated_rows_.insert(trailing_indices2_.at(0));

          auto gradient_view = this->gradient_accumulation_->View(trailing_indices2_);

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
      this->reset_gradients_ = true;
    }

    return {TensorType(error_signal.shape())};
  }

  void ApplySparseGradient(TensorType const &grad, SizeSet &update_rows) override
  {
    if (!this->value_frozen_)
    {
      this->ApplyRegularisation();

      // Normal apply
      if (update_rows.empty() ||
          update_rows.size() > (this->gradient_accumulation_->shape().at(1) / 4))
      {
        this->data_->InlineAdd(grad);
      }
      // Sparse apply
      else
      {
        memory::Range range(0, std::size_t(this->data_->height()));

        for (SizeType update_index : update_rows)
        {
          auto data_slice = this->data_->data().slice(this->data_->padded_height() * update_index,
                                                      this->data_->padded_height());
          auto gradient_slice =
              grad.data().slice(grad.padded_height() * update_index, grad.padded_height());

          data_slice.in_parallel().RangedApplyMultiple(
              range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, data_slice,
              gradient_slice);
        }
      }

      this->ResetGradients();
    }
  }

  void ApplyGradient(TensorType const &grad) override
  {
    if (!this->value_frozen_)
    {
      this->ApplyRegularisation();
      this->data_->InlineAdd(grad);
      this->ResetGradients();
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape = {this->data_->shape().at(0), inputs.front()->shape().at(0),
                                          inputs.front()->shape().at(1)};
    return output_shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_EMBEDDINGS;
  }
  static constexpr char const *DESCRIPTOR = "Embedding";

private:
  std::vector<SizeType> trailing_indices1_ = {0, 0};
  std::vector<SizeType> trailing_indices2_ = {0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
