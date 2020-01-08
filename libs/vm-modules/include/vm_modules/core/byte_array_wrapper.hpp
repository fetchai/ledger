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

#include "vm/object.hpp"
#include "vm/string.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {

class ByteArrayWrapper : public fetch::vm::Object
{
public:
  ByteArrayWrapper()           = delete;
  ~ByteArrayWrapper() override = default;

  static void Bind(fetch::vm::Module &module);

  ByteArrayWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray bytearray);

  static fetch::vm::Ptr<ByteArrayWrapper> Constructor(vm::VM *vm, vm::TypeId type_id, int32_t n);

  fetch::vm::Ptr<ByteArrayWrapper> Copy();
  fetch::vm::Ptr<vm::String>       ToBase64();
  bool                             FromBase64(fetch::vm::Ptr<vm::String> const &value_b64);
  fetch::vm::Ptr<vm::String>       ToHex();
  bool                             FromHex(fetch::vm::Ptr<vm::String> const &value_hex);
  fetch::vm::Ptr<vm::String>       ToBase58();
  bool                             FromBase58(fetch::vm::Ptr<vm::String> const &value_b58);

  bool IsEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsNotEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsLessThan(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;
  bool IsGreaterThan(fetch::vm::Ptr<Object> const &lhso,
                     fetch::vm::Ptr<Object> const &rhso) override;
  bool IsLessThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                         fetch::vm::Ptr<Object> const &rhso) override;
  bool IsGreaterThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                            fetch::vm::Ptr<Object> const &rhso) override;

  vm::ChargeAmount IsEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                          fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsNotEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                             fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsLessThanChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                             fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsGreaterThanChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsLessThanOrEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                    fetch::vm::Ptr<Object> const &rhso) override;
  vm::ChargeAmount IsGreaterThanOrEqualChargeEstimator(fetch::vm::Ptr<Object> const &lhso,
                                                       fetch::vm::Ptr<Object> const &rhso) override;

  byte_array::ConstByteArray const &byte_array() const;

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

private:
  byte_array::ByteArray byte_array_;
};

}  // namespace vm_modules
}  // namespace fetch
