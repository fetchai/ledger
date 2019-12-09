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

#include "math/tensor.hpp"
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/tensor/tensor_estimator.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/use_estimator.hpp"

#include <cstdint>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

static constexpr char const *LOGGING_NAME = "VMTensorEstimator";

using ArrayType  = fetch::math::Tensor<VMTensor::DataType>;
using SizeType   = ArrayType::SizeType;
using SizeVector = ArrayType::SizeVector;

TensorEstimator::TensorEstimator(VMTensor &tensor)
  : tensor_{tensor}
{}

ChargeAmount TensorEstimator::size()
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::AtOne(TensorType::SizeType /*idx1*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::AtTwo(uint64_t /*idx1*/, uint64_t /*idx2*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::AtThree(uint64_t /*idx1*/, uint64_t /*idx2*/, uint64_t /*idx3*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::AtFour(uint64_t /*idx1*/, uint64_t /*idx2*/, uint64_t /*idx3*/,
                                     uint64_t /*idx4*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::SetAtOne(uint64_t /*idx1*/, DataType const & /*value*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::SetAtTwo(uint64_t /*idx1*/, uint64_t /*idx2*/,
                                       DataType const & /*value*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::SetAtThree(uint64_t /*idx1*/, uint64_t /*idx2*/, uint64_t /*idx3*/,
                                         DataType const & /*value*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::SetAtFour(uint64_t /*idx1*/, uint64_t /*idx2*/, uint64_t /*idx3*/,
                                        uint64_t /*idx4*/, DataType const & /*value*/)
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::Fill(DataType const & /*value*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(FILL_PADDED_SIZE_COEF() * padded_size + FILL_SIZE_COEF() * size +
                                   FILL_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::FillRandom()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(FILL_RANDOM_PADDED_SIZE_COEF() * padded_size +
                                   FILL_RANDOM_SIZE_COEF() * size + FILL_RANDOM_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Min()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(MIN_PADDED_SIZE_COEF() * padded_size + MIN_SIZE_COEF() * size +
                                   MIN_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Max()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(MAX_PADDED_SIZE_COEF() * padded_size + MAX_SIZE_COEF() * size +
                                   MAX_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Reshape(
    fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape)
{
  auto                  n_elements = new_shape->elements.size();
  std::vector<SizeType> c_data(n_elements);

  for (fetch::math::SizeType i{0}; i < n_elements; i++)
  {
    c_data.at(i) = new_shape->elements.at(i);
  }

  return GetReshapeCost(c_data);
}

ChargeAmount TensorEstimator::Squeeze()
{
  SizeVector new_shape = tensor_.shape();
  new_shape.emplace_back(1);
  return GetReshapeCost(new_shape);
}

ChargeAmount TensorEstimator::Sum()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(SUM_PADDED_SIZE_COEF() * padded_size + SUM_SIZE_COEF() * size +
                                   SUM_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::GetReshapeCost(SizeVector const &new_shape)
{

  if (new_shape == tensor_.shape())
  {
    return LOW_CHARGE;
  }

  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(new_shape);
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(new_shape);

  return static_cast<ChargeAmount>(RESHAPE_PADDED_SIZE_COEF() * padded_size +
                                   RESHAPE_SIZE_COEF() * size + RESHAPE_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

/// OPERATORS ///

ChargeAmount TensorEstimator::EqualOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::NotEqualOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::NegateOperator()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::AddOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::SubtractOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::MultiplyOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::DivideOperator(vm::Ptr<VMTensor> const & /*other*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

// TODO (ML-340) - replace member functions 'operators' above with below operator functions when
// operators can take estimators

ChargeAmount TensorEstimator::Negate()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(SUM_PADDED_SIZE_COEF() * padded_size + SUM_SIZE_COEF() * size +
                                   SUM_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Equal()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::NotEqual()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Add()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Subtract()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceAdd()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceSubtract()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Multiply()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Divide()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceMultiply()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceDivide()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());
  return static_cast<ChargeAmount>(DEFAULT_PADDED_SIZE_COEF * padded_size +
                                   DEFAULT_SIZE_COEF * size + DEFAULT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

/// END OF OPERATORS ///

ChargeAmount TensorEstimator::Transpose()
{
  if (tensor_.shape().size() != 2)
  {
    return MaximumCharge("Cannot transpose tensor, only two-dimensional one can be transposed.");
  }

  return GetReshapeCost({tensor_.shape().at(1), tensor_.shape().at(0)});
}

ChargeAmount TensorEstimator::Unsqueeze()
{
  SizeVector new_shape = tensor_.shape();
  new_shape.emplace_back(1);
  return GetReshapeCost(new_shape);
}

ChargeAmount TensorEstimator::FromString(fetch::vm::Ptr<fetch::vm::String> const &string)
{
  return static_cast<ChargeAmount>(FROM_STRING_SIZE_COEF() * string->Length() +
                                   FROM_STRING_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::ToString()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return static_cast<ChargeAmount>(TO_STRING_PADDED_SIZE_COEF() * padded_size +
                                   TO_STRING_SIZE_COEF() * size + TO_STRING_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::MaximumCharge(std::string const &log_msg)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "operation charge is vm::MAXIMUM_CHARGE : " + log_msg);
  return vm::MAXIMUM_CHARGE;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
