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
#include "ml/ops/dataholder.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

// TODO         - Constants, Variables, Placeholders
// serial           - save,     save,      do not save
// trainable        - no,      yes,        no
// data mutable     - no,       yes,        yes
// shareable?       - yes,      yes,       no

template <class T>
class Variable : public DataHolder<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using TensorPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpVariableSaveableParams<TensorType>;
  using MyType        = Variable<TensorType>;

  Variable() = default;

  explicit Variable(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Variable() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    return std::make_shared<SPType>();
  }

  /**
   * abstract method, variable sharing should be implemented by concrete Op
   * @param me
   * @return
   */
  virtual std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override = 0;

  /**
   * forward recovers the stored data
   * @param inputs
   * @param output
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    assert(data_);
    output = *(data_);
  }

  /**
   * backward simply passes back the error signal
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    return {error_signal};
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_VARIABLE;
  }

  static constexpr char const *DESCRIPTOR = "Variable";

protected:
  TensorPtrType data_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
