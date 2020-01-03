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

#include "math/tensor/tensor.hpp"
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

ChargeAmount TensorEstimator::VMShape()
{
  return LOW_CHARGE;
}

ChargeAmount TensorEstimator::Copy()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  auto ret = static_cast<ChargeAmount>(COPY_PADDED_SIZE_COEF * padded_size + COPY_SIZE_COEF * size +
                                       COPY_CONST_COEF) *
             COMPUTE_CHARGE_COST;

  // Ensure that estimate will never be 0
  if (ret < std::numeric_limits<uint64_t>::max())
  {
    ret += 1;
  }

  return ret;
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

  return ToChargeAmount(FILL_PADDED_SIZE_COEF * padded_size + FILL_SIZE_COEF * size +
                        FILL_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::FillRandom()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(FILL_RANDOM_PADDED_SIZE_COEF * padded_size + FILL_RANDOM_SIZE_COEF * size +
                        FILL_RANDOM_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Min()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(MIN_PADDED_SIZE_COEF * padded_size + MIN_SIZE_COEF * size +
                        MIN_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Max()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(MAX_PADDED_SIZE_COEF * padded_size + MAX_SIZE_COEF * size +
                        MAX_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Reshape(
    fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape)
{
  if (new_shape->elements.empty())
  {
    return MaximumCharge("Can not reshape a Tensor : new shape is empty!");
  }
  auto                  n_elements = new_shape->elements.size();
  std::vector<SizeType> c_data(n_elements);

  std::size_t new_total_elements = 1;
  for (fetch::math::SizeType i{0}; i < n_elements; i++)
  {
    SizeType const axis_size = new_shape->elements.at(i);
    if (axis_size == 0)
    {
      return MaximumCharge("Can not reshape a Tensor : axis of size 0 found in new shape!");
    }
    new_total_elements *= c_data.at(i) = new_shape->elements.at(i);
  }
  if (new_total_elements != tensor_.size())
  {
    return MaximumCharge("Can not reshape a Tensor : total elements count in the new shape (" +
                         std::to_string(new_total_elements) +
                         ") mismatch. Expected : " + std::to_string(tensor_.size()));
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

  return ToChargeAmount(SUM_PADDED_SIZE_COEF * padded_size + SUM_SIZE_COEF * size +
                        SUM_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::ArgMax(SizeType const &indices)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  if (indices == 0)
  {
    return ToChargeAmount(ARGMAX_FIRST_PADDED_SIZE_COEF * padded_size +
                          ARGMAX_FIRST_SIZE_COEF * size + ARGMAX_FIRST_CONST_COEF) *
           COMPUTE_CHARGE_COST;
  }
  if (indices == tensor_.shape().size() - 1)
  {
    return ToChargeAmount(ARGMAX_LAST_PADDED_SIZE_COEF * padded_size +
                          ARGMAX_LAST_SIZE_COEF * size + ARGMAX_LAST_CONST_COEF) *
           COMPUTE_CHARGE_COST;
  }

  return static_cast<ChargeAmount>(ARGMAX_MID_PADDED_SIZE_COEF * padded_size +
                                   ARGMAX_MID_SIZE_COEF * size + ARGMAX_MID_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::ArgMaxNoIndices()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(ARGMAX_FIRST_PADDED_SIZE_COEF * padded_size +
                        ARGMAX_FIRST_SIZE_COEF * size + ARGMAX_FIRST_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::Dot(vm::Ptr<VMTensor> const &other)
{
  SizeType x = tensor_.shape().at(0);
  SizeType y = other->GetTensor().shape().at(1);
  SizeType c = tensor_.shape().at(1);

  return ToChargeAmount(DOT_X_COEF * x + DOT_Y_COEF * y + DOT_C_COEF * c +
                        DOT_CUBIC_COEF * x * y * c + DOT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::GetReshapeCost(SizeVector const &new_shape)
{
  if (new_shape == tensor_.shape())
  {
    return LOW_CHARGE;
  }

  SizeType padded_size_from = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType padded_size_to   = fetch::math::Tensor<DataType>::PaddedSizeFromShape(new_shape);

  return ToChargeAmount(RESHAPE_PADDED_SIZE_FROM_COEF * padded_size_from +
                        RESHAPE_PADDED_SIZE_TO_COEF * padded_size_to + RESHAPE_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

/// OPERATORS ///

ChargeAmount TensorEstimator::NegateChargeEstimator(vm::Ptr<Object> const & /*object*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(NEGATE_PADDED_SIZE_COEF * padded_size + NEGATE_SIZE_COEF * size +
                        NEGATE_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::IsEqualChargeEstimator(vm::Ptr<Object> const & /*lhso*/,
                                                     vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(IS_EQUAL_PADDED_SIZE_COEF * padded_size + IS_EQUAL_SIZE_COEF * size +
                        IS_EQUAL_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::IsNotEqualChargeEstimator(vm::Ptr<Object> const & /*lhso*/,
                                                        vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(IS_NOT_EQUAL_PADDED_SIZE_COEF * padded_size +
                        IS_NOT_EQUAL_SIZE_COEF * size + IS_NOT_EQUAL_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::AddChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                 vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(ADD_PADDED_SIZE_COEF * padded_size + ADD_SIZE_COEF * size +
                        ADD_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::SubtractChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                      vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(SUBTRACT_PADDED_SIZE_COEF * padded_size + SUBTRACT_SIZE_COEF * size +
                        SUBTRACT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceAddChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                        vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(INPLACE_ADD_PADDED_SIZE_COEF * padded_size + INPLACE_ADD_SIZE_COEF * size +
                        INPLACE_ADD_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceSubtractChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                             vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(INPLACE_SUBTRACT_PADDED_SIZE_COEF * padded_size +
                        INPLACE_SUBTRACT_SIZE_COEF * size + INPLACE_SUBTRACT_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::MultiplyChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                      vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(MULTIPLY_PADDED_SIZE_COEF * padded_size + MULTIPLY_SIZE_COEF * size +
                        MULTIPLY_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::DivideChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                    vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(DIVIDE_PADDED_SIZE_COEF * padded_size + DIVIDE_SIZE_COEF * size +
                        DIVIDE_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceMultiplyChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                             vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(INPLACE_MULTIPLY_PADDED_SIZE_COEF * padded_size +
                        INPLACE_MULTIPLY_SIZE_COEF * size + INPLACE_MULTIPLY_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::InplaceDivideChargeEstimator(vm::Ptr<vm::Object> const & /*lhso*/,
                                                           vm::Ptr<Object> const & /*rhso*/)
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(INPLACE_DIVIDE_PADDED_SIZE_COEF * padded_size +
                        INPLACE_DIVIDE_SIZE_COEF * size + INPLACE_DIVIDE_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

/// END OF OPERATORS ///

ChargeAmount TensorEstimator::Transpose()
{
  if (tensor_.shape().size() != 2)
  {
    return MaximumCharge("Cannot transpose tensor, only two-dimensional Tensor can be transposed.");
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
  return ToChargeAmount(FROM_STRING_SIZE_COEF * string->Length() + FROM_STRING_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::ToString()
{
  SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_.shape());
  SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_.shape());

  return ToChargeAmount(TO_STRING_PADDED_SIZE_COEF * padded_size + TO_STRING_SIZE_COEF * size +
                        TO_STRING_CONST_COEF) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount TensorEstimator::MaximumCharge(std::string const &log_msg)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "operation charge is vm::MAXIMUM_CHARGE : " + log_msg);
  return vm::MAXIMUM_CHARGE;
}

ChargeAmount TensorEstimator::ToChargeAmount(fixed_point::fp64_t const &val)
{
  auto ret = static_cast<ChargeAmount>(val);
  // Ensure that estimate will never be 0
  if (ret < std::numeric_limits<uint64_t>::max())
  {
    ret += 1;
  }
  return ret;
}

// Fill
fixed_point::fp64_t const TensorEstimator::FILL_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.00023451");
fixed_point::fp64_t const TensorEstimator::FILL_SIZE_COEF  = fixed_point::fp64_t("0.00107809");
fixed_point::fp64_t const TensorEstimator::FILL_CONST_COEF = fixed_point::fp64_t("5");

// FillRandom
fixed_point::fp64_t const TensorEstimator::FILL_RANDOM_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0001");
fixed_point::fp64_t const TensorEstimator::FILL_RANDOM_SIZE_COEF  = fixed_point::fp64_t("0.049");
fixed_point::fp64_t const TensorEstimator::FILL_RANDOM_CONST_COEF = fixed_point::fp64_t("5");

// Min
fixed_point::fp64_t const TensorEstimator::MIN_PADDED_SIZE_COEF = fixed_point::fp64_t("0.00023451");
fixed_point::fp64_t const TensorEstimator::MIN_SIZE_COEF        = fixed_point::fp64_t("0.00107809");
fixed_point::fp64_t const TensorEstimator::MIN_CONST_COEF       = fixed_point::fp64_t("5");

// MAX
fixed_point::fp64_t const TensorEstimator::MAX_PADDED_SIZE_COEF = fixed_point::fp64_t("0.00023451");
fixed_point::fp64_t const TensorEstimator::MAX_SIZE_COEF        = fixed_point::fp64_t("0.00107809");
fixed_point::fp64_t const TensorEstimator::MAX_CONST_COEF       = fixed_point::fp64_t("5");

// SUM
fixed_point::fp64_t const TensorEstimator::SUM_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0005");
fixed_point::fp64_t const TensorEstimator::SUM_SIZE_COEF        = fixed_point::fp64_t("0.007");
fixed_point::fp64_t const TensorEstimator::SUM_CONST_COEF       = fixed_point::fp64_t("5");

// RESHAPE
fixed_point::fp64_t const TensorEstimator::RESHAPE_PADDED_SIZE_FROM_COEF =
    fixed_point::fp64_t("0.004");
fixed_point::fp64_t const TensorEstimator::RESHAPE_PADDED_SIZE_TO_COEF =
    fixed_point::fp64_t("0.004");
fixed_point::fp64_t const TensorEstimator::RESHAPE_CONST_COEF = fixed_point::fp64_t("35");

// FROM_STRING
fixed_point::fp64_t const TensorEstimator::FROM_STRING_SIZE_COEF =
    fixed_point::fp64_t("0.00107809");
fixed_point::fp64_t const TensorEstimator::FROM_STRING_CONST_COEF = fixed_point::fp64_t("5");

// TO_STRING
fixed_point::fp64_t const TensorEstimator::TO_STRING_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.00023451");
fixed_point::fp64_t const TensorEstimator::TO_STRING_SIZE_COEF  = fixed_point::fp64_t("0.00107809");
fixed_point::fp64_t const TensorEstimator::TO_STRING_CONST_COEF = fixed_point::fp64_t("5");

// DOT
fixed_point::fp64_t const TensorEstimator::DOT_X_COEF = fixed_point::fp64_t("0.003225806451613");
fixed_point::fp64_t const TensorEstimator::DOT_Y_COEF = fixed_point::fp64_t("0.125");
fixed_point::fp64_t const TensorEstimator::DOT_C_COEF = fixed_point::fp64_t("0.020408163265306");
fixed_point::fp64_t const TensorEstimator::DOT_CUBIC_COEF =
    fixed_point::fp64_t("0.006711409395973");
fixed_point::fp64_t const TensorEstimator::DOT_CONST_COEF = fixed_point::fp64_t("5");

// Negate
fixed_point::fp64_t const TensorEstimator::NEGATE_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::NEGATE_SIZE_COEF        = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::NEGATE_CONST_COEF       = fixed_point::fp64_t("5");

// IsEqual
fixed_point::fp64_t const TensorEstimator::IS_EQUAL_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::IS_EQUAL_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::IS_EQUAL_CONST_COEF = fixed_point::fp64_t("5");

// IsNotEqual
fixed_point::fp64_t const TensorEstimator::IS_NOT_EQUAL_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::IS_NOT_EQUAL_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::IS_NOT_EQUAL_CONST_COEF = fixed_point::fp64_t("5");

// Add
fixed_point::fp64_t const TensorEstimator::ADD_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::ADD_SIZE_COEF        = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::ADD_CONST_COEF       = fixed_point::fp64_t("5");

// InplaceAdd
fixed_point::fp64_t const TensorEstimator::INPLACE_ADD_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::INPLACE_ADD_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::INPLACE_ADD_CONST_COEF = fixed_point::fp64_t("5");

// Subtract
fixed_point::fp64_t const TensorEstimator::SUBTRACT_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::SUBTRACT_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::SUBTRACT_CONST_COEF = fixed_point::fp64_t("5");

// InplaceSubtract
fixed_point::fp64_t const TensorEstimator::INPLACE_SUBTRACT_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::INPLACE_SUBTRACT_SIZE_COEF =
    fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::INPLACE_SUBTRACT_CONST_COEF = fixed_point::fp64_t("5");

// Multiply
fixed_point::fp64_t const TensorEstimator::MULTIPLY_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::MULTIPLY_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::MULTIPLY_CONST_COEF = fixed_point::fp64_t("5");

// InplaceMultiply
fixed_point::fp64_t const TensorEstimator::INPLACE_MULTIPLY_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::INPLACE_MULTIPLY_SIZE_COEF =
    fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::INPLACE_MULTIPLY_CONST_COEF = fixed_point::fp64_t("5");

// Divide
fixed_point::fp64_t const TensorEstimator::DIVIDE_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::DIVIDE_SIZE_COEF        = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::DIVIDE_CONST_COEF       = fixed_point::fp64_t("5");

// InplaceDivide
fixed_point::fp64_t const TensorEstimator::INPLACE_DIVIDE_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::INPLACE_DIVIDE_SIZE_COEF  = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::INPLACE_DIVIDE_CONST_COEF = fixed_point::fp64_t("5");

// Copy
fixed_point::fp64_t const TensorEstimator::COPY_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0058611875");
fixed_point::fp64_t const TensorEstimator::COPY_SIZE_COEF  = fixed_point::fp64_t("0.008");
fixed_point::fp64_t const TensorEstimator::COPY_CONST_COEF = fixed_point::fp64_t("50");

// ArgMax
fixed_point::fp64_t const TensorEstimator::ARGMAX_FIRST_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.001");
fixed_point::fp64_t const TensorEstimator::ARGMAX_FIRST_SIZE_COEF  = fixed_point::fp64_t("0.11");
fixed_point::fp64_t const TensorEstimator::ARGMAX_FIRST_CONST_COEF = fixed_point::fp64_t("50");

fixed_point::fp64_t const TensorEstimator::ARGMAX_MID_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0032");
fixed_point::fp64_t const TensorEstimator::ARGMAX_MID_SIZE_COEF  = fixed_point::fp64_t("0.0452");
fixed_point::fp64_t const TensorEstimator::ARGMAX_MID_CONST_COEF = fixed_point::fp64_t("50");

fixed_point::fp64_t const TensorEstimator::ARGMAX_LAST_PADDED_SIZE_COEF =
    fixed_point::fp64_t("0.0001");
fixed_point::fp64_t const TensorEstimator::ARGMAX_LAST_SIZE_COEF  = fixed_point::fp64_t("0.0562");
fixed_point::fp64_t const TensorEstimator::ARGMAX_LAST_CONST_COEF = fixed_point::fp64_t("50");

// DEFAULT
fixed_point::fp64_t const TensorEstimator::DEFAULT_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0042");
fixed_point::fp64_t const TensorEstimator::DEFAULT_SIZE_COEF        = fixed_point::fp64_t("0.009");
fixed_point::fp64_t const TensorEstimator::DEFAULT_CONST_COEF       = fixed_point::fp64_t("5");

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
