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

#include "math/base_types.hpp"
#include "vectorise/uint/uint.hpp"
#include "vm/object.hpp"

#include <cstdint>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {

class ByteArrayWrapper;

namespace math {

class UInt256Wrapper : public fetch::vm::Object
{
public:
  using UInt256 = vectorise::UInt<256>;

  UInt256Wrapper()           = delete;
  ~UInt256Wrapper() override = default;

  static void Bind(fetch::vm::Module &module);

  explicit UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, UInt256 data);

  explicit UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t data);

  explicit UInt256Wrapper(fetch::vm::VM *vm, UInt256 data);

  explicit UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                          byte_array::ConstByteArray const &data,
                          platform::Endian                  endianess_of_input_data);

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                    uint64_t val);

  vm::Ptr<UInt256Wrapper> Copy() const;

  fetch::math::SizeType size() const;

  fetch::vectorise::UInt<256> const &number() const;

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  bool ToJSON(fetch::vm::JSONVariant &variant) override;

  bool FromJSON(fetch::vm::JSONVariant const &variant) override;

  void Add(fetch::vm::Ptr<Object> &lhso, fetch::vm::Ptr<Object> &rhso) override;
  void InplaceAdd(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  void Subtract(fetch::vm::Ptr<Object> &lhso, fetch::vm::Ptr<Object> &rhso) override;
  void InplaceSubtract(fetch::vm::Ptr<Object> const &lhso,
                       fetch::vm::Ptr<Object> const &rhso) override;
  void Multiply(fetch::vm::Ptr<Object> &lhso, fetch::vm::Ptr<Object> &rhso) override;
  void InplaceMultiply(fetch::vm::Ptr<Object> const &lhso,
                       fetch::vm::Ptr<Object> const &rhso) override;
  void Divide(fetch::vm::Ptr<Object> &lhso, fetch::vm::Ptr<Object> &rhso) override;
  void InplaceDivide(fetch::vm::Ptr<Object> const &lhso,
                     fetch::vm::Ptr<Object> const &rhso) override;
  bool IsEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsNotEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsLessThan(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsGreaterThan(fetch::vm::Ptr<Object> const &lhso,
                     fetch::vm::Ptr<Object> const &rhso) override;

  bool IsLessThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                         fetch::vm::Ptr<Object> const &rhso) override;

  bool             IsGreaterThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                        fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount AddChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                      fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount InplaceAddChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                             fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount SubtractChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                           fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount InplaceSubtractChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                  fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount MultiplyChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                           fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount InplaceMultiplyChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                  fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount DivideChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                         fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount InplaceDivideChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                          fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsNotEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                             fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsLessThanChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                             fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsLessThanOrEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                    fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsGreaterThanChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsGreaterThanOrEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                       fetch::vm::Ptr<Object> const &rhso) override;

private:
  UInt256 number_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
