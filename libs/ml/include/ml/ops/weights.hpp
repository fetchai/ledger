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

#include "core/random/lfg.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/ops/variable.hpp"
#include "ml/state_dict.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

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

template <class T>
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

  explicit Weights(SPType const &sp)
    : Variable<T>(sp)
  {}

  ~Weights() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp   = std::make_shared<SPType>();
    auto p_sp = Variable<T>::GetOpSaveableParams();

    auto cast_sp = std::static_pointer_cast<OpVariableSaveableParams<TensorType>>(sp);
    *cast_sp     = *(std::static_pointer_cast<OpVariableSaveableParams<TensorType>>(p_sp));

    return sp;
  }

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override
  {
    // This overrides implementation in Placeholder
    assert(me.get() == this);
    return me;
  }

  /**
   * constructs a state dictionary used for exporting/saving weights
   * @return
   */
  fetch::ml::StateDict<T> StateDict() const override
  {
    fetch::ml::StateDict<T> d;
    d.weights_ = this->data_;
    return d;
  }

  /**
   * load from a state dictionary to import weights
   * @param dict
   */
  void LoadStateDict(fetch::ml::StateDict<T> const &dict) override
  {
    assert(dict.dict_.empty());
    this->SetData(*dict.weights_);
  }

  /**
   * interface to call standard weights initialisation routines. defaults to xavier
   * @param mode  An enum indicating which type of initialisation to perform
   */
  static void Initialise(TensorType &array, uint64_t in_size, uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ZEROS:
    {
      array.Fill(DataType{0});
      break;
    }
    case WeightsInitialisation::ONES:
    {
      array.Fill(static_cast<DataType>(1));
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT:
    {
      XavierInitialisation(
          array,
          fetch::math::Sqrt(static_cast<DataType>(static_cast<DataType>(2) /
                                                  static_cast<DataType>(in_size + out_size))),
          seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_IN:
    {
      XavierInitialisation(array,
                           fetch::math::Sqrt(static_cast<DataType>(static_cast<DataType>(1) /
                                                                   static_cast<DataType>(in_size))),
                           seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_OUT:
    {
      XavierInitialisation(array,
                           fetch::math::Sqrt(static_cast<DataType>(
                               static_cast<DataType>(1) / static_cast<DataType>(out_size))),
                           seed);
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT_UNIFORM:
    {
      XavierInitialisationUniform(
          array,
          fetch::math::Sqrt(static_cast<DataType>(static_cast<DataType>(6) /
                                                  static_cast<DataType>(in_size + out_size))),
          seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_IN_UNIFORM:
    {
      XavierInitialisationUniform(array,
                                  fetch::math::Sqrt(static_cast<DataType>(
                                      static_cast<DataType>(3) / static_cast<DataType>(in_size))),
                                  seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_OUT_UNIFORM:
    {
      XavierInitialisationUniform(array,
                                  fetch::math::Sqrt(static_cast<DataType>(
                                      static_cast<DataType>(3) / static_cast<DataType>(out_size))),
                                  seed);
      break;
    }
    default:
      std::cerr << "unrecognised weights initialisation" << std::endl;
      throw;
    }
  }

  /**
   * interface to call standard weights initialisation routines. defaults to xavier.
   * Fan in and fan out xavier not permitted with input and output sizes not known independently
   * @param mode  An enum indicating which type of initialisation to perform
   */
  static void Initialise(TensorType &array, uint64_t data_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ONES:
    {
      array.Fill(static_cast<DataType>(1));
      break;
    }
    case WeightsInitialisation::ZEROS:
    {
      array.Fill(DataType{0});
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT:
    {
      XavierInitialisation(
          array, fetch::math::Sqrt(static_cast<DataType>(2) / static_cast<DataType>(data_size)),
          seed);
      break;
    }
    default:
      std::cerr << "unrecognised weights initialisation" << std::endl;
      throw;
    }
  }

  /**
   * exports the weight values Array
   * @return const reference to internal values Array
   */
  TensorType const &GetWeights() const override
  {
    return *this->data_;
  }

  void SetWeights(TensorType const &new_value) override
  {
    this->data_->Assign(new_value);
  }

  /**
   * exports the weight gradients Array
   * @return const reference to internal accumulated gradient Array and unordered set of indices
   * which were updated
   */
  std::pair<TensorType const, SizeSet const> GetSparseGradientsReferences() const override
  {
    return std::move(std::make_pair(*this->gradient_accumulation_, this->updated_rows_));
  }

  /**
   * exports the weight gradients Array
   * @return const reference to internal accumulated gradient Array
   */
  TensorType const &GetGradientsReferences() const override
  {
    return *this->gradient_accumulation_;
  }

  SizeSet const &GetUpdatedRowsReferences() const override
  {
    return this->updated_rows_;
  }

  /**
   * returns deep copy of the weight gradients Array
   * @return Internal accumulated gradient Array
   */
  TensorType GetGradients() const override
  {
    return this->gradient_accumulation_->Copy();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_WEIGHTS;
  }
  static constexpr char const *DESCRIPTOR = "Weights";

private:
  /**
   * xavier weights initialisation assuming guassian generator
   * using a normal distribution with mean 0 and variance 2 / (input nodes + output nodes)
   * @param weights
   */
  static void XavierInitialisation(TensorType &array, DataType normalising_factor,
                                   SizeType seed = 123456789)
  {
    // TODO (665) this is a uniform distribution; in principle we should be using a guassian
    // distribution instead we use a unifrom from -std dev -> + std dev
    fetch::random::LaggedFibonacciGenerator<> lfg(seed);

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    auto it = array.begin();
    while (it.is_valid())
    {
      auto ran_val = lfg.AsType<DataType>();  // random value in range 0 <-> 1
      ran_val      = ran_val - HALF;
      ran_val      = ran_val * DataType{2};         // random value in range -1 <-> +1
      ran_val      = ran_val * normalising_factor;  // random value in range -sigma <-> +sigma

      *it = static_cast<DataType>(ran_val);
      ++it;
    }
  }

  static void XavierInitialisationUniform(TensorType &array, DataType normalising_factor,
                                          SizeType seed = 123456789)
  {
    // TODO (#1562) this is based on uniform random generator, and it should be set to default
    // weight initialization method distribution instead we use a unifrom from -std dev -> + std dev
    fetch::random::LaggedFibonacciGenerator<> lfg(seed);

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    auto it = array.begin();
    while (it.is_valid())
    {
      auto ran_val = lfg.AsType<DataType>();  // random value in range 0 <-> 1
      ran_val      = ran_val - HALF;
      ran_val      = ran_val * DataType{2};         // random value in range -1 <-> +1
      ran_val      = ran_val * normalising_factor;  // random value in range -sigma <-> +sigma

      *it = static_cast<DataType>(ran_val);
      ++it;
    }
  }

  static const DataType HALF;
};

}  // namespace ops

template <class TensorType>
struct OpWeightsSaveableParams : public OpVariableSaveableParams<TensorType>
{
  fetch::ml::OpType op_type = OpType::OP_WEIGHTS;
};

template <class T>
const typename T::Type ops::Weights<T>::HALF = fetch::math::Type<DataType>("0.5");

}  // namespace ml
}  // namespace fetch
