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
#include "ml/state_dict.hpp"

#include <random>

namespace fetch {
namespace ml {
namespace ops {

/**
 * enum for selecting which type of initialisation to use with weights
 */
enum class WeightsInitialisation
{
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

  virtual void                           Step(typename T::Type learningRate) = 0;
  virtual struct fetch::ml::StateDict<T> StateDict() const                   = 0;
  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict)     = 0;
};

template <class T>
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using ConstSliceType    = typename ArrayType::ConstSliceType;

protected:
  ArrayPtrType gradient_accumulation_;

public:
  Weights()          = default;
  virtual ~Weights() = default;

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.empty());
    gradient_accumulation_->InlineAdd(errorSignal);
    return {};
  }

  virtual void SetData(ArrayType const &data)
  {
    PlaceHolder<T>::SetData(data);
    if (this->output_ &&
        (!gradient_accumulation_ || gradient_accumulation_->shape() != this->output_->shape()))
    {
      gradient_accumulation_ = std::make_shared<ArrayType>(this->output_->shape());
    }
  }

  virtual void Step(typename T::Type learningRate)
  {
    this->gradient_accumulation_->InlineMultiply(-learningRate);
    this->output_->InlineAdd(*gradient_accumulation_);
    // Major DL framework do not do that, but as I can't think of any reason why, I'll leave it here
    // for convenience. Remove if needed -- Pierre
    gradient_accumulation_->Fill(typename T::Type(0));
  }

  /**
   * constructs a state dictionary used for exporting/saving weights
   * @return
   */
  virtual struct fetch::ml::StateDict<T> StateDict() const
  {
    struct fetch::ml::StateDict<T> d;
    d.weights_ = this->output_;
    return d;
  }

  /**
   * load from a state dictionary to import weights
   * @param dict
   */
  virtual void
  LoadStateDict(struct fetch::ml::StateDict<T> const &dict)
  {
    assert(dict.dict_.empty());
    SetData(*dict.weights_);
  }

  /**
   * interface to call standard weights initialisation routines. defaults to xavier
   * @param mode  An enum indicating which type of initialisation to perform
   */
  static void Initialise(ArrayType &array, std::uint64_t in_size, std::uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT)
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
      XavierInitialisation(array, std::sqrt(2.0 / double(in_size + out_size)));
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_IN:
    {
      XavierInitialisation(array, std::sqrt(1.0 / double(in_size)));
      break;
    }
    case WeightsInitialisation::XAVIER_FAN_OUT:
    {
      XavierInitialisation(array, std::sqrt(1.0 / double(out_size)));
      break;
    }
    default:
      std::cerr << "unrecognised weights initialisation" << std::endl;
      throw;
    }
  }

  /**
   * exports the weights Array
   * @return
   */
  ArrayType const &GetWeights() const
  {
    return *this->output_;
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
    for (auto &e : array)
    {
      auto ran_val = lfg_.AsDouble();  // random value in range 0 <-> 1
      ran_val -= 0.5;
      ran_val *= 2.0;                 // random value in range -1 <-> +1
      ran_val *= normalising_factor;  // random value in range -sigma <-> +sigma

      e = typename ArrayType::Type(ran_val);
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
