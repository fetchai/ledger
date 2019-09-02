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

#include "core/random/lfg.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/trainable.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
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
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<T>
{
public:
  using TensorType     = T;
  using SizeType       = typename TensorType::SizeType;
  using DataType       = typename TensorType::Type;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename PlaceHolder<T>::VecTensorType;
  using SPType         = OpWeightsSaveableParams<TensorType>;
  using WeightsPtrType = typename std::shared_ptr<Weights<TensorType>>;

protected:
  ArrayPtrType gradient_accumulation_;

public:
  Weights() = default;

  explicit Weights(SPType const &sp)
    : PlaceHolder<T>(sp)
  {
    if (sp.gradient_accumulation)
    {
      gradient_accumulation_ = std::make_shared<TensorType>();
      gradient_accumulation_->Resize(sp.gradient_accumulation->shape());
      gradient_accumulation_->Copy(*(sp.gradient_accumulation));
    }

    this->SetRegularisation(
        fetch::ml::details::CreateRegulariser<TensorType>(sp.regularisation_type),
        sp.regularisation_rate);
  }

  ~Weights() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp   = std::make_shared<SPType>();
    auto p_sp = PlaceHolder<T>::GetOpSaveableParams();

    auto cast_sp = std::static_pointer_cast<OpPlaceholderSaveableParams<TensorType>>(sp);
    *cast_sp     = *(std::static_pointer_cast<OpPlaceholderSaveableParams<TensorType>>(p_sp));

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
    return sp;
  }

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override
  {
    // This overrides implementation in Placeholder
    assert(me.get() == this);
    return me;
  }

  ArrayPtrType GetShareableWeights()
  {
    return this->output_;
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());

    gradient_accumulation_->InlineAdd(error_signal);

    return {};
  }

  bool SetData(TensorType const &data) override
  {
    bool shape_changed = PlaceHolder<T>::SetData(data);
    if (shape_changed)
    {
      gradient_accumulation_ = std::make_shared<TensorType>(this->output_->shape());
      return true;
    }
    return false;
  }

  void Step(typename T::Type learning_rate) override
  {
    this->gradient_accumulation_->InlineMultiply(-learning_rate);
    this->output_->InlineAdd(*gradient_accumulation_);
    ResetGradients();
  }

  void ApplyGradient(TensorType const &grad) override
  {
    this->output_->InlineAdd(grad);
    ResetGradients();
  }

  /**
   * Set all gradient values to 0
   */
  void ResetGradients() override
  {
    gradient_accumulation_->Fill(typename T::Type(0));
  }

  void ApplyRegularisation() override
  {
    if (this->regulariser_)
    {
      this->regulariser_->ApplyRegularisation(*this->output_, this->regularisation_rate_);
    }
  }

  /**
   * constructs a state dictionary used for exporting/saving weights
   * @return
   */
  struct fetch::ml::StateDict<T> StateDict() const override
  {
    struct fetch::ml::StateDict<T> d;
    d.weights_ = this->output_;
    return d;
  }

  /**
   * load from a state dictionary to import weights
   * @param dict
   */
  void
  LoadStateDict(struct fetch::ml::StateDict<T> const &dict) override
  {
    assert(dict.dict_.empty());
    SetData(*dict.weights_);
  }

  /**
   * interface to call standard weights initialisation routines. defaults to xavier
   * @param mode  An enum indicating which type of initialisation to perform
   */
  static void Initialise(TensorType &array, std::uint64_t in_size, std::uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ZEROS:
    {
      array.Fill(typename TensorType::Type(0));
      break;
    }
    case WeightsInitialisation::ONES:
    {
      array.Fill(typename TensorType::Type(1));
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT:
    {
      XavierInitialisation(array, std::sqrt(2.0 / double(in_size + out_size)), seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_IN:
    {
      XavierInitialisation(array, std::sqrt(1.0 / double(in_size)), seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_OUT:
    {
      XavierInitialisation(array, std::sqrt(1.0 / double(out_size)), seed);
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT_UNIFORM:
    {
      XavierInitialisationUniform(array, std::sqrt(6.0 / double(in_size + out_size)), seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_IN_UNIFORM:
    {
      XavierInitialisationUniform(array, std::sqrt(3.0 / double(in_size)), seed);
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_OUT_UNIFORM:
    {
      XavierInitialisationUniform(array, std::sqrt(3.0 / double(out_size)), seed);
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
  static void Initialise(TensorType &array, std::uint64_t data_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ONES:
    {
      array.Fill(static_cast<typename TensorType::Type>(1));
      break;
    }
    case WeightsInitialisation::ZEROS:
    {
      array.Fill(static_cast<typename TensorType::Type>(0));
      break;
    }
    case WeightsInitialisation::XAVIER_GLOROT:
    {
      XavierInitialisation(array, std::sqrt(2.0 / double(data_size)), seed);
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
  TensorType const &get_weights() const override
  {
    return *this->output_;
  }

  void SetWeights(TensorType &new_value) override
  {
    this->output_->Assign(new_value);
  }

  /**
   * exports the weight gradients Array
   * @return const reference to internal accumulated gradient Array
   */
  TensorType const &GetGradientsReferences() const override
  {
    return *this->gradient_accumulation_;
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
  static void XavierInitialisation(TensorType &array, double normalising_factor,
                                   SizeType seed = 123456789)
  {
    // TODO (665) this is a uniform distribution; in principle we should be using a guassian
    // distribution instead we use a unifrom from -std dev -> + std dev
    fetch::random::LaggedFibonacciGenerator<> lfg_(seed);

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    auto it = array.begin();
    while (it.is_valid())
    {
      auto ran_val = lfg_.AsDouble();  // random value in range 0 <-> 1
      ran_val -= 0.5;
      ran_val *= 2.0;                 // random value in range -1 <-> +1
      ran_val *= normalising_factor;  // random value in range -sigma <-> +sigma

      *it = typename TensorType::Type(ran_val);
      ++it;
    }
  }

  static void XavierInitialisationUniform(TensorType &array, double normalising_factor,
                                          SizeType seed = 123456789)
  {
    // TODO (#1562) this is based on uniform random generator, and it should be set to default
    // weight initialization method distribution instead we use a unifrom from -std dev -> + std dev
    fetch::random::LaggedFibonacciGenerator<> lfg_(seed);

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    auto it = array.begin();
    while (it.is_valid())
    {
      auto ran_val = lfg_.AsDouble();  // random value in range 0 <-> 1
      ran_val -= 0.5;
      ran_val *= 2.0;                 // random value in range -1 <-> +1
      ran_val *= normalising_factor;  // random value in range -sigma <-> +sigma

      *it = typename TensorType::Type(ran_val);
      ++it;
    }
  }
};

}  // namespace ops

template <class TensorType>
struct OpWeightsSaveableParams : public OpPlaceholderSaveableParams<TensorType>
{
  fetch::ml::OpType           op_type = OpType::OP_WEIGHTS;
  std::shared_ptr<TensorType> gradient_accumulation;
  RegularisationType          regularisation_type;
  typename TensorType::Type   regularisation_rate;
};

}  // namespace ml
}  // namespace fetch
