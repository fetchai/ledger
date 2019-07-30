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
  XAVIER_FAN_OUT
};

/**
 * Provide an interface for any trainable ops
 * @tparam T passes tensors to graph during update step
 */
template <class T>
class Trainable
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using DataType     = typename ArrayType::Type;
  using RegPtrType   = std::shared_ptr<fetch::ml::regularisers::Regulariser<T>>;

  virtual void                           Step(typename T::Type learning_rate)        = 0;
  virtual struct fetch::ml::StateDict<T> StateDict() const                           = 0;
  virtual void             LoadStateDict(struct fetch::ml::StateDict<T> const &dict) = 0;
  virtual ArrayType const &get_weights() const                                       = 0;
  virtual ArrayType const &get_gradients() const                                     = 0;
  virtual void             ResetGradients()                                          = 0;
  virtual void             ApplyGradient(ArrayType const &grad)                      = 0;
  virtual void             ApplyRegularisation()                                     = 0;

  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0.0})
  {
    this->regulariser_         = regulariser;
    this->regularisation_rate_ = regularisation_rate;
  }

protected:
  RegPtrType regulariser_;
  DataType   regularisation_rate_ = static_cast<DataType>(0);
};

template <class T>
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<T>
{
public:
  using ArrayType      = T;
  using SizeType       = typename ArrayType::SizeType;
  using DataType       = typename ArrayType::Type;
  using ArrayPtrType   = std::shared_ptr<ArrayType>;
  using VecTensorType  = typename PlaceHolder<T>::VecTensorType;
  using WeightsPtrType = typename std::shared_ptr<Weights<ArrayType>>;

protected:
  ArrayPtrType gradient_accumulation_;

public:
  Weights()           = default;
  ~Weights() override = default;

  ArrayPtrType GetShareableWeights()
  {
    return this->output_;
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    gradient_accumulation_->InlineAdd(error_signal);
    return {};
  }

  bool SetData(ArrayType const &data) override
  {
    if (PlaceHolder<T>::SetData(data))  // if input_size_changed
    {
      gradient_accumulation_ = std::make_shared<ArrayType>(this->output_->shape());
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

  void ApplyGradient(ArrayType const &grad) override
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
  static void Initialise(ArrayType &array, std::uint64_t in_size, std::uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ZEROS:
    {
      for (std::uint64_t j = 0; j < array.data().size(); ++j)
      {
        array.data()[j] = typename ArrayType::Type(0);
      }
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
  static void Initialise(ArrayType &array, std::uint64_t data_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT,
                         SizeType              seed = 123456789)
  {
    switch (mode)
    {
    case WeightsInitialisation::ONES:
    {
	    for (std::uint64_t j = 0; j < array.data().size(); ++j)
	    {
		    array.data()[j] = typename ArrayType::Type(1);
	    }
	    break;
    }
    case WeightsInitialisation::ZEROS:
    {
      for (std::uint64_t j = 0; j < array.data().size(); ++j)
      {
        array.data()[j] = typename ArrayType::Type(0);
      }
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
  ArrayType const &get_weights() const override
  {
    return *this->output_;
  }

  /**
   * exports the weight gradients Array
   * @return const reference to internal accumulated gradient Array
   */
  ArrayType const &get_gradients() const override
  {
    return *this->gradient_accumulation_;
  }

  static constexpr char const *DESCRIPTOR = "Weights";

private:
  /**
   * xavier weights initialisation
   * using a normal distribution with mean 0 and variance 2 / (input nodes + output nodes)
   * @param weights
   */
  static void XavierInitialisation(ArrayType &array, double normalising_factor,
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

      *it = typename ArrayType::Type(ran_val);
      ++it;
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
