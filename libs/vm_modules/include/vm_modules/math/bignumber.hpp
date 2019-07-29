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
  using SizeType = uint64_t;
  using UInt256  = vectorise::UInt<256>;

  UInt256Wrapper()           = delete;
  ~UInt256Wrapper() override = default;

  static fetch::vm::Ptr<fetch::vm::String> ToString(fetch::vm::VM *                       vm,
                                                    fetch::vm::Ptr<UInt256Wrapper> const &n);

  template <typename T>
  static T ToPrimitive(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<UInt256Wrapper> const &a);

  static void Bind(fetch::vm::Module &module);

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, UInt256 data);

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray const &data);

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t data);

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                    uint64_t val);

  static fetch::vm::Ptr<UInt256Wrapper> ConstructorFromBytes(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<ByteArrayWrapper> const &ba);

  double ToFloat64() const;

  int32_t ToInt32() const;

  double LogValue() const;

  bool LessThan(fetch::vm::Ptr<UInt256Wrapper> const &other) const;

  void Increase();

  SizeType size() const;

  fetch::vectorise::UInt<256> const &number() const;

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  bool ToJSON(fetch::vm::JSONVariant &variant) override;

  bool FromJSON(fetch::vm::JSONVariant const &variant) override;

  bool IsEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;

  bool IsNotEqual(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;

  bool IsLessThan(fetch::vm::Ptr<Object> const &lhso, fetch::vm::Ptr<Object> const &rhso) override;

  bool IsGreaterThan(fetch::vm::Ptr<Object> const &lhso,
                     fetch::vm::Ptr<Object> const &rhso) override;

private:
  UInt256 number_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
