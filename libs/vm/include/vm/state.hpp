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

#include "core/serializers/byte_array_buffer.hpp"
#include "vm/address.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IState : public Object
{
public:
  IState()           = delete;
  ~IState() override = default;

  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<String> const &name);
  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<Address> const &name);

  static Ptr<IState> ConstructIntrinsic(VM *vm, TypeId type_id, TypeId template_param_type_id,
                                        Ptr<String> const &name);

  virtual TemplateParameter1 Get()                                        = 0;
  virtual TemplateParameter1 Get(TemplateParameter1 const &default_value) = 0;
  virtual void               Set(TemplateParameter1 const &value)         = 0;
  virtual bool               Existed()                                    = 0;

protected:
  IState(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

}  // namespace vm
}  // namespace fetch
