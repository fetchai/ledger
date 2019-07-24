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
  static Ptr<IShardedState> Constructor(VM *vm, TypeId type_id, Ptr<String> const &name);
  static Ptr<IShardedState> Constructor(VM *vm, TypeId type_id, Ptr<Address> const &name);

  IShardedState(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}

  // TODO(issue 1259): Support for indexing operators will remain disabled for now just due to
  // keeping similarity of the interface with State interface..
  virtual TemplateParameter1 GetIndexedValue(Ptr<String> const &key)                     = 0;
  virtual void SetIndexedValue(Ptr<String> const &key, TemplateParameter1 const &value)  = 0;
  virtual TemplateParameter1 GetIndexedValue(Ptr<Address> const &key)                    = 0;
  virtual void SetIndexedValue(Ptr<Address> const &key, TemplateParameter1 const &value) = 0;

  virtual TemplateParameter1 GetFromString(Ptr<String> const &key)                              = 0;
  virtual TemplateParameter1 GetFromAddress(Ptr<Address> const &key)                            = 0;
  virtual TemplateParameter1 GetFromStringWithDefault(Ptr<String> const &       key,
                                                      TemplateParameter1 const &default_value)  = 0;
  virtual TemplateParameter1 GetFromAddressWithDefault(Ptr<Address> const &      key,
                                                       TemplateParameter1 const &default_value) = 0;
  virtual void SetFromString(Ptr<String> const &key, TemplateParameter1 const &value)           = 0;
  virtual void SetFromAddress(Ptr<Address> const &key, TemplateParameter1 const &value)         = 0;
};

}  // namespace vm
}  // namespace fetch
