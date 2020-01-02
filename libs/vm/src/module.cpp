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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/common.hpp"
#include "vm/fixed.hpp"
#include "vm/map.hpp"
#include "vm/module.hpp"
#include "vm/pair.hpp"
#include "vm/sharded_state.hpp"
#include "vm/state.hpp"
#include "vm/string.hpp"
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
  case TypeIds::UInt8:
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
  case TypeIds::Fixed32:
  {
    to = static_cast<To>(fixed_point::fp32_t::FromBase(from.primitive.i32));
    break;
  }
  case TypeIds::Fixed64:
  {
    to = static_cast<To>(fixed_point::fp64_t::FromBase(from.primitive.i64));
    break;
  }
  default:
  {
    to = 0;
    // Not a primitive
    assert(false);
    break;
  }
  }  // switch
  return to;
}

int8_t toInt8(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<int8_t>(from);
}

uint8_t toUInt8(VM * /* vm */, AnyPrimitive const &from)
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

fixed_point::fp32_t toFixed32(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<fixed_point::fp32_t>(from);
}

fixed_point::fp64_t toFixed64(VM * /* vm */, AnyPrimitive const &from)
{
  return Cast<fixed_point::fp64_t>(from);
}

Ptr<Fixed128> toFixed128(VM *vm, AnyPrimitive const &from)
{
  fixed_point::fp128_t fixed;
  switch (from.type_id)
  {
  case TypeIds::Bool:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int8:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.i8);
    break;
  }
  case TypeIds::UInt8:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.ui8);
    break;
  }
  case TypeIds::Int16:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.i16);
    break;
  }
  case TypeIds::UInt16:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.ui16);
    break;
  }
  case TypeIds::Int32:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.i32);
    break;
  }
  case TypeIds::UInt32:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.ui32);
    break;
  }
  case TypeIds::Int64:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.i64);
    break;
  }
  case TypeIds::UInt64:
  {
    fixed = static_cast<fixed_point::fp128_t>(from.primitive.ui64);
    break;
  }
  case TypeIds::Fixed32:
  {
    fixed = static_cast<fixed_point::fp128_t>(fixed_point::fp32_t::FromBase(from.primitive.i32));
    break;
  }
  case TypeIds::Fixed64:
  {
    fixed = static_cast<fixed_point::fp128_t>(fixed_point::fp64_t::FromBase(from.primitive.i64));
    break;
  }
  default:
  {
    fixed = 0;
    // Not a primitive
    assert(false);
    break;
  }
  }  // switch

  return Ptr<Fixed128>(new Fixed128(vm, fixed));
}

}  // namespace

Module::Module()
{
  CreateFreeFunction("toInt8", &toInt8);
  CreateFreeFunction("toUInt8", &toUInt8);
  CreateFreeFunction("toInt16", &toInt16);
  CreateFreeFunction("toUInt16", &toUInt16);
  CreateFreeFunction("toInt32", &toInt32);
  CreateFreeFunction("toUInt32", &toUInt32);
  CreateFreeFunction("toInt64", &toInt64);
  CreateFreeFunction("toUInt64", &toUInt64);
  CreateFreeFunction("toFixed32", &toFixed32);
  CreateFreeFunction("toFixed64", &toFixed64);
  CreateFreeFunction("toFixed128", &toFixed128);

  GetClassInterface<IArray>()
      .CreateConstructor(&IArray::Constructor)
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) { return IArray::Constructor(vm, type_id, 0u); })
      .CreateMemberFunction("append", &IArray::Append)
      .CreateMemberFunction("count", &IArray::Count)
      .CreateMemberFunction("erase", &IArray::Erase)
      .CreateMemberFunction("extend", &IArray::Extend)
      .CreateMemberFunction("popBack", &IArray::PopBackOne)
      .CreateMemberFunction("popBack", &IArray::PopBackMany)
      .CreateMemberFunction("popFront", &IArray::PopFrontOne)
      .CreateMemberFunction("popFront", &IArray::PopFrontMany)
      .CreateMemberFunction("reverse", &IArray::Reverse)
      .EnableIndexOperator(&IArray::GetIndexedValue, &IArray::SetIndexedValue)
      .CreateInstantiationType<Array<bool>>()
      .CreateInstantiationType<Array<int8_t>>()
      .CreateInstantiationType<Array<uint8_t>>()
      .CreateInstantiationType<Array<int16_t>>()
      .CreateInstantiationType<Array<uint16_t>>()
      .CreateInstantiationType<Array<int32_t>>()
      .CreateInstantiationType<Array<uint32_t>>()
      .CreateInstantiationType<Array<int64_t>>()
      .CreateInstantiationType<Array<uint64_t>>()
      .CreateInstantiationType<Array<fixed_point::fp32_t>>()
      .CreateInstantiationType<Array<fixed_point::fp64_t>>()
      .CreateCPPCopyConstructor<std::vector<fixed_point::fp64_t>>(
          [](VM *vm, TypeId, std::vector<fixed_point::fp64_t> const &arr) -> Ptr<IArray> {
            auto ret = Ptr<Array<fixed_point::fp64_t>>(
                new Array<fixed_point::fp64_t>(vm, vm->GetTypeId<Array<fixed_point::fp64_t>>(),
                                               vm->GetTypeId<fixed_point::fp64_t>(), 0));
            ret->elements = arr;
            return ret;
          })
      .CreateCPPCopyConstructor<std::vector<std::vector<fixed_point::fp64_t>>>(
          [](VM *                                                 vm, TypeId,
             std::vector<std::vector<fixed_point::fp64_t>> const &arr) -> Ptr<IArray> {
            auto outerid = vm->GetTypeId<Array<Ptr<Array<fixed_point::fp64_t>>>>();
            auto innerid = vm->GetTypeId<Array<fixed_point::fp64_t>>();
            std::cout << "ID: " << vm->GetTypeId<Array<Ptr<Array<fixed_point::fp64_t>>>>()
                      << std::endl;
            auto ret = Ptr<Array<Ptr<Array<fixed_point::fp64_t>>>>(
                new Array<Ptr<Array<fixed_point::fp64_t>>>(vm, outerid, innerid, 0));

            for (auto &element : arr)
            {
              auto a = Ptr<Array<fixed_point::fp64_t>>(
                  new Array<fixed_point::fp64_t>(vm, vm->GetTypeId<Array<fixed_point::fp64_t>>(),
                                                 vm->GetTypeId<fixed_point::fp64_t>(), 0));

              a->elements = element;
              ret->elements.emplace_back(a);
            }

            return ret;
          })
      .CreateInstantiationType<Array<Ptr<Fixed128>>>()
      .CreateInstantiationType<Array<Ptr<String>>>()
      .CreateInstantiationType<Array<Ptr<Address>>>();

  GetClassInterface<String>()
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId) -> Ptr<String> { return Ptr<String>{new String(vm, "")}; })
      .CreateCPPCopyConstructor<std::string>(
          [](VM *vm, TypeId, std::string const &s) -> Ptr<String> {
            return Ptr<String>{new String(vm, s)};
          })
      .CreateMemberFunction("find", &String::Find)
      .CreateMemberFunction("length", &String::Length)
      .CreateMemberFunction("sizeInBytes", &String::SizeInBytes)
      .CreateMemberFunction("reverse", &String::Reverse)
      .CreateMemberFunction("split", &String::Split)
      .CreateMemberFunction("substr", &String::Substring)
      .CreateMemberFunction("trim", &String::Trim);

  GetClassInterface<IMap>()
      .CreateConstructor(&IMap::Constructor)
      .CreateMemberFunction("count", &IMap::Count)
      .EnableIndexOperator(&IMap::GetIndexedValue, &IMap::SetIndexedValue);

  GetClassInterface<IPair>()
      .CreateConstructor(&IPair::Constructor)
      .CreateMemberFunction("first", &IPair::GetFirst)
      .CreateMemberFunction("second", &IPair::GetSecond)
      .CreateMemberFunction("first", &IPair::SetFirst)
      .CreateMemberFunction("second", &IPair::SetSecond);

  GetClassInterface<Address>()
      .CreateSerializeDefaultConstructor(&Address::Constructor)
      .CreateConstructor(&Address::ConstructorFromString)
      .CreateMemberFunction("signedTx", &Address::HasSignedTx);

  CreateFreeFunction("toString", &Address::ToString);

  GetClassInterface<IState>()
      .CreateConstructor(&IState::ConstructorFromString)
      .CreateConstructor(&IState::ConstructorFromAddress)
      .CreateMemberFunction("get", &IState::Get)
      .CreateMemberFunction("get", &IState::GetWithDefault)
      .CreateMemberFunction("set", &IState::Set)
      .CreateMemberFunction("existed", &IState::Existed);

  GetClassInterface<IShardedState>()
      .CreateConstructor(&IShardedState::ConstructorFromString)
      .CreateConstructor(&IShardedState::ConstructorFromAddress)
      // TODO (issue 1172): This will be enabled once the issue is resolved
      //.EnableIndexOperator<Ptr<String>, TemplateParameter1>()
      //.EnableIndexOperator<Ptr<Address>, TemplateParameter1>();
      .CreateMemberFunction("get", &IShardedState::GetFromString)
      .CreateMemberFunction("get", &IShardedState::GetFromAddress)
      .CreateMemberFunction("get", &IShardedState::GetFromStringWithDefault)
      .CreateMemberFunction("get", &IShardedState::GetFromAddressWithDefault)
      .CreateMemberFunction("set", &IShardedState::SetFromString)
      .CreateMemberFunction("set", &IShardedState::SetFromAddress);

  GetClassInterface<Fixed128>()
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId) -> Ptr<Fixed128> {
        return Ptr<Fixed128>{new Fixed128(vm, fixed_point::fp128_t::_0)};
      })
      .CreateMemberFunction("copy", &Fixed128::Copy);
}

}  // namespace vm
}  // namespace fetch
