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

#include "ml/core/subgraph.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/weights.hpp"

namespace fetch {
namespace ml {

namespace layers {

template <class T>
class MultiheadAttention : public SubGraph<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using DataType      = typename T::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;

  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;
  using SPType          = fetch::ml::LayerMultiHeadSaveableParams<T>;

  MultiheadAttention() = default;

  MultiheadAttention(SizeType n_heads, SizeType model_dim,
                     DataType dropout = fetch::math::Type<DataType>("0.1"));

  std::string create_one_attention_head(std::string const &head_name, std::string const &query,
                                        std::string const &key, std::string const &value,
                                        std::string const &mask);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_MULTI_HEAD_ATTENTION;
  }

  static constexpr char const *DESCRIPTOR = "MultiheadAttention";

private:
  SizeType key_dim_{};
  SizeType value_dim_{};
  SizeType n_heads_{};
  SizeType model_dim_{};
  DataType dropout_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
