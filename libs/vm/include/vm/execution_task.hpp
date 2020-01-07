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

#include "core/byte_array/const_byte_array.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

struct ExecutionTask
{
  using ConstByteArray = byte_array::ConstByteArray;

  std::string    function;
  ConstByteArray parameters;

  bool DeserializeParameters(vm::VM *vm, ParameterPack &params, Executable *exe,
                             Executable::Function const *f) const
  {
    vm->LoadExecutable(exe);

    // Finding the relevant function in the executable
    if (f == nullptr)
    {
      return false;
    }

    // Preparing serializer and return value.
    MsgPackSerializer serializer{parameters};

    // Extracting parameters
    auto const num_parameters = static_cast<std::size_t>(f->num_parameters);

    // loop through the parameters, type check and populate the stack
    for (std::size_t i = 0; i < num_parameters; ++i)
    {
      auto type_id = f->variables[i].type_id;
      if (type_id <= vm::TypeIds::PrimitiveMaxId)
      {
        Variant param;

        serializer >> param.primitive.i64;
        param.type_id = type_id;

        params.AddSingle(param);
      }
      else
      {
        // Checking if we can construct the object
        if (!vm->IsDefaultSerializeConstructable(type_id))
        {
          vm->UnloadExecutable();
          return false;
        }

        // Creating the object
        vm::Ptr<vm::Object> object  = vm->DefaultSerializeConstruct(type_id);
        auto                success = object->DeserializeFrom(serializer);

        // If deserialization failed we return
        if (!success)
        {
          vm->UnloadExecutable();
          return false;
        }

        // Adding the parameter to the parameter pack
        params.AddSingle(object);
      }
    }

    vm->UnloadExecutable();
    return true;
  }
};

}  // namespace vm
}  // namespace fetch
