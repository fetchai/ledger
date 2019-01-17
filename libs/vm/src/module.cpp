//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "vm/module.hpp"

namespace fetch {
namespace vm {

Module::Module()
{
  next_type_id_ = TypeIds::NumReserved;
  next_opcode_  = Opcodes::NumReserved;

  RegisterType(TypeIndex(typeid(T)),              TypeIds::Parameter1);
  RegisterType(TypeIndex(typeid(MapKey)),         TypeIds::Parameter1);
  RegisterType(TypeIndex(typeid(MapValue)),       TypeIds::Parameter2);

  RegisterType(TypeIndex(typeid(void)),           TypeIds::Void);
  RegisterType(TypeIndex(typeid(bool)),           TypeIds::Bool);
  RegisterType(TypeIndex(typeid(int8_t)),         TypeIds::Int8);
  RegisterType(TypeIndex(typeid(uint8_t)),        TypeIds::Byte);
  RegisterType(TypeIndex(typeid(int16_t)),        TypeIds::Int16);
  RegisterType(TypeIndex(typeid(uint16_t)),       TypeIds::UInt16);
  RegisterType(TypeIndex(typeid(int32_t)),        TypeIds::Int32);
  RegisterType(TypeIndex(typeid(uint32_t)),       TypeIds::UInt32);
  RegisterType(TypeIndex(typeid(int64_t)),        TypeIds::Int64);
  RegisterType(TypeIndex(typeid(uint64_t)),       TypeIds::UInt64);
  RegisterType(TypeIndex(typeid(float)),          TypeIds::Float32);
  RegisterType(TypeIndex(typeid(double)),         TypeIds::Float64);

  RegisterTemplateType<IMatrix>(TypeIds::IMatrix)
    .CreateTypeConstuctor<int32_t, int32_t>();

  RegisterTemplateType<IArray>(TypeIds::IArray)
    .CreateTypeConstuctor<int32_t>();

  RegisterTemplateType<IMap>(TypeIds::IMap)
    .CreateTypeConstuctor<>();

  RegisterClassType<String>(TypeIds::String);
}

} // namespace vm
} // namespace fetch
