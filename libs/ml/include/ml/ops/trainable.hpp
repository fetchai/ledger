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
#include "math/tensor/tensor.hpp"
#include "ml/ops/ops.hpp"
#include "ml/regularisers/reg_types.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/state_dict.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

/**
 * Provide an interface for any trainable ops
 * @tparam T passes tensors to graph during update step
 */
template <class T>
class Trainable
{
public:
  using TensorType   = T;
  using ArrayPtrType = std::shared_ptr<TensorType>;
  using SizeSet      = std::unordered_set<fetch::math::SizeType>;
  using DataType     = typename TensorType::Type;
  using RegPtrType   = std::shared_ptr<fetch::ml::regularisers::Regulariser<T>>;

  virtual fetch::ml::StateDict<T> StateDict() const                                        = 0;
  virtual void                    LoadStateDict(fetch::ml::StateDict<T> const &dict)       = 0;
  virtual TensorType const &      GetWeights() const                                       = 0;
  virtual void                    SetWeights(TensorType const &new_value)                  = 0;
  virtual TensorType const &      GetGradientsReferences() const                           = 0;
  virtual SizeSet const &         GetUpdatedRowsReferences() const                         = 0;
  virtual TensorType              GetGradients() const                                     = 0;
  virtual std::pair<TensorType const, SizeSet const> GetSparseGradientsReferences() const  = 0;
  virtual void                                       ResetGradients()                      = 0;
  virtual void                                       ApplyGradient(TensorType const &grad) = 0;
  virtual void ApplySparseGradient(TensorType const &grad, SizeSet &update_rows)           = 0;
  virtual void ApplyRegularisation()                                                       = 0;

  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0})
  {
    regulariser_         = regulariser;
    regularisation_rate_ = regularisation_rate;
  }

  /**
   * Enable or disable trainable gradient update freezing
   * @param new_frozen_state
   */
  void SetFrozenState(bool new_frozen_state)
  {
    value_frozen_ = new_frozen_state;
  }

  bool GetFrozenState() const
  {
    return value_frozen_;
  }

protected:
  RegPtrType regulariser_;
  DataType   regularisation_rate_ = DataType{0};
  bool       value_frozen_        = false;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
