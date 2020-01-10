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

#include "math/base_types.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/variable.hpp"

namespace fetch {
namespace ml {

template <typename TensorType>
struct StateDict;

struct OpsSaveableParams;

namespace ops {

template <class TensorType>
class Variable;

template <class TensorType>
class Ops;

/**
 * enum for selecting which type of initialisation to use with weights
 */
enum class WeightsInitialisation
{
  ONES,
  ZEROS,
  XAVIER_GLOROT,
  XAVIER_FAN_IN,
  XAVIER_FAN_OUT,
  XAVIER_GLOROT_UNIFORM,
  XAVIER_FAN_IN_UNIFORM,
  XAVIER_FAN_OUT_UNIFORM
};

template <typename T>
class Weights : public fetch::ml::ops::Variable<T>
{
public:
  using TensorType     = T;
  using SizeType       = fetch::math::SizeType;
  using SizeSet        = std::unordered_set<SizeType>;
  using DataType       = typename TensorType::Type;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Variable<T>::VecTensorType;
  using SPType         = OpWeightsSaveableParams<TensorType>;
  using WeightsPtrType = typename std::shared_ptr<Weights<TensorType>>;

public:
  Weights() = default;

  explicit Weights(SPType const &sp);

  ~Weights() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override;

  fetch::ml::StateDict<T> StateDict() const override;

  void LoadStateDict(fetch::ml::StateDict<T> const &dict) override;

  static void Initialise(TensorType &array, uint64_t in_size, uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789);

  static void Initialise(TensorType &array, uint64_t data_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789);

  TensorType const &GetWeights() const override;

  void SetWeights(TensorType const &new_value) override;

  std::pair<TensorType const, SizeSet const> GetSparseGradientsReferences() const override;

  TensorType const &GetGradientsReferences() const override;

  SizeSet const &GetUpdatedRowsReferences() const override;

  TensorType GetGradients() const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_WEIGHTS;
  }

  static constexpr char const *DESCRIPTOR = "Weights";

private:
  static void XavierInitialisation(TensorType &array, DataType normalising_factor,
                                   SizeType seed = 123456789);

  static void XavierInitialisationUniform(TensorType &array, DataType normalising_factor,
                                          SizeType seed = 123456789);

  static const DataType HALF;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
