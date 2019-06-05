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

#include "vm/common.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"

#include <cstdint>

namespace fetch {
namespace vm {

namespace {

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

}  // namespace

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

  GetClassInterface<String>()
      .CreateMemberFunction("length", &String::Length)
      .CreateMemberFunction("trim", &String::Trim)
      .CreateMemberFunction("find", &String::Find)
      .CreateMemberFunction("substr", &String::Substring)
      .CreateMemberFunction("reverse", &String::Reverse);

  GetClassInterface<IMatrix>()
      .CreateConstuctor<int32_t, int32_t>()
      .EnableIndexOperator<AnyInteger, AnyInteger, TemplateParameter>()
      .CreateInstantiationType<Matrix<double>>()
      .CreateInstantiationType<Matrix<float>>();

  GetClassInterface<IArray>()
      .CreateConstuctor<int32_t>()
      .CreateMemberFunction("count", &IArray::Count)
      .CreateMemberFunction("append", &IArray::Append)
      .CreateMemberFunction("popBack", &IArray::PopBackOne)
      .CreateMemberFunction("popBack", &IArray::PopBackMany)
      .CreateMemberFunction("popFront", &IArray::PopFrontOne)
      .CreateMemberFunction("popFront", &IArray::PopFrontMany)
      .CreateMemberFunction("reverse", &IArray::Reverse)
      .CreateMemberFunction("extend", &IArray::Extend)
      .EnableIndexOperator<AnyInteger, TemplateParameter>()
      .CreateInstantiationType<Array<bool>>()
      .CreateInstantiationType<Array<int8_t>>()
      .CreateInstantiationType<Array<uint8_t>>()
      .CreateInstantiationType<Array<int16_t>>()
      .CreateInstantiationType<Array<uint16_t>>()
      .CreateInstantiationType<Array<int32_t>>()
      .CreateInstantiationType<Array<uint32_t>>()
      .CreateInstantiationType<Array<int64_t>>()
      .CreateInstantiationType<Array<uint64_t>>()
      .CreateInstantiationType<Array<float>>()
      .CreateInstantiationType<Array<double>>()
      .CreateInstantiationType<Array<Ptr<String>>>();

  GetClassInterface<IMap>()
      .CreateConstuctor<>()
      .CreateMemberFunction("count", &IMap::Count)
      .EnableIndexOperator<TemplateParameter1, TemplateParameter2>();

  GetClassInterface<Address>()
      .CreateConstuctor<>()
      .CreateConstuctor<Ptr<String>>()
      .CreateMemberFunction("signedTx", &Address::HasSignedTx);

  GetClassInterface<IState>()
      .CreateConstuctor<Ptr<String>, TemplateParameter>()
      .CreateConstuctor<Ptr<Address>, TemplateParameter>()
      .CreateMemberFunction("get", &IState::Get)
      .CreateMemberFunction("set", &IState::Set)
      .CreateMemberFunction("existed", &IState::Existed);
}

}  // namespace vm
}  // namespace fetch
