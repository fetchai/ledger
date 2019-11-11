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
  {}

  ~Embeddings() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp   = std::make_shared<SPType>();
    auto w_sp = Weights<T>::GetOpSaveableParams();

    auto cast_sp = std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(sp);
    *cast_sp     = *(std::static_pointer_cast<OpWeightsSaveableParams<TensorType>>(w_sp));

    return sp;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(this->data_);
    assert(inputs.size() == 1);
    assert(inputs.front()->shape().size() == 2);

    SizeType batch_size = inputs.front()->shape().at(1);

    assert(output.shape().at(1) == inputs.front()->shape().at(0));
    assert(output.shape().at(0) == this->data_->shape().at(0));
    assert(output.shape().at(2) == batch_size);

    auto indices  = inputs.front()->shape().at(0);
    auto input_it = inputs.front()->begin();
    for (SizeType i{0}; i < indices; i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {
        auto output_view    = output.View({i, n});
        auto embedding_view = this->data_->View(static_cast<SizeType>(*input_it));
        output_view.Assign(embedding_view);
        ++input_it;
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

      auto indices  = inputs.front()->shape().at(0);
      auto input_it = inputs.front()->begin();

      for (SizeType i{0}; i < indices; i++)
      {
        for (SizeType n{0}; n < batch_size; n++)
        {
          auto error_view    = error_signal.View({i, n});
          auto gradient_view = this->gradient_accumulation_->View(static_cast<SizeType>(*input_it));

          auto error_view_it    = error_view.cbegin();
          auto gradient_view_it = gradient_view.begin();
          while (error_view_it.is_valid())
          {
            *gradient_view_it += *error_view_it;
            ++error_view_it;
            ++gradient_view_it;
          }
          ++input_it;
        }
      }

      this->reset_gradients_ = true;
    }

    return {TensorType(error_signal.shape())};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    auto                  feature_size = this->data_->shape().at(0);
    std::vector<SizeType> output_shape{feature_size, inputs.front()->shape().at(0),
                                       inputs.front()->shape().at(1)};
    return output_shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_EMBEDDINGS;
  }
  static constexpr char const *DESCRIPTOR = "Embedding";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
