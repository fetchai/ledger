#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "ml/ops/dataholder.hpp"
#include "ml/ops/trainable.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/utilities/sparse_tensor_utilities.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {

template <typename TensorType>
struct OpVariableSaveableParams : public OpDataHolderSaveableParams<TensorType>
{
  using DataType                      = typename TensorType::Type;
  fetch::ml::OpType           op_type = OpType::OP_PLACEHOLDER;
  std::shared_ptr<TensorType> data;
  std::shared_ptr<TensorType> gradient_accumulation;
  RegularisationType          regularisation_type = RegularisationType::NONE;
  DataType                    regularisation_rate = fetch::math::numeric_max<DataType>();
  bool                        value_frozen;
};

namespace ops {

/**
 * A Variable is a DataHolder intended to store trainable data; for example weights.
 * It has the following features:
 * 1. trainable: yes
 * 2. mutable: yes, the data can be repeatedly overwritten
 * 3. shareable: yes, shared layers should be able to reuse variables
 * 4. saveable: yes, the data is stored upon serialisation
 * @tparam T
 */
template <class T>
class Variable : public DataHolder<T>, public Trainable<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using SizeSet       = std::unordered_set<SizeType>;
  using SizeVector    = std::vector<SizeType>;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpVariableSaveableParams<TensorType>;
  using MyType        = Variable<TensorType>;

  Variable() = default;

  explicit Variable(SPType const &sp)
    : DataHolder<T>(sp)
  {
    if (sp.data)
    {
      this->data_ = std::make_shared<TensorType>(sp.data->Copy());
    }

    if (sp.gradient_accumulation)
    {
      gradient_accumulation_ = std::make_shared<TensorType>(sp.gradient_accumulation->Copy());
    }

    this->SetRegularisation(
        fetch::ml::details::CreateRegulariser<TensorType>(sp.regularisation_type),
        sp.regularisation_rate);

    this->value_frozen_ = sp.value_frozen;
  }

  ~Variable() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();
    if (this->data_)
    {
      sp->data = std::make_shared<TensorType>(this->data_->Copy());
    }

    if (gradient_accumulation_)
    {
      sp->gradient_accumulation = std::make_shared<TensorType>(gradient_accumulation_->Copy());
    }

    if (this->regulariser_)
    {
      sp->regularisation_type = this->regulariser_->reg_type;
    }
    else
    {
      sp->regularisation_type = RegularisationType::NONE;
    }

    sp->regularisation_rate = this->regularisation_rate_;
    sp->value_frozen        = this->value_frozen_;

    return sp;
  }

  /**
   * Responsibility:
   * When overloaded it needs to reset gradient flag if not frozen
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());

    if (!this->value_frozen_)
    {
      gradient_accumulation_->InlineAdd(error_signal);
      reset_gradients_ = true;
    }
    return {error_signal};
  }

  void AddToGradient(TensorType const &extern_grad)
  {
    if (!this->value_frozen_)
    {
      // Make sure that all rows will get updated
      if (!updated_rows_.empty())
      {
        updated_rows_.clear();
      }

      gradient_accumulation_->InlineAdd(extern_grad);
      reset_gradients_ = true;
    }
  }

  /**
   * Add external gradient for specified rows from update_rows set to gradient gradient accumulation
   * @param grad TensorType gradient
   * @param update_rows SizeSet
   */
  void AddToGradient(TensorType const &extern_grad, SizeSet const &rows_updated)
  {
    if (!this->value_frozen_)
    {
      // Handling of empty set of updates for non-v2w cases
      if (rows_updated.empty())
      {
        AddToGradient(extern_grad);
        return;
      }

      if (!rows_updated.empty() && this->data_->shape().size() != 2)
      {
        throw fetch::ml::exceptions::InvalidMode("Sparse gradient supported for 2D tensors only.");
      }

      // Add external information about row updates
      this->updated_rows_.insert(rows_updated.begin(), rows_updated.end());

      // Add gradient only to updated rows
      utilities::SparseAdd(extern_grad, *this->gradient_accumulation_, rows_updated);
      this->reset_gradients_ = true;
    }
  }

  /**
   * Add external gradient for specified rows from update_rows vector to gradient accumulation
   * This function is used for translated external sparse gradient updates for distributed
   * w2v learning
   * Because we can't keep order of elements in set after translating w2v embeddings update we need
   * to use vectors instead.
   *
   * @param extern_grad
   * @param rows_updated stored as vector
   */
  void AddToGradient(TensorType const &extern_grad, SizeVector const &rows_updated)
  {

    if (!this->value_frozen_)
    {
      // Handling of empty vector of updates for non-v2w cases
      if (rows_updated.empty())
      {
        AddToGradient(extern_grad);
        return;
      }

      if (!rows_updated.empty() && this->data_->shape().size() != 2)
      {
        throw fetch::ml::exceptions::InvalidMode("Sparse gradient supported for 2D tensors only.");
      }

      // Add external information about row updates
      for (auto row : rows_updated)
      {
        if (row == fetch::math::numeric_max<SizeType>())
        {
          // Skip unknown word row
          continue;
        }
        // Add external information about row updates
        this->updated_rows_.insert(row);
      }

      // Add gradient only to updated rows
      utilities::SparseAdd(extern_grad, *this->gradient_accumulation_, rows_updated);
      this->reset_gradients_ = true;
    }
  }

  /**
   * Sets the internally stored data, and ensures the correct shape for
   * gradient accumulation
   * @param data
   * @return
   */
  bool SetData(TensorType const &data) override
  {
    bool shape_changed = DataHolder<T>::SetData(data);
    if (shape_changed)
    {
      gradient_accumulation_ = std::make_shared<TensorType>(this->data_->shape());
      reset_gradients_       = true;
      return true;
    }
    return false;
  }

  /**
   * Function for applying gradient for specific rows only
   * @param grad
   * @param update_rows
   */
  void ApplySparseGradient(TensorType const &grad, SizeSet &update_rows) override
  {
    // skip frozen trainables
    if (!this->value_frozen_)
    {

      if (!update_rows.empty() && this->data_->shape().size() != 2)
      {
        throw fetch::ml::exceptions::InvalidMode("Sparse gradient not supported.");
      }

      // Apply gradient only to updated rows
      utilities::SparseAdd(grad, *this->data_, update_rows);
      this->ResetGradients();
    }
  }

  void ApplyGradient(TensorType const &grad) override
  {
    if (!this->value_frozen_)
    {
      ApplyRegularisation();
      this->data_->InlineAdd(grad);
      ResetGradients();
    }
  }

  /**
   * Set all gradient values to 0 and clear updated rows set
   */
  void ResetGradients() override
  {
    if (reset_gradients_)
    {
      gradient_accumulation_->Fill(typename T::Type(0));
      reset_gradients_ = false;

      // Clear updates
      updated_rows_.clear();
    }
  }

  /**
   * shares the variable
   * @param me
   * @return
   */
  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override
  {
    assert(me.get() == this);
    return me;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_VARIABLE;
  }

  static constexpr char const *DESCRIPTOR = "Variable";

protected:
  bool               reset_gradients_ = false;
  TensorPtrType      gradient_accumulation_;
  SizeSet            updated_rows_;
  RegularisationType regularisation_type = RegularisationType::NONE;
  DataType           regularisation_rate = fetch::math::numeric_max<DataType>();

  void ApplyRegularisation() override
  {
    if (this->regulariser_)
    {
      this->regulariser_->ApplyRegularisation(*this->data_, this->regularisation_rate_);
    }
  }
};
}  // namespace ops
}  // namespace ml
}  // namespace fetch
