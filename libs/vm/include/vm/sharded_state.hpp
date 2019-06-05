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

  virtual TemplateParameter1 GetIndexedValue(Ptr<String> const &key)                    = 0;
  virtual void SetIndexedValue(Ptr<String> const &key, TemplateParameter1 const &value) = 0;

  virtual TemplateParameter1 GetIndexedValue(Ptr<Address> const &key)                    = 0;
  virtual void SetIndexedValue(Ptr<Address> const &key, TemplateParameter1 const &value) = 0;

protected:
  std::string name_;
  TypeId      value_type_;
};

}  // namespace vm
}  // namespace fetch
