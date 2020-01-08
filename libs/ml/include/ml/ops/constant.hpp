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

#include "ml/ops/dataholder.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

/**
 * A Constant is a DataHolder intended to store an immutable value.
 * It has the following features:
 * 1. trainable: no
 * 2. mutable: no, the data can be only written once
 * 3. shareable: yes, shared layers can re-use constants
 * 4. saveable: yes, the data is stored upon serialisation
 * @tparam T
 */
template <class T>
class Constant : public DataHolder<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpConstantSaveableParams<TensorType>;
  using MyType        = Constant<TensorType>;

  Constant() = default;

  explicit Constant(SPType const &sp)
    : DataHolder<T>(sp)
  {
    if (sp.data)
    {
      this->data_ = std::make_shared<TensorType>(sp.data->Copy());
    }
  }

  ~Constant() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override;

  bool SetData(TensorType const &data) override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_CONSTANT;
  }

  static constexpr char const *DESCRIPTOR = "CONSTANT";

protected:
  bool data_set_once_ = false;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
