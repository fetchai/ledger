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

#include "meta/type_util.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
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
static constexpr TypeId Float32         = 13;
static constexpr TypeId Float64         = 14;
static constexpr TypeId Fixed32         = 15;
static constexpr TypeId Fixed64         = 16;
static constexpr TypeId PrimitiveMaxId  = 16;
static constexpr TypeId String          = 17;
static constexpr TypeId Address         = 18;
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
  Unknown                                   = 0,
  Root                                      = 1,
  File                                      = 2,
  FunctionDefinitionStatement               = 3,
  WhileStatement                            = 4,
  ForStatement                              = 5,
  If                                        = 6,
  ElseIf                                    = 7,
  Else                                      = 8,
  Annotations                               = 9,
  Annotation                                = 10,
  AnnotationNameValuePair                   = 11,
  IfStatement                               = 12,
  VarDeclarationStatement                   = 13,
  VarDeclarationTypedAssignmentStatement    = 14,
  VarDeclarationTypelessAssignmentStatement = 15,
  ReturnStatement                           = 16,
  BreakStatement                            = 17,
  ContinueStatement                         = 18,
  Assign                                    = 19,
  Identifier                                = 20,
  Template                                  = 21,
  Integer8                                  = 22,
  UnsignedInteger8                          = 23,
  Integer16                                 = 24,
  UnsignedInteger16                         = 25,
  Integer32                                 = 26,
  UnsignedInteger32                         = 27,
  Integer64                                 = 28,
  UnsignedInteger64                         = 29,
  Float32                                   = 30,
  Float64                                   = 31,
  Fixed32                                   = 32,
  Fixed64                                   = 33,
  String                                    = 34,
  True                                      = 35,
  False                                     = 36,
  Null                                      = 37,
  Equal                                     = 38,
  NotEqual                                  = 39,
  LessThan                                  = 40,
  LessThanOrEqual                           = 41,
  GreaterThan                               = 42,
  GreaterThanOrEqual                        = 43,
  And                                       = 44,
  Or                                        = 45,
  Not                                       = 46,
  PrefixInc                                 = 47,
  PrefixDec                                 = 48,
  PostfixInc                                = 49,
  PostfixDec                                = 50,
  UnaryPlus                                 = 51,
  Negate                                    = 52,
  Index                                     = 53,
  Dot                                       = 54,
  Invoke                                    = 55,
  ParenthesisGroup                          = 56,
  Add                                       = 57,
  InplaceAdd                                = 58,
  Subtract                                  = 59,
  InplaceSubtract                           = 60,
  Multiply                                  = 61,
  InplaceMultiply                           = 62,
  Divide                                    = 63,
  InplaceDivide                             = 64,
  Modulo                                    = 65,
  InplaceModulo                             = 66,
  InitialiserList                           = 67
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
  Unknown                  = 0,
  Primitive                = 1,
  Meta                     = 2,
  Group                    = 3,
  Class                    = 4,
  Template                 = 5,
  Instantiation            = 6,
  UserDefinedInstantiation = 7
};

enum class VariableKind : uint8_t
{
  Unknown   = 0,
  Parameter = 1,
  For       = 2,
  Local     = 3
};

enum class FunctionKind : uint8_t
{
  Unknown                 = 0,
  FreeFunction            = 1,
  Constructor             = 2,
  StaticMemberFunction    = 3,
  MemberFunction          = 4,
  UserDefinedFreeFunction = 5
};

struct TypeInfo
{
  TypeInfo() = default;
  TypeInfo(TypeKind type_kind__, std::string name__, TypeId template_type_id__,
           TypeIdArray parameter_type_ids__)
    : type_kind{type_kind__}
    , name{std::move(name__)}
    , template_type_id{template_type_id__}
    , parameter_type_ids{std::move(parameter_type_ids__)}
  {}

  TypeKind    type_kind = TypeKind::Unknown;
  std::string name;
  TypeId      template_type_id = TypeIds::Unknown;
  TypeIdArray parameter_type_ids;
};
using TypeInfoArray = std::vector<TypeInfo>;
using TypeInfoMap   = std::unordered_map<std::string, TypeId>;

class VM;
template <typename T>
class Ptr;
class Object;

using Handler                   = std::function<void(VM *)>;
using DefaultConstructorHandler = std::function<Ptr<Object>(VM *, TypeId)>;

struct FunctionInfo
{
  FunctionInfo()
  {
    function_kind = FunctionKind::Unknown;
  }
  FunctionInfo(FunctionKind function_kind__, std::string const &unique_id__,
               Handler const &handler__)
  {
    function_kind = function_kind__;
    unique_id     = unique_id__;
    handler       = handler__;
  }
  FunctionKind function_kind;
  std::string  unique_id;
  Handler      handler;
};
using FunctionInfoArray = std::vector<FunctionInfo>;

using DeserializeConstructorMap = std::unordered_map<TypeIndex, DefaultConstructorHandler>;

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

    return TypeIndex(typeid(void ***));
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

}  // namespace vm
}  // namespace fetch
