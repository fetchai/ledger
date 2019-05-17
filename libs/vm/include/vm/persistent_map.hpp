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

#include "core/macros.hpp"
#include "vm/state.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IPersistentMap : public Object
{
public:
  // Factory
  static Ptr<IPersistentMap> Constructor(VM *vm, TypeId type_id, Ptr<Object> name);

  // Construction / Destruction
  IPersistentMap(VM *vm, TypeId type_id, Ptr<Object> name, TypeId value_type)
    : Object(vm, type_id)
    , name_{IState::NameToString(vm, std::move(name))}
    , value_type_{value_type}
  {}

  ~IPersistentMap() = default;

  virtual TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key)                    = 0;
  virtual void SetIndexedValue(TemplateParameter1 const &key, TemplateParameter2 const &value) = 0;

protected:
  std::string name_;
  TypeId      value_type_;
};

class PersistentMap : public IPersistentMap
{
public:
  // Construction / Destruction
  PersistentMap(VM *vm, TypeId type_id, Ptr<Object> name, TypeId value_type)
    : IPersistentMap(vm, type_id, std::move(name), value_type)
  {}
  ~PersistentMap() override = default;

protected:
  using ByteBuffer = std::vector<uint8_t>;

  Ptr<String> ExtractKey(Variant const &key_v)
  {
    Ptr<String> key;

    switch (key_v.type_id)
    {
    case TypeIds::String:
      key = key_v.Get<Ptr<String>>();
      break;

    case TypeIds::Address:
      key = key_v.Get<Ptr<Address>>()->AsBase64String();
      break;

    default:
      RuntimeError("Unexpected type of key value. It must be either String or Address.");
      return nullptr;
    }

    return new String{vm_, name_ + "." + key->str};
  }

  TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key_v) override
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return {};
    }

    auto const key{ExtractKey(key_v)};
    if (!key)
    {
      RuntimeError("Extraction of key value failed.");
      return {};
    }

    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_, key, TemplateParameter{})};
    auto value = state->Get();

    return {value};
  }

  void SetIndexedValue(TemplateParameter1 const &key_v, TemplateParameter2 const &value_v) override
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return;
    }

    auto const key{ExtractKey(key_v)};
    if (!key)
    {
      RuntimeError("Extraction of key value failed.");
      return;
    }

    if (value_type_ != value_v.type_id)
    {
      RuntimeError("Incorrect value type for PersistentMap<...> type.");
    }
    else
    {
      auto state{
          IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_, key, TemplateParameter{})};
      state->Set(TemplateParameter{value_v});
    }
  }
};

inline Ptr<IPersistentMap> IPersistentMap::Constructor(VM *vm, TypeId type_id, Ptr<Object> name)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    key_type_id   = type_info.parameter_type_ids[0];
  TypeId const    value_type_id = type_info.parameter_type_ids[1];

  // the template parameters are restricted for the time being
  bool const valid_key = (TypeIds::String == key_type_id) || (TypeIds::Address == key_type_id);

  if (!valid_key)
  {
    vm->RuntimeError("Incompatible key type");
    return nullptr;
  }

  return new PersistentMap(vm, type_id, std::move(name), value_type_id);
}

}  // namespace vm
}  // namespace fetch
