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

#include "vm/user_defined_object.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

UserDefinedObject::UserDefinedObject(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  Executable::UserDefinedType const &user_defined_type = vm_->GetUserDefinedType(type_id);
  std::size_t                        num_variables     = user_defined_type.variables.size();
  variables_                                           = VariantArray(num_variables);
  for (std::size_t i = 0; i < num_variables; ++i)
  {
    Executable::Variable const &exe_variable = user_defined_type.variables[i];
    Variant &                   variable     = variables_[i];
    if (exe_variable.type_id > TypeIds::PrimitiveMaxId)
    {
      variable.Construct(Ptr<Object>(), exe_variable.type_id);
    }
    else
    {
      Primitive p{0u};
      variable.Construct(p, exe_variable.type_id);
    }
  }
}

Variant &UserDefinedObject::GetVariable(uint16_t index)
{
  assert(index < variables_.size());
  return variables_[index];
}

bool UserDefinedObject::SerializeTo(MsgPackSerializer & /* buffer */)
{
  return true;
}

bool UserDefinedObject::DeserializeFrom(MsgPackSerializer & /* buffer */)
{
  return true;
}

}  // namespace vm
}  // namespace fetch
