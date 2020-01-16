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
 * A PlaceHolder is a DataHolder intended to store input data.
 * It has the following features:
 * 1. trainable: no
 * 2. mutable: yes, the data can be repeatedly overwritten
 * 3. shareable: no, shared layers should have their own placeholders
 * 4. saveable: no, the data is not stored upon serialisation
 * @tparam T
 */
template <class T>
class PlaceHolder : public DataHolder<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpPlaceholderSaveableParams<TensorType>;
  using MyType        = PlaceHolder<TensorType>;

  PlaceHolder() = default;

  explicit PlaceHolder(SPType const &sp);

  ~PlaceHolder() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_PLACEHOLDER;
  }

  static constexpr char const *DESCRIPTOR = "PlaceHolder";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
