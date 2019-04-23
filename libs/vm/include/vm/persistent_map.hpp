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
  static Ptr<IPersistentMap> Constructor(VM *vm, TypeId type_id, Ptr<Object> const &name);

  // Construction / Destruction
  IPersistentMap(VM *vm, TypeId type_id, Ptr<String> name, TypeId value_type)
    : Object(vm, type_id)
    , name_{std::move(name)}
    , value_type_{value_type}
  {}

  IPersistentMap(VM *vm, TypeId type_id, Ptr<Address> addr, TypeId value_type)
    : IPersistentMap(vm, type_id, addr->AsBase64String(), value_type)
  {}

  ~IPersistentMap() override = default;

protected:
  Ptr<String> name_;
  TypeId      value_type_;
};

class PersistentMap : public IPersistentMap
{
public:
  // Construction / Destruction
  PersistentMap(VM *vm, TypeId type_id, Ptr<String> name, TypeId value_type)
    : IPersistentMap(vm, type_id, std::move(name), value_type)
  {}
  ~PersistentMap() override = default;

protected:
  using ByteBuffer = std::vector<uint8_t>;

  Ptr<String> ExtractKey(Variant &key_v)
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

    return new String{vm_, name_->str + "." + key->str};
  }

  void PushElement(TypeId /*element_type_id*/) override
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return;
    }

    Variant &  key_v = Pop();
    auto const key{ExtractKey(key_v)};
    key_v.Reset();
    if (!key)
    {
      RuntimeError("Extraction of key value failed.");
      return;
    }
    // std::cout << "Resource would be: " << key->str << std::endl;

    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::IState, value_type_, key, TemplateParameter{})};
    auto value = state->Get();

    Variant &value_v = Push();
    value_v.Construct(value);
  }

  void PopToElement() override
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return;
    }

    Variant &  key_v = Pop();
    auto const key{ExtractKey(key_v)};
    key_v.Reset();
    if (!key)
    {
      RuntimeError("Extraction of key value failed.");
      return;
    }

    Variant &value_v = Pop();
    if (value_type_ != value_v.type_id)
    {
      RuntimeError("Incorrect value type for PersistentMap<...> type.");
    }
    else
    {
      // std::cout << "Resource would be: " << key->str << std::endl;
      auto state{
          IState::ConstructIntrinsic(vm_, TypeIds::IState, value_type_, key, TemplateParameter{})};
      state->Set(TemplateParameter{value_v});
    }
    value_v.Reset();
  }
};

inline Ptr<IPersistentMap> IPersistentMap::Constructor(VM *vm, TypeId type_id,
                                                       Ptr<Object> const &name)
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

  return new PersistentMap(vm, type_id, name, value_type_id);
}

}  // namespace vm
}  // namespace fetch
