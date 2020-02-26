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

#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/variable.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/utilities/sparse_tensor_utilities.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Variable<TensorType>::Variable(SPType const &sp)
  : DataHolder<TensorType>(sp)
{
  if (sp.gradient_accumulation)
  {
    gradient_accumulation_ = std::make_shared<TensorType>(sp.gradient_accumulation->Copy());
  }

  this->SetRegularisation(fetch::ml::details::CreateRegulariser<TensorType>(sp.regularisation_type),
                          sp.regularisation_rate);

  this->value_frozen_ = sp.value_frozen;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Variable<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();

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

  // Add base class savable params
  auto ops_sp  = ParentClass::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<typename ParentClass::SPType>(sp);
  *cast_sp     = *(std::static_pointer_cast<typename ParentClass::SPType>(ops_sp));

  return sp;
}

/**
 * shares the variable
 * @param me
 * @return
 */
template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Variable<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  assert(me.get() == this);
  return me;
}

/**
 * Responsibility:
 * When overloaded it needs to reset gradient flag if not frozen
 * @param inputs
 * @param error_signal
 * @return
 */
template <typename TensorType>
std::vector<TensorType> Variable<TensorType>::Backward(VecTensorType const &inputs,
                                                       TensorType const &   error_signal)
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

template <typename TensorType>
void Variable<TensorType>::AddToGradient(TensorType const &extern_grad)
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
template <class TensorType>
void Variable<TensorType>::AddToGradient(TensorType const &extern_grad, SizeSet const &rows_updated)
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
template <class TensorType>
void Variable<TensorType>::AddToGradient(TensorType const &extern_grad,
                                         SizeVector const &rows_updated)
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
template <class TensorType>
bool Variable<TensorType>::SetData(TensorType const &data)
{
  bool shape_changed = DataHolder<TensorType>::SetData(data);
  if (shape_changed)
  {
    gradient_accumulation_->Reshape(this->data_->shape());
    reset_gradients_ = true;
    return true;
  }
  return false;
}

/**
 * Function for applying gradient for specific rows only
 * @param grad
 * @param update_rows
 */
template <class TensorType>
void Variable<TensorType>::ApplySparseGradient(TensorType const &grad, SizeSet &update_rows)
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

template <typename TensorType>
void Variable<TensorType>::ApplyGradient(TensorType const &grad)
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
template <typename TensorType>
void Variable<TensorType>::ResetGradients()
{
  if (reset_gradients_)
  {
    gradient_accumulation_->Fill(DataType{0});
    reset_gradients_ = false;

    // Clear updates
    updated_rows_.clear();
  }
}

template <typename TensorType>
void Variable<TensorType>::ApplyRegularisation()
{
  if (this->regulariser_)
  {
    this->regulariser_->ApplyRegularisation(*this->data_, this->regularisation_rate_);
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Variable<math::Tensor<int8_t>>;
template class Variable<math::Tensor<int16_t>>;
template class Variable<math::Tensor<int32_t>>;
template class Variable<math::Tensor<int64_t>>;
template class Variable<math::Tensor<float>>;
template class Variable<math::Tensor<double>>;
template class Variable<math::Tensor<fixed_point::fp32_t>>;
template class Variable<math::Tensor<fixed_point::fp64_t>>;
template class Variable<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
