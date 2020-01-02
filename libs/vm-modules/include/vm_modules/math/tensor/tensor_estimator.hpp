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

#include "math/tensor/tensor.hpp"
#include "vm/common.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <cstdint>
#include <vector>

namespace fetch {

namespace math {
template <typename, typename>
class Tensor;
}
namespace vm {
class Module;
template <typename T>
struct Array;
}  // namespace vm

namespace vm_modules {
namespace math {

class VMTensor;

class TensorEstimator
{
public:
  using VMObjectType = VMTensor;
  using ChargeAmount = fetch::vm::ChargeAmount;
  using TensorType   = typename fetch::math::Tensor<DataType>;

  explicit TensorEstimator(VMObjectType &tensor);
  ~TensorEstimator() = default;

  ChargeAmount size();

  ChargeAmount VMShape();

  ChargeAmount Copy();

  ChargeAmount AtOne(TensorType::SizeType idx1);

  ChargeAmount AtTwo(uint64_t idx1, uint64_t idx2);

  ChargeAmount AtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3);

  ChargeAmount AtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4);

  ChargeAmount SetAtOne(uint64_t idx1, DataType const &value);

  ChargeAmount SetAtTwo(uint64_t idx1, uint64_t idx2, DataType const &value);

  ChargeAmount SetAtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3, DataType const &value);

  ChargeAmount SetAtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4,
                         DataType const &value);

  ChargeAmount Fill(DataType const &value);

  ChargeAmount FillRandom();

  ChargeAmount Min();

  ChargeAmount Max();

  ChargeAmount Reshape(fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape);

  ChargeAmount Squeeze();

  ChargeAmount Sum();

  ChargeAmount ArgMax(SizeType const &indices);

  ChargeAmount ArgMaxNoIndices();

  ChargeAmount Dot(vm::Ptr<VMTensor> const &other);

  /// OPERATORS ///

  ChargeAmount NegateChargeEstimator(vm::Ptr<vm::Object> const & /*object*/);

  ChargeAmount IsEqualChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                      vm::Ptr<vm::Object> const &rhso);

  ChargeAmount IsNotEqualChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                         vm::Ptr<vm::Object> const &rhso);

  ChargeAmount AddChargeEstimator(vm::Ptr<vm::Object> const &lhso, vm::Ptr<vm::Object> const &rhso);

  ChargeAmount SubtractChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                       vm::Ptr<vm::Object> const &rhso);

  ChargeAmount InplaceAddChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                         vm::Ptr<vm::Object> const &rhso);

  ChargeAmount InplaceSubtractChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                              vm::Ptr<vm::Object> const &rhso);

  ChargeAmount MultiplyChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                       vm::Ptr<vm::Object> const &rhso);

  ChargeAmount DivideChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                     vm::Ptr<vm::Object> const &rhso);

  ChargeAmount InplaceMultiplyChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                              vm::Ptr<vm::Object> const &rhso);

  ChargeAmount InplaceDivideChargeEstimator(vm::Ptr<vm::Object> const &lhso,
                                            vm::Ptr<vm::Object> const &rhso);

  /// END OF OPERATORS ///

  ChargeAmount GetReshapeCost(SizeVector const &new_shape);

  ChargeAmount Transpose();

  ChargeAmount Unsqueeze();

  ChargeAmount FromString(fetch::vm::Ptr<fetch::vm::String> const &string);

  ChargeAmount ToString();

  // Fill
  static const fixed_point::fp64_t FILL_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t FILL_SIZE_COEF;
  static const fixed_point::fp64_t FILL_CONST_COEF;

  // FillRandom
  static const fixed_point::fp64_t FILL_RANDOM_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t FILL_RANDOM_SIZE_COEF;
  static const fixed_point::fp64_t FILL_RANDOM_CONST_COEF;

  // Min
  static const fixed_point::fp64_t MIN_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t MIN_SIZE_COEF;
  static const fixed_point::fp64_t MIN_CONST_COEF;

  // MAX
  static const fixed_point::fp64_t MAX_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t MAX_SIZE_COEF;
  static const fixed_point::fp64_t MAX_CONST_COEF;

  // SUM
  static const fixed_point::fp64_t SUM_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t SUM_SIZE_COEF;
  static const fixed_point::fp64_t SUM_CONST_COEF;

  // RESHAPE
  static const fixed_point::fp64_t RESHAPE_PADDED_SIZE_FROM_COEF;
  static const fixed_point::fp64_t RESHAPE_PADDED_SIZE_TO_COEF;
  static const fixed_point::fp64_t RESHAPE_CONST_COEF;

  // FROM_STRING
  static const fixed_point::fp64_t FROM_STRING_SIZE_COEF;
  static const fixed_point::fp64_t FROM_STRING_CONST_COEF;

  // TO_STRING
  static const fixed_point::fp64_t TO_STRING_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t TO_STRING_SIZE_COEF;
  static const fixed_point::fp64_t TO_STRING_CONST_COEF;

  // DOT
  static const fixed_point::fp64_t DOT_X_COEF;
  static const fixed_point::fp64_t DOT_Y_COEF;
  static const fixed_point::fp64_t DOT_C_COEF;
  static const fixed_point::fp64_t DOT_CUBIC_COEF;
  static const fixed_point::fp64_t DOT_CONST_COEF;

  // Negate
  static const fixed_point::fp64_t NEGATE_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t NEGATE_SIZE_COEF;
  static const fixed_point::fp64_t NEGATE_CONST_COEF;

  // IsEqual
  static const fixed_point::fp64_t IS_EQUAL_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t IS_EQUAL_SIZE_COEF;
  static const fixed_point::fp64_t IS_EQUAL_CONST_COEF;

  // IsNotEqual
  static const fixed_point::fp64_t IS_NOT_EQUAL_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t IS_NOT_EQUAL_SIZE_COEF;
  static const fixed_point::fp64_t IS_NOT_EQUAL_CONST_COEF;

  // Add
  static const fixed_point::fp64_t ADD_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t ADD_SIZE_COEF;
  static const fixed_point::fp64_t ADD_CONST_COEF;

  // InplaceAdd
  static const fixed_point::fp64_t INPLACE_ADD_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_ADD_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_ADD_CONST_COEF;

  // Subtract
  static const fixed_point::fp64_t SUBTRACT_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t SUBTRACT_SIZE_COEF;
  static const fixed_point::fp64_t SUBTRACT_CONST_COEF;

  // InplaceSubtract
  static const fixed_point::fp64_t INPLACE_SUBTRACT_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_SUBTRACT_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_SUBTRACT_CONST_COEF;

  // Multiply
  static const fixed_point::fp64_t MULTIPLY_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t MULTIPLY_SIZE_COEF;
  static const fixed_point::fp64_t MULTIPLY_CONST_COEF;

  // InplaceMultiply
  static const fixed_point::fp64_t INPLACE_MULTIPLY_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_MULTIPLY_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_MULTIPLY_CONST_COEF;

  // Divide
  static const fixed_point::fp64_t DIVIDE_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t DIVIDE_SIZE_COEF;
  static const fixed_point::fp64_t DIVIDE_CONST_COEF;

  // InplaceDivide
  static const fixed_point::fp64_t INPLACE_DIVIDE_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_DIVIDE_SIZE_COEF;
  static const fixed_point::fp64_t INPLACE_DIVIDE_CONST_COEF;

  // Copy
  static const fixed_point::fp64_t COPY_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t COPY_SIZE_COEF;
  static const fixed_point::fp64_t COPY_CONST_COEF;

  // ArgMax
  static const fixed_point::fp64_t ARGMAX_FIRST_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_FIRST_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_FIRST_CONST_COEF;

  static const fixed_point::fp64_t ARGMAX_MID_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_MID_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_MID_CONST_COEF;

  static const fixed_point::fp64_t ARGMAX_LAST_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_LAST_SIZE_COEF;
  static const fixed_point::fp64_t ARGMAX_LAST_CONST_COEF;

  // Default
  static const fixed_point::fp64_t DEFAULT_PADDED_SIZE_COEF;
  static const fixed_point::fp64_t DEFAULT_SIZE_COEF;
  static const fixed_point::fp64_t DEFAULT_CONST_COEF;

  // Function call overhead for LOW_CHARGE functions
  static constexpr SizeType LOW_CHARGE_CONST_COEF = 5;

private:
  static ChargeAmount const LOW_CHARGE{LOW_CHARGE_CONST_COEF * fetch::vm::COMPUTE_CHARGE_COST};

  static ChargeAmount MaximumCharge(std::string const &log_msg = "");

  static ChargeAmount ToChargeAmount(fixed_point::fp64_t const &val);

  VMObjectType &tensor_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
