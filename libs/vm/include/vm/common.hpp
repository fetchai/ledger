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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "meta/type_util.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

using TypeId         = uint16_t;
using TypeIdArray    = std::vector<TypeId>;
using TypeIndex      = std::type_index;
using TypeIndexArray = std::vector<TypeIndex>;

namespace TypeIds {
static constexpr TypeId Unknown         = 0;
static constexpr TypeId Null            = 1;
static constexpr TypeId InitialiserList = 2;
static constexpr TypeId Void            = 3;
static constexpr TypeId Bool            = 4;
static constexpr TypeId Int8            = 5;
static constexpr TypeId UInt8           = 6;
static constexpr TypeId Int16           = 7;
static constexpr TypeId UInt16          = 8;
static constexpr TypeId Int32           = 9;
static constexpr TypeId UInt32          = 10;
static constexpr TypeId Int64           = 11;
static constexpr TypeId UInt64          = 12;
static constexpr TypeId Fixed32         = 13;
static constexpr TypeId Fixed64         = 14;
static constexpr TypeId PrimitiveMaxId  = 14;
static constexpr TypeId String          = 15;
static constexpr TypeId Address         = 16;
static constexpr TypeId Fixed128        = 17;
static constexpr TypeId UInt256         = 18;
static constexpr TypeId NumReserved     = 19;
}  // namespace TypeIds

enum class NodeCategory : uint8_t
{
  Unknown    = 0,
  Basic      = 1,
  Block      = 2,
  Expression = 3
};

enum class NodeKind : uint16_t
{
  Unknown                                        = 0,
  Root                                           = 1,
  File                                           = 2,
  FreeFunctionDefinition                         = 3,
  WhileStatement                                 = 4,
  ForStatement                                   = 5,
  If                                             = 6,
  ElseIf                                         = 7,
  Else                                           = 8,
  Annotations                                    = 9,
  Annotation                                     = 10,
  AnnotationNameValuePair                        = 11,
  IfStatement                                    = 12,
  LocalVarDeclarationStatement                   = 13,
  LocalVarDeclarationTypedAssignmentStatement    = 14,
  LocalVarDeclarationTypelessAssignmentStatement = 15,
  ReturnStatement                                = 16,
  BreakStatement                                 = 17,
  ContinueStatement                              = 18,
  Assign                                         = 19,
  Identifier                                     = 20,
  Template                                       = 21,
  Integer8                                       = 22,
  UnsignedInteger8                               = 23,
  Integer16                                      = 24,
  UnsignedInteger16                              = 25,
  Integer32                                      = 26,
  UnsignedInteger32                              = 27,
  Integer64                                      = 28,
  UnsignedInteger64                              = 29,
  Fixed32                                        = 32,
  Fixed64                                        = 33,
  String                                         = 34,
  True                                           = 35,
  False                                          = 36,
  Null                                           = 37,
  Equal                                          = 38,
  NotEqual                                       = 39,
  LessThan                                       = 40,
  LessThanOrEqual                                = 41,
  GreaterThan                                    = 42,
  GreaterThanOrEqual                             = 43,
  And                                            = 44,
  Or                                             = 45,
  Not                                            = 46,
  PrefixInc                                      = 47,
  PrefixDec                                      = 48,
  PostfixInc                                     = 49,
  PostfixDec                                     = 50,
  UnaryPlus                                      = 51,
  Negate                                         = 52,
  Index                                          = 53,
  Dot                                            = 54,
  Invoke                                         = 55,
  Parenthesis                                    = 56,
  Add                                            = 57,
  InplaceAdd                                     = 58,
  Subtract                                       = 59,
  InplaceSubtract                                = 60,
  Multiply                                       = 61,
  InplaceMultiply                                = 62,
  Divide                                         = 63,
  InplaceDivide                                  = 64,
  Modulo                                         = 65,
  InplaceModulo                                  = 66,
  PersistentStatement                            = 67,
  UseStatement                                   = 68,
  UseStatementKeyList                            = 69,
  UseAnyStatement                                = 70,
  InitialiserList                                = 71,
  ContractDefinition                             = 72,
  ContractFunction                               = 73,
  ContractStatement                              = 74,
  Fixed128                                       = 75,
  StructDefinition                               = 76,
  MemberFunctionDefinition                       = 77,
  MemberVarDeclarationStatement                  = 78
};

enum class ExpressionKind : uint8_t
{
  Unknown       = 0,
  Variable      = 1,
  LV            = 2,
  RV            = 3,
  Type          = 4,
  FunctionGroup = 5
};

enum class TypeKind : uint8_t
{
  Unknown                          = 0,
  Primitive                        = 1,
  Meta                             = 2,
  Group                            = 3,
  Class                            = 4,
  Template                         = 5,
  TemplateInstantiation            = 6,
  UserDefinedTemplateInstantiation = 7,
  UserDefinedContract              = 8,
  UserDefinedStruct                = 9
};

enum class VariableKind : uint8_t
{
  Unknown   = 0,
  Parameter = 1,
  For       = 2,
  Local     = 3,
  Member    = 4,
  Use       = 5,
  UseAny    = 6
};

enum class FunctionKind : uint8_t
{
  Unknown                     = 0,
  FreeFunction                = 1,
  Constructor                 = 2,
  StaticMemberFunction        = 3,
  MemberFunction              = 4,
  UserDefinedContractFunction = 5,
  UserDefinedFreeFunction     = 6,
  UserDefinedConstructor      = 7,
  UserDefinedMemberFunction   = 8
};

struct TypeInfo
{
  TypeInfo() = default;
  TypeInfo(TypeKind kind__, std::string name__, TypeId type_id__, TypeId template_type_id__,
           TypeIdArray template_parameter_type_ids__)
    : kind{kind__}
    , name{std::move(name__)}
    , type_id{type_id__}
    , template_type_id{template_type_id__}
    , template_parameter_type_ids{std::move(template_parameter_type_ids__)}
  {}

  TypeKind    kind{TypeKind::Unknown};
  std::string name;
  TypeId      type_id{TypeIds::Unknown};
  TypeId      template_type_id{TypeIds::Unknown};
  TypeIdArray template_parameter_type_ids;
};
using TypeInfoArray = std::vector<TypeInfo>;
using TypeInfoMap   = std::unordered_map<std::string, TypeId>;

class VM;
template <typename T>
class Ptr;
class Object;

using ChargeAmount                                = uint64_t;
static constexpr ChargeAmount COMPUTE_CHARGE_COST = 1u;
static constexpr ChargeAmount MAXIMUM_CHARGE      = std::numeric_limits<ChargeAmount>::max();
template <typename... Args>
using ChargeEstimator = std::function<ChargeAmount(Args const &...)>;

using Handler                   = std::function<void(VM *)>;
using DefaultConstructorHandler = std::function<Ptr<Object>(VM *, TypeId)>;
using CPPCopyConstructorHandler = std::function<Ptr<Object>(VM *, void const *)>;

struct FunctionInfo
{
  FunctionInfo() = default;
  FunctionInfo(FunctionKind function_kind__, std::string unique_name__, Handler handler__,
               ChargeAmount static_charge__)
    : function_kind{function_kind__}
    , unique_name{std::move(unique_name__)}
    , handler{std::move(handler__)}
    , static_charge{static_charge__}
  {}

  FunctionKind function_kind{FunctionKind::Unknown};
  std::string  unique_name;
  Handler      handler;
  ChargeAmount static_charge{};
};
using FunctionInfoArray = std::vector<FunctionInfo>;

using DeserializeConstructorMap = std::unordered_map<TypeIndex, DefaultConstructorHandler>;
using CPPCopyConstructorMap     = std::unordered_map<TypeIndex, CPPCopyConstructorHandler>;

class RegisteredTypes
{
public:
  TypeId GetTypeId(TypeIndex type_index) const
  {
    auto it = map_.find(type_index);
    if (it != map_.end())
    {
      return it->second;
    }
    return TypeIds::Unknown;
  }

  TypeIndex GetTypeIndex(TypeId type_id) const
  {
    auto it = reverse_.find(type_id);
    if (it != reverse_.end())
    {
      return it->second;
    }

    return {typeid(void ***)};
  }

private:
  void Add(TypeIndex type_index, TypeId type_id)
  {
    map_[type_index] = type_id;
    reverse_.insert({type_id, type_index});
  }
  std::unordered_map<TypeIndex, TypeId> map_;
  std::unordered_map<TypeId, TypeIndex> reverse_;
  friend class Analyser;
};

struct InitialiserListPlaceholder
{
};

struct SourceFile
{
  SourceFile() = default;
  SourceFile(std::string filename__, std::string source__)
    : filename{std::move(filename__)}
    , source{std::move(source__)}
  {}
  std::string filename;
  std::string source;
};
using SourceFiles = std::vector<SourceFile>;

}  // namespace vm

namespace serializers {

template <typename D>
struct MapSerializer<fetch::vm::SourceFile, D>
{
public:
  using Type       = fetch::vm::SourceFile;
  using DriverType = D;

  static uint8_t const FILENAME = 1;
  static uint8_t const SOURCE   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &source_file)
  {
    auto map = map_constructor(2);
    map.Append(FILENAME, source_file.filename);
    map.Append(SOURCE, source_file.source);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &source_file)
  {
    map.ExpectKeyGetValue(FILENAME, source_file.filename);
    map.ExpectKeyGetValue(SOURCE, source_file.source);
  }
};

}  // namespace serializers
}  // namespace fetch
