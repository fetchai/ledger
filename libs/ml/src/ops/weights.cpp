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
#include "ml/ops/weights.hpp"
#include "ml/state_dict.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
Weights<TensorType>::Weights(SPType const &sp)
  : Variable<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Weights<TensorType>::GetOpSaveableParams()
{
  auto sp   = std::make_shared<SPType>();
  auto p_sp = Variable<TensorType>::GetOpSaveableParams();

  auto cast_sp = std::static_pointer_cast<OpVariableSaveableParams<TensorType>>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpVariableSaveableParams<TensorType>>(p_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Weights<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  // This overrides implementation in Placeholder
  assert(me.get() == this);
  return me;
}

/**
 * constructs a state dictionary used for exporting/saving weights
 * @return
 */
template <typename TensorType>
fetch::ml::StateDict<TensorType> Weights<TensorType>::StateDict() const
{
  fetch::ml::StateDict<TensorType> d;
  d.weights_ = this->data_;
  return d;
}

/**
 * load from a state dictionary to import weights
 * @param dict
 */
template <typename TensorType>
void Weights<TensorType>::LoadStateDict(fetch::ml::StateDict<TensorType> const &dict)
{
  assert(dict.dict_.empty());
  this->SetData(*dict.weights_);
}

/**
 * interface to call standard weights initialisation routines. defaults to xavier
 * @param mode  An enum indicating which type of initialisation to perform
 */
template <typename TensorType>
void Weights<TensorType>::Initialise(TensorType &array, uint64_t in_size, uint64_t out_size,
                                     WeightsInitialisation mode, SizeType seed)
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
    XavierInitialisation(array,
                         fetch::math::Sqrt(static_cast<DataType>(
                             static_cast<DataType>(2) / static_cast<DataType>(in_size + out_size))),
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
                         fetch::math::Sqrt(static_cast<DataType>(static_cast<DataType>(1) /
                                                                 static_cast<DataType>(out_size))),
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
template <typename TensorType>
void Weights<TensorType>::Initialise(TensorType &array, uint64_t data_size,
                                     WeightsInitialisation mode, SizeType seed)
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
    XavierInitialisation(array,
                         static_cast<DataType>(fetch::math::Sqrt(static_cast<DataType>(2) /
                                                                 static_cast<DataType>(data_size))),
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
template <typename TensorType>
TensorType const &Weights<TensorType>::GetWeights() const
{
  return *this->data_;
}

template <typename TensorType>
void Weights<TensorType>::SetWeights(TensorType const &new_value)
{
  this->data_->Assign(new_value);
}

/**
 * exports the weight gradients Array
 * @return const reference to internal accumulated gradient Array and unordered set of indices
 * which were updated
 */
template <typename TensorType>
std::pair<TensorType const, typename Weights<TensorType>::SizeSet const>
Weights<TensorType>::GetSparseGradientsReferences() const
{
  return std::move(std::make_pair(*this->gradient_accumulation_, this->updated_rows_));
}

/**
 * exports the weight gradients Array
 * @return const reference to internal accumulated gradient Array
 */
template <typename TensorType>
TensorType const &Weights<TensorType>::GetGradientsReferences() const
{
  return *this->gradient_accumulation_;
}

template <typename TensorType>
typename Weights<TensorType>::SizeSet const &Weights<TensorType>::GetUpdatedRowsReferences() const
{
  return this->updated_rows_;
}

/**
 * returns deep copy of the weight gradients Array
 * @return Internal accumulated gradient Array
 */
template <typename TensorType>
TensorType Weights<TensorType>::GetGradients() const
{
  return this->gradient_accumulation_->Copy();
}

/**
 * xavier weights initialisation assuming guassian generator
 * using a normal distribution with mean 0 and variance 2 / (input nodes + output nodes)
 * @param weights
 */
template <typename TensorType>
void Weights<TensorType>::XavierInitialisation(TensorType &array, DataType normalising_factor,
                                               SizeType seed)
{
  // TODO (665) this is a uniform distribution; in principle we should be using a guassian
  // distribution instead we use a unifrom from -std dev -> + std dev
  fetch::random::LaggedFibonacciGenerator<> lfg(seed);

  // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
  auto it = array.begin();
  while (it.is_valid())
  {
    auto ran_val = lfg.AsType<DataType>();  // random value in range 0 <-> 1
    ran_val      = static_cast<DataType>(ran_val - HALF);
    ran_val      = static_cast<DataType>(ran_val * DataType{2});  // random value in range -1 <-> +1
    ran_val      = static_cast<DataType>(ran_val *
                                    normalising_factor);  // random value in range -sigma <-> +sigma

    *it = static_cast<DataType>(ran_val);
    ++it;
  }
}

template <typename TensorType>
void Weights<TensorType>::XavierInitialisationUniform(TensorType &array,
                                                      DataType normalising_factor, SizeType seed)
{
  // TODO (#1562) this is based on uniform random generator, and it should be set to default
  // weight initialization method distribution instead we use a unifrom from -std dev -> + std dev
  fetch::random::LaggedFibonacciGenerator<> lfg(seed);

  // http://proceedings.mlr.press/v9/glorot10a/glorot10a.pdf
  auto it = array.begin();
  while (it.is_valid())
  {
    auto ran_val = lfg.AsType<DataType>();  // random value in range 0 <-> 1
    ran_val      = static_cast<DataType>(ran_val - HALF);
    ran_val      = static_cast<DataType>(ran_val * DataType{2});  // random value in range -1 <-> +1
    ran_val      = static_cast<DataType>(ran_val *
                                    normalising_factor);  // random value in range -sigma <-> +sigma

    *it = static_cast<DataType>(ran_val);
    ++it;
  }
}

template <class T>
const typename T::Type ops::Weights<T>::HALF = fetch::math::Type<DataType>("0.5");

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Weights<math::Tensor<int8_t>>;
template class Weights<math::Tensor<int16_t>>;
template class Weights<math::Tensor<int32_t>>;
template class Weights<math::Tensor<int64_t>>;
template class Weights<math::Tensor<uint8_t>>;
template class Weights<math::Tensor<uint16_t>>;
template class Weights<math::Tensor<uint32_t>>;
template class Weights<math::Tensor<uint64_t>>;
template class Weights<math::Tensor<float>>;
template class Weights<math::Tensor<double>>;
template class Weights<math::Tensor<fixed_point::fp32_t>>;
template class Weights<math::Tensor<fixed_point::fp64_t>>;
template class Weights<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
