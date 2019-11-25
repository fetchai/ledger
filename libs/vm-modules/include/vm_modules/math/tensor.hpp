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

class VMTensor : public fetch::vm::Object
{
public:
  using DataType   = fetch::vm_modules::math::DataType;
  using TensorType = typename fetch::math::Tensor<DataType>;

  class TensorEstimator
  {
  public:
    using ObjectType   = VMTensor;
    using ChargeAmount = fetch::vm::ChargeAmount;

    explicit TensorEstimator(VMTensor &tensor);
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

    ChargeAmount Squeeze();

    ChargeAmount Unsqueeze();

    ChargeAmount Reshape(fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape);

    ChargeAmount Transpose();

    ChargeAmount FromString(fetch::vm::Ptr<fetch::vm::String> const &string);

    ChargeAmount ToString();

  private:
    ChargeAmount low_charge{fetch::vm::CHARGE_UNIT};
    ChargeAmount charge_func_of_tensor_size(size_t factor = 1);

    VMTensor &tensor_;
  };

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::vector<uint64_t> const &shape);

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, TensorType tensor);

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMTensor> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &shape);

  static void Bind(fetch::vm::Module &module);

  TensorType::SizeVector shape() const;

  TensorType::SizeType size() const;

  ////////////////////////////////////
  /// ACCESSING AND SETTING VALUES ///
  ////////////////////////////////////

  DataType AtOne(TensorType::SizeType idx1) const;

  DataType AtTwo(uint64_t idx1, uint64_t idx2) const;

  DataType AtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3) const;

  DataType AtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4) const;

  void SetAtOne(uint64_t idx1, DataType const &value);

  void SetAtTwo(uint64_t idx1, uint64_t idx2, DataType const &value);

  void SetAtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3, DataType const &value);

  void SetAtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4, DataType const &value);

  void Copy(TensorType const &other);

  void Fill(DataType const &value);

  void FillRandom();

  /////////////////
  /// RESHAPING ///
  /////////////////

  fetch::vm::Ptr<VMTensor> Squeeze();

  fetch::vm::Ptr<VMTensor> Unsqueeze();

  bool Reshape(fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &new_shape);

  void Transpose();

  //////////////////////////////
  /// PRINTING AND EXPORTING ///
  //////////////////////////////

  void FromString(fetch::vm::Ptr<fetch::vm::String> const &string);

  fetch::vm::Ptr<fetch::vm::String> ToString() const;

  TensorType &GetTensor();

  TensorType const &GetConstTensor();

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  TensorEstimator &Estimator();

private:
  TensorType      tensor_;
  TensorEstimator estimator_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
