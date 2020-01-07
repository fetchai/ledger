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

#include "variant/variant.hpp"
#include "vm/address.hpp"
#include "vm/fixed.hpp"
#include "vm/object.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/math/bignumber.hpp"

namespace fetch {

namespace vm {
template <typename T>
struct Array;
}

namespace vm_modules {

class StructuredData : public vm::Object
{
public:
  template <typename T, typename Y = void>
  static constexpr bool IsSupportedRefType =
      type_util::IsAnyOfV<meta::Decay<T>, vm::String, vm::Address, vm::Fixed128,
                          vm_modules::ByteArrayWrapper, vm_modules::math::UInt256Wrapper>;

  template <typename T, typename Y = void>
  using IfIsSupportedRefType = meta::EnableIf<IsSupportedRefType<T>, Y>;

  static void                    Bind(vm::Module &module);
  static vm::Ptr<StructuredData> Constructor(vm::VM *vm, vm::TypeId type_id);
  static vm::Ptr<StructuredData> ConstructorFromVariant(vm::VM *vm, vm::TypeId type_id,
                                                        variant::Variant const &data);

  StructuredData() = delete;
  StructuredData(vm::VM *vm, vm::TypeId type_id);
  ~StructuredData() override = default;

protected:
  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;
  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  bool ToJSON(vm::JSONVariant &variant) override;
  bool FromJSON(vm::JSONVariant const &variant) override;

private:
  bool Has(vm::Ptr<vm::String> const &s);

  template <typename T>
  T GetPrimitive(vm::Ptr<vm::String> const &s);
  template <typename T>
  IfIsSupportedRefType<T, vm::Ptr<T>> GetObject(vm::Ptr<vm::String> const &s);
  template <typename T>
  vm::Ptr<vm::Array<T>> GetArray(vm::Ptr<vm::String> const &s);
  template <typename T>
  IfIsSupportedRefType<T, vm::Ptr<vm::Array<vm::Ptr<T>>>> GetObjectArray(
      vm::Ptr<vm::String> const &s);

  template <typename T>
  void SetPrimitive(vm::Ptr<vm::String> const &s, T value);
  template <typename T>
  IfIsSupportedRefType<T> SetObject(vm::Ptr<vm::String> const &s, vm::Ptr<T> const &value);
  template <typename T>
  void SetArray(vm::Ptr<vm::String> const &s, vm::Ptr<vm::Array<T>> const &arr);
  template <typename T>
  void SetObjectArray(vm::Ptr<vm::String> const &s, vm::Ptr<vm::Array<vm::Ptr<T>>> const &arr);

  // Data
  variant::Variant contents_{variant::Variant::Object()};
};

}  // namespace vm_modules
}  // namespace fetch
