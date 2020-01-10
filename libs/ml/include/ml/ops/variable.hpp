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
#include "ml/regularisers/reg_types.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
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

  explicit Variable(SPType const &sp);

  ~Variable() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  void AddToGradient(TensorType const &extern_grad);

  void AddToGradient(TensorType const &extern_grad, SizeSet const &rows_updated);

  void AddToGradient(TensorType const &extern_grad, SizeVector const &rows_updated);

  bool SetData(TensorType const &data) override;

  void ApplySparseGradient(TensorType const &grad, SizeSet &update_rows) override;

  void ApplyGradient(TensorType const &grad) override;

  void ResetGradients() override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_VARIABLE;
  }

  static constexpr char const *DESCRIPTOR = "Variable";

protected:
  bool          reset_gradients_ = false;
  TensorPtrType gradient_accumulation_;
  SizeSet       updated_rows_;
  //  RegularisationType regularisation_type = RegularisationType::NONE;
  DataType regularisation_rate = fetch::math::numeric_max<DataType>();

  void ApplyRegularisation() override;
};
}  // namespace ops
}  // namespace ml
}  // namespace fetch
