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

#include "math/tensor.hpp"
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

  ChargeAmount Squeeze();

  ChargeAmount Unsqueeze();

  ChargeAmount Reshape(fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape);

  ChargeAmount GetReshapeCost(SizeVector const &new_shape);

  ChargeAmount Sum();

  ChargeAmount Transpose();

  ChargeAmount FromString(fetch::vm::Ptr<fetch::vm::String> const &string);

  ChargeAmount ToString();

  // Fill
  static constexpr DataType FILL_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType FILL_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType FILL_CONST_COEF()
  {
    return DataType(5);
  };

  // FillRandom
  static constexpr DataType FILL_RANDOM_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType FILL_RANDOM_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType FILL_RANDOM_CONST_COEF()
  {
    return DataType(5);
  };

  // Min
  static constexpr DataType MIN_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType MIN_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType MIN_CONST_COEF()
  {
    return DataType(5);
  };

  // MAX
  static constexpr DataType MAX_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType MAX_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType MAX_CONST_COEF()
  {
    return DataType(5);
  };

  // SUM
  static constexpr DataType SUM_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType SUM_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType SUM_CONST_COEF()
  {
    return DataType(5);
  };

  // RESHAPE
  static constexpr DataType RESHAPE_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType RESHAPE_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType RESHAPE_CONST_COEF()
  {
    return DataType(5);
  };

  // FROM_STRING
  static constexpr DataType FROM_STRING_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType FROM_STRING_CONST_COEF()
  {
    return DataType(5);
  };

  // TO_STRING
  static constexpr DataType TO_STRING_PADDED_SIZE_COEF()
  {
    return DataType(0.00023451);
  };
  static constexpr DataType TO_STRING_SIZE_COEF()
  {
    return DataType(0.00107809);
  };
  static constexpr DataType TO_STRING_CONST_COEF()
  {
    return DataType(5);
  };

  // Function call overhead for LOW_CHARGE functions
  static constexpr SizeType LOW_CHARGE_CONST_COEF = 5;

private:
  static ChargeAmount const LOW_CHARGE{LOW_CHARGE_CONST_COEF * fetch::vm::COMPUTE_CHARGE_COST};

  static ChargeAmount infinite_charge(std::string const &log_msg = "");

  VMObjectType &tensor_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
