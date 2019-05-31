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
#include "vm/persistent_map.hpp"

#include <cstdint>

namespace fetch {
namespace vm {

template <typename To>
To Cast(Variant const &from)
{
  To to;
  switch (from.type_id)
  {
  case TypeIds::Bool:
  {
    to = static_cast<To>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int8:
  {
    to = static_cast<To>(from.primitive.i8);
    break;
  }
  case TypeIds::Byte:
  {
    to = static_cast<To>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int16:
  {
    to = static_cast<To>(from.primitive.i16);
    break;
  }
  case TypeIds::UInt16:
  {
    to = static_cast<To>(from.primitive.ui16);
    break;
  }
  case TypeIds::Int32:
  {
    to = static_cast<To>(from.primitive.i32);
    break;
  }
  case TypeIds::UInt32:
  {
    to = static_cast<To>(from.primitive.ui32);
    break;
  }
  case TypeIds::Int64:
  {
    to = static_cast<To>(from.primitive.i64);
    break;
  }
  case TypeIds::UInt64:
  {
    to = static_cast<To>(from.primitive.ui64);
    break;
  }
  case TypeIds::Float32:
  {
    to = static_cast<To>(from.primitive.f32);
    break;
  }
  case TypeIds::Float64:
  {
    to = static_cast<To>(from.primitive.f64);
    break;
  }
  default:
  {
    to = 0;
    break;
  }
  }  // switch
  return to;
}

int8_t toInt8(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int8_t>(from);
}

uint8_t toByte(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint8_t>(from);
}

int16_t toInt16(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int16_t>(from);
}

uint16_t toUInt16(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint16_t>(from);
}

int32_t toInt32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int32_t>(from);
}

uint32_t toUInt32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint32_t>(from);
}

int64_t toInt64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int64_t>(from);
}

uint64_t toUInt64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<uint64_t>(from);
}

float toFloat32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<float>(from);
}

double toFloat64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<double>(from);
}

Module::Module()
{
  CreateFreeFunction("toInt8", &toInt8);
  CreateFreeFunction("toByte", &toByte);
  CreateFreeFunction("toInt16", &toInt16);
  CreateFreeFunction("toUInt16", &toUInt16);
  CreateFreeFunction("toInt32", &toInt32);
  CreateFreeFunction("toUInt32", &toUInt32);
  CreateFreeFunction("toInt64", &toInt64);
  CreateFreeFunction("toUInt64", &toUInt64);
  CreateFreeFunction("toFloat32", &toFloat32);
  CreateFreeFunction("toFloat64", &toFloat64);

  auto istring = GetClassInterface<String>();
  istring.CreateMemberFunction("length", &String::Length);
  istring.CreateMemberFunction("trim", &String::Trim);
  istring.CreateMemberFunction("find", &String::Find);
  istring.CreateMemberFunction("substr", &String::Substring);
  istring.CreateMemberFunction("reverse", &String::Reverse);

  auto imatrix = GetClassInterface<IMatrix>();
  imatrix.CreateConstuctor<int32_t, int32_t>();
  imatrix.EnableIndexOperator<AnyInteger, AnyInteger, TemplateParameter>();
  imatrix.CreateInstantiationType<Matrix<double>>();
  imatrix.CreateInstantiationType<Matrix<float>>();

  auto iarray = GetClassInterface<IArray>();
  iarray.CreateConstuctor<int32_t>();
  iarray.CreateMemberFunction("count", &IArray::Count);
  iarray.CreateMemberFunction("append", &IArray::Append);
  iarray.CreateMemberFunction("pop_back", &IArray::PopBackOne);
  iarray.CreateMemberFunction("pop_back", &IArray::PopBackMany);
  iarray.CreateMemberFunction("pop_front", &IArray::PopFrontOne);
  iarray.CreateMemberFunction("pop_front", &IArray::PopFrontMany);
  iarray.CreateMemberFunction("reverse", &IArray::Reverse);
  iarray.EnableIndexOperator<AnyInteger, TemplateParameter>();
  iarray.CreateInstantiationType<Array<bool>>();
  iarray.CreateInstantiationType<Array<int8_t>>();
  iarray.CreateInstantiationType<Array<uint8_t>>();
  iarray.CreateInstantiationType<Array<int16_t>>();
  iarray.CreateInstantiationType<Array<uint16_t>>();
  iarray.CreateInstantiationType<Array<int32_t>>();
  iarray.CreateInstantiationType<Array<uint32_t>>();
  iarray.CreateInstantiationType<Array<int64_t>>();
  iarray.CreateInstantiationType<Array<uint64_t>>();
  iarray.CreateInstantiationType<Array<float>>();
  iarray.CreateInstantiationType<Array<double>>();
  iarray.CreateInstantiationType<Array<Ptr<String>>>();

  auto imap = GetClassInterface<IMap>();
  imap.CreateConstuctor<>();
  imap.CreateMemberFunction("count", &IMap::Count);
  imap.EnableIndexOperator<TemplateParameter1, TemplateParameter2>();

  auto address = GetClassInterface<Address>();
  address.CreateConstuctor<>();
  address.CreateConstuctor<Ptr<String>>();
  address.CreateMemberFunction("signed_tx", &Address::HasSignedTx);

  auto istate = GetClassInterface<IState>();
  istate.CreateConstuctor<Ptr<String>, TemplateParameter>();
  istate.CreateConstuctor<Ptr<Address>, TemplateParameter>();
  istate.CreateMemberFunction("get", &IState::Get);
  istate.CreateMemberFunction("set", &IState::Set);
  istate.CreateMemberFunction("existed", &IState::Existed);

  auto ipersistentmap = GetClassInterface<IPersistentMap>();
  ipersistentmap.CreateConstuctor<Ptr<String>>();
  ipersistentmap.CreateConstuctor<Ptr<Address>>();
  ipersistentmap.EnableIndexOperator<TemplateParameter1, TemplateParameter2>();
}

}  // namespace vm
}  // namespace fetch
