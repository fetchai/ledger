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

#include "vm/module.hpp"

namespace fetch {
namespace vm {

Module::Module()
{
  next_type_id_ = TypeIds::NumReserved;
  next_opcode_  = Opcodes::NumReserved;

  RegisterType(TypeIndex(typeid(TemplateParameter)), TypeIds::TemplateParameter1);
  RegisterType(TypeIndex(typeid(TemplateParameter1)), TypeIds::TemplateParameter1);
  RegisterType(TypeIndex(typeid(TemplateParameter2)), TypeIds::TemplateParameter2);

  RegisterType(TypeIndex(typeid(void)), TypeIds::Void);
  RegisterType(TypeIndex(typeid(bool)), TypeIds::Bool);
  RegisterType(TypeIndex(typeid(int8_t)), TypeIds::Int8);
  RegisterType(TypeIndex(typeid(uint8_t)), TypeIds::Byte);
  RegisterType(TypeIndex(typeid(int16_t)), TypeIds::Int16);
  RegisterType(TypeIndex(typeid(uint16_t)), TypeIds::UInt16);
  RegisterType(TypeIndex(typeid(int32_t)), TypeIds::Int32);
  RegisterType(TypeIndex(typeid(uint32_t)), TypeIds::UInt32);
  RegisterType(TypeIndex(typeid(int64_t)), TypeIds::Int64);
  RegisterType(TypeIndex(typeid(uint64_t)), TypeIds::UInt64);
  RegisterType(TypeIndex(typeid(float)), TypeIds::Float32);
  RegisterType(TypeIndex(typeid(double)), TypeIds::Float64);
  RegisterClassType<String>(TypeIds::String);

  RegisterTemplateType<IMatrix>(TypeIds::IMatrix).CreateTypeConstuctor<int32_t, int32_t>();

  auto iarray = RegisterTemplateType<IArray>(TypeIds::IArray);
  iarray.CreateTypeConstuctor<int32_t>();
  iarray.CreateInstanceFunction("count", &IArray::Count);

  CreateTemplateInstantiationType<Array, bool>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, int8_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, uint8_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, int16_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, uint16_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, int32_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, uint32_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, int64_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, uint64_t>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, float>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, double>(TypeIds::IArray);
  CreateTemplateInstantiationType<Array, Ptr<String>>(TypeIds::IArray);

  auto imap = RegisterTemplateType<IMap>(TypeIds::IMap);
  imap.CreateTypeConstuctor<>();
  imap.CreateInstanceFunction("count", &IMap::Count);

  auto address = RegisterClassType<Address>(TypeIds::Address);
  address.CreateTypeConstuctor<>();
  address.CreateInstanceFunction("signed_tx", &Address::HasSignedTx);
  address.CreateInstanceFunction("AsString", &Address::AsString);

  auto istate = RegisterTemplateType<IState>(TypeIds::IState);
  istate.CreateTypeConstuctor<Ptr<String>, TemplateParameter>();
  istate.CreateInstanceFunction("get", &IState::Get);
  istate.CreateInstanceFunction("set", &IState::Set);
  istate.CreateInstanceFunction("existed", &IState::Existed);
}

}  // namespace vm
}  // namespace fetch
