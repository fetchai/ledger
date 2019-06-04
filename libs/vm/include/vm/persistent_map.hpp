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

class IShardedState : public Object
{
public:
  // Factory
  static Ptr<IShardedState> Constructor(VM *vm, TypeId type_id, Ptr<Object> name);

  // Construction / Destruction
  IShardedState(VM *vm, TypeId type_id, Ptr<Object> name, TypeId value_type)
    : Object(vm, type_id)
    , name_{IState::NameToString(vm, std::move(name))}
    , value_type_{value_type}
  {}

  ~IShardedState() = default;

  virtual TemplateParameter1 GetIndexedValue(Ptr<String> const &key)                    = 0;
  virtual void SetIndexedValue(Ptr<String> const &key, TemplateParameter1 const &value) = 0;

  virtual TemplateParameter1 GetIndexedValue(Ptr<Address> const &key)                    = 0;
  virtual void SetIndexedValue(Ptr<Address> const &key, TemplateParameter1 const &value) = 0;

protected:
  std::string name_;
  TypeId      value_type_;
};

class ShardedState : public IShardedState
{
public:
  // Construction / Destruction
  ShardedState(VM *vm, TypeId type_id, Ptr<Object> name, TypeId value_type)
    : IShardedState(vm, type_id, std::move(name), value_type)
  {}

protected:
  using ByteBuffer = std::vector<uint8_t>;

  TemplateParameter1 GetIndexedValue(Ptr<String> const &key) override
  {
    return GetIndexedValueInternal(key);
  }

  void SetIndexedValue(Ptr<String> const &key, TemplateParameter1 const &value) override
  {
    SetIndexedValueInternal(key, value);
  }

  TemplateParameter1 GetIndexedValue(Ptr<Address> const &key) override
  {
    if (!key)
    {
      RuntimeError("Index is null reference.");
      return {};
    }
    return GetIndexedValueInternal(key->AsString());
  }

  void SetIndexedValue(Ptr<Address> const &key, TemplateParameter1 const &value) override
  {
    if (!key)
    {
      RuntimeError("Index is null reference.");
    }
    SetIndexedValueInternal(key->AsString(), value);
  }

private:
  Ptr<String> ComposeFullKey(Ptr<String> const &key)
  {
    if (!key)
    {
      RuntimeError("Key is null reference.");
      return nullptr;
    }

    return new String{vm_, name_ + "." + key->str};
  }

  TemplateParameter1 GetIndexedValueInternal(Ptr<String> const &index)
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return {};
    }

    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_, ComposeFullKey(index), TemplateParameter1{})};
    return state->Get();
  }

  void SetIndexedValueInternal(Ptr<String> const &index, TemplateParameter1 const &value_v)
  {
    if (!vm_->HasIoObserver())
    {
      RuntimeError("No IOObserver registered in VM.");
      return;
    }

    auto state {IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_, ComposeFullKey(index), value_v)};
    state->Set(value_v);
  }
};

inline Ptr<IShardedState> IShardedState::Constructor(VM *vm, TypeId type_id, Ptr<Object> name)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    value_type_id = type_info.parameter_type_ids[0];

  return new ShardedState(vm, type_id, std::move(name), value_type_id);
}

}  // namespace vm
}  // namespace fetch
