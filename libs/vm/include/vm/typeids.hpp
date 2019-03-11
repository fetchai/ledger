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

#include <cstdint>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace vm {

using TypeId         = uint16_t;
using TypeIdArray    = std::vector<TypeId>;
using TypeIndex      = std::type_index;
using TypeIndexArray = std::vector<TypeIndex>;

namespace TypeIds {
static TypeId const Unknown            = 0;
static TypeId const Any                = 1;
static TypeId const TemplateParameter1 = 2;
static TypeId const TemplateParameter2 = 3;

static TypeId const Void    = 20;
static TypeId const Null    = 21;
static TypeId const Bool    = 22;
static TypeId const Int8    = 23;
static TypeId const Byte    = 24;
static TypeId const Int16   = 25;
static TypeId const UInt16  = 26;
static TypeId const Int32   = 27;
static TypeId const UInt32  = 28;
static TypeId const Int64   = 29;
static TypeId const UInt64  = 30;
static TypeId const Float32 = 31;
static TypeId const Float64 = 32;

static TypeId const IntegerVariant = 50;
static TypeId const RealVariant    = 51;
static TypeId const NumberVariant  = 52;
static TypeId const CastVariant    = 53;

static TypeId const ObjectMinId = 70;
static TypeId const IMatrix     = 70;
static TypeId const IArray      = 71;
static TypeId const IMap        = 72;

// ledger specific type ids
static TypeId const Address     = 80;
static TypeId const IState      = 81;

static TypeId const String = 90;

static TypeId const NumReserved = 500;
}  // namespace TypeIds

enum class TypeCategory : uint16_t
{
  Meta,
  Primitive,
  Class,
  Template,
  TemplateInstantiation,
  Variant
};

class RegisteredTypes
{
public:
  void RegisterType(TypeIndex type_index, TypeId type_id)
  {
    map_.insert(std::pair<TypeIndex, TypeId>(type_index, type_id));
  }
  TypeId GetTypeId(TypeIndex type_index) const
  {
    auto it = map_.find(type_index);
    if (it != map_.end())
    {
      return it->second;
    }
    return TypeIds::Unknown;
  }

private:
  std::unordered_map<TypeIndex, TypeId> map_;
};

struct TypeInfo
{
  TypeInfo()
  {
    id = TypeIds::Unknown;
  }
  TypeInfo(std::string const &name__, TypeId id__, TypeCategory category__,
           TypeIdArray const &parameter_type_ids__)
  {
    name               = name__;
    id                 = id__;
    category           = category__;
    parameter_type_ids = parameter_type_ids__;
  }
  std::string  name;
  TypeId       id;
  TypeCategory category;
  TypeIdArray  parameter_type_ids;
};
using TypeInfoTable = std::unordered_map<TypeId, TypeInfo>;

}  // namespace vm
}  // namespace fetch
