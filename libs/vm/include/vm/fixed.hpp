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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/common.hpp"
#include "vm/object.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace fetch {
namespace vm {

class VM;

struct Fixed128 : public Object
{
  Fixed128()           = delete;
  ~Fixed128() override = default;

  Fixed128(VM *vm, fixed_point::fp128_t const &data);

  Fixed128(vm::VM *vm, vm::TypeId type_id, fixed_point::fp128_t const &data);

  Fixed128(vm::VM *vm, vm::TypeId type_id, byte_array::ByteArray const &data);

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  bool IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  bool IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  bool IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  bool IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  bool IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override;
  void InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;

  void Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso) override;
  void InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  void Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso) override;
  void InplaceMultiply(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  void Divide(Ptr<Object> &lhso, Ptr<Object> &rhso) override;
  void InplaceDivide(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  void Negate(Ptr<Object> &object) override;

  bool SerializeTo(MsgPackSerializer &buffer) override;
  bool DeserializeFrom(MsgPackSerializer &buffer) override;

  fixed_point::fp128_t data_;
};

}  // namespace vm
}  // namespace fetch
