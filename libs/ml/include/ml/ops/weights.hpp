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

#include "ml/ops/placeholder.hpp"
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
 * A utility class to extract the network trainable parameter and serialize them for saving /
 * sharing
 * @tparam T
 */
template <class T>
struct StateDict
{
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  ArrayPtrType                        weights_;
  std::map<std::string, StateDict<T>> dict_;

  bool operator==(StateDict<T> const &o) const
  {
    return !((bool(weights_) ^ bool(o.weights_)) || (weights_ && (*weights_ != *(o.weights_))) ||
             (dict_ != o.dict_));
  }
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

  virtual void                Step(typename T::Type learningRate)                            = 0;
  virtual struct StateDict<T> StateDict() const                                              = 0;
  virtual void                LoadStateDict(struct fetch::ml::ops::StateDict<T> const &dict) = 0;
};

template <class T>
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

private:
  ArrayPtrType gradientAccumulation_;

public:
  Weights()          = default;
  virtual ~Weights() = default;

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.empty());
    gradientAccumulation_->InlineAdd(*errorSignal);
    return {};
  }

  virtual void SetData(ArrayPtrType const &data)
  {
    PlaceHolder<T>::SetData(data);
    if (this->output_ &&
        (!gradientAccumulation_ || gradientAccumulation_->shape() != this->output_->shape()))
    {
      gradientAccumulation_ = std::make_shared<ArrayType>(this->output_->shape());
    }
  }

  virtual void Step(typename T::Type learningRate)
  {
    for (std::uint64_t i(0); i < this->gradientAccumulation_->size(); ++i)
    {
      this->gradientAccumulation_->At(i) = this->gradientAccumulation_->At(i) * -learningRate;
    }
    this->output_->InlineAdd(*gradientAccumulation_);
    // Major DL framework do not do that, but as I can't think of any reason why, I'll leave it here
    // for convenience. Remove if needed -- Pierre
    gradientAccumulation_->Fill(typename T::Type(0));
  }

  /**
   * constructs a state dictionary used for exporting/saving weights
   * @return
   */
  virtual struct StateDict<T> StateDict() const
  {
    struct fetch::ml::ops::StateDict<T> d;
    d.weights_ = this->output_;
    return d;
  }

  /**
   * load from a state dictionary to import weights
   * @param dict
   */
  virtual void
  LoadStateDict(struct fetch::ml::ops::StateDict<T> const &dict)
  {
    assert(dict.dict_.empty());
    SetData(dict.weights_);
  }

  /**
   * interface to call standard weights initialisation routines. defaults to xavier
   * @param mode  An enum indicating which type of initialisation to perform
   */
  static void Initialise(ArrayPtrType array, std::uint64_t in_size, std::uint64_t out_size,
                         WeightsInitialisation mode = WeightsInitialisation::XAVIER_GLOROT)
  {
    switch (mode)
    {
    case WeightsInitialisation::ZEROS:
    {
      for (std::uint64_t j = 0; j < array->size(); ++j)
      {
        array->At(j) = typename ArrayType::Type(0);
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

  static constexpr char const *DESCRIPTOR = "Weights";

private:
  /**
   * xavier weights initialisation
   * using a normal distribution with mean 0 and variance 2 / (input nodes + output nodes)
   * @param weights
   */
  static void XavierInitialisation(ArrayPtrType array, double normalising_factor)
  {
    std::random_device rd{};
    std::mt19937       gen{rd()};

    // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
    std::normal_distribution<> rng(0, normalising_factor);
    for (std::uint64_t i(0); i < array->size(); ++i)
    {
      array->At(i) = typename ArrayType::Type(rng(gen));
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
