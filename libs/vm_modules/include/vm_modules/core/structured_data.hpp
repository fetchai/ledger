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

#include "variant/variant.hpp"
#include "vm/object.hpp"

namespace fetch {

namespace vm {
template <typename T>
struct Array;
}

namespace vm_modules {

class StructuredData : public vm::Object
{
public:
  static void                    Bind(vm::Module &module);
  static vm::Ptr<StructuredData> Constructor(vm::VM *vm, vm::TypeId type_id);
  static vm::Ptr<StructuredData> Constructor(vm::VM *vm, vm::TypeId type_id,
                                             variant::Variant const &data);

  StructuredData() = delete;
  StructuredData(vm::VM *vm, vm::TypeId type_id);
  ~StructuredData() override = default;

protected:
  bool SerializeTo(vm::ByteArrayBuffer &buffer) override;
  bool DeserializeFrom(vm::ByteArrayBuffer &buffer) override;

  bool ToJSON(vm::JSONVariant &variant) override;
  bool FromJSON(vm::JSONVariant const &variant) override;

private:
  bool Has(vm::Ptr<vm::String> const &s);

  vm::Ptr<vm::String> GetString(vm::Ptr<vm::String> const &s);
  template <typename T>
  T GetPrimitive(vm::Ptr<vm::String> const &s);
  template <typename T>
  vm::Ptr<vm::Array<T>> GetArray(vm::Ptr<vm::String> const &s);

  template <typename T>
  void SetPrimitive(vm::Ptr<vm::String> const &s, T value);
  template <typename T>
  void SetArray(vm::Ptr<vm::String> const &s, vm::Ptr<vm::Array<T>> const &arr);
  void SetString(vm::Ptr<vm::String> const &s, vm::Ptr<vm::String> const &value);

  // Data
  variant::Variant contents_{variant::Variant::Object()};
};

}  // namespace vm_modules
}  // namespace fetch
