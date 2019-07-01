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

#include "vm/sharded_state.hpp"

namespace fetch {
namespace vm {
namespace {
class ShardedState : public IShardedState
{
  std::string name_;
  TypeId      value_type_id_;

public:
  ShardedState(VM *vm, TypeId type_id, Ptr<String> const &name, TypeId value_type_id)
    : IShardedState(vm, type_id)
    , name_{name ? name->str : ""}
    , value_type_id_{value_type_id}
  {
    if (!name)
    {
      vm->RuntimeError("ShardedState: the `name` is null reference.");
    }
  }

protected:
  // TODO(issue 1259): Support for indexing operators will remain disabled for now just due to
  // keeping similarity of the interface with State interface.
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
    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_id_, ComposeFullKey(index))};
    return state->Get();
  }

  TemplateParameter1 GetIndexedValueInternal(Ptr<String> const &      index,
                                             TemplateParameter1 const default_value)
  {
    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_id_, ComposeFullKey(index))};
    return state->Get(default_value);
  }

  void SetIndexedValueInternal(Ptr<String> const &index, TemplateParameter1 const &value_v)
  {
    auto state{
        IState::ConstructIntrinsic(vm_, TypeIds::Unknown, value_type_id_, ComposeFullKey(index))};
    state->Set(value_v);
  }

  TemplateParameter1 Get(Ptr<String> const &key) override
  {
    return GetIndexedValueInternal(key);
  }

  TemplateParameter1 Get(Ptr<Address> const &key) override
  {
    if (!key)
    {
      RuntimeError("Index is null reference.");
      return {};
    }
    return GetIndexedValueInternal(key->AsString());
  }

  TemplateParameter1 Get(Ptr<String> const &key, TemplateParameter1 const &default_value) override
  {
    return GetIndexedValueInternal(key, default_value);
  }

  TemplateParameter1 Get(Ptr<Address> const &key, TemplateParameter1 const &default_value) override
  {
    if (!key)
    {
      RuntimeError("Index is null reference.");
      return {};
    }
    return GetIndexedValueInternal(key->AsString(), default_value);
  }

  void Set(Ptr<String> const &key, TemplateParameter1 const &value) override
  {
    SetIndexedValueInternal(key, value);
  }

  void Set(Ptr<Address> const &key, TemplateParameter1 const &value) override
  {
    if (!key)
    {
      RuntimeError("Index is null reference.");
    }
    SetIndexedValueInternal(key->AsString(), value);
  }
};

}  // namespace

Ptr<IShardedState> IShardedState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name)
{
  if (name)
  {
    TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
    TypeId const    value_type_id = type_info.parameter_type_ids[0];
    return new ShardedState(vm, type_id, name, value_type_id);
  }

  vm->RuntimeError("Failed to construct ShardedState instance: the `name` is null reference.");
  return nullptr;
}

Ptr<IShardedState> IShardedState::Constructor(VM *vm, TypeId type_id, Ptr<Address> const &name)
{
  return Constructor(vm, type_id, name ? name->AsString() : nullptr);
}

}  // namespace vm
}  // namespace fetch
