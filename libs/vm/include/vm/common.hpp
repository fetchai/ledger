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
static TypeId const Unknown        = 0;
static TypeId const Null           = 1;
static TypeId const Void           = 2;
static TypeId const Bool           = 3;
static TypeId const Int8           = 4;
static TypeId const UInt8          = 5;
static TypeId const Int16          = 6;
static TypeId const UInt16         = 7;
static TypeId const Int32          = 8;
static TypeId const UInt32         = 9;
static TypeId const Int64          = 10;
static TypeId const UInt64         = 11;
static TypeId const Float32        = 12;
static TypeId const Float64        = 13;
static TypeId const PrimitiveMaxId = 13;
static TypeId const String         = 14;
static TypeId const Address        = 15;
static TypeId const NumReserved    = 16;
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
  String                                    = 32,
  True                                      = 33,
  False                                     = 34,
  Null                                      = 35,
  Equal                                     = 36,
  NotEqual                                  = 37,
  LessThan                                  = 38,
  LessThanOrEqual                           = 39,
  GreaterThan                               = 40,
  GreaterThanOrEqual                        = 41,
  And                                       = 42,
  Or                                        = 43,
  Not                                       = 44,
  PrefixInc                                 = 45,
  PrefixDec                                 = 46,
  PostfixInc                                = 47,
  PostfixDec                                = 48,
  UnaryPlus                                 = 49,
  Negate                                    = 50,
  Index                                     = 51,
  Dot                                       = 52,
  Invoke                                    = 53,
  ParenthesisGroup                          = 54,
  Add                                       = 55,
  InplaceAdd                                = 56,
  Subtract                                  = 57,
  InplaceSubtract                           = 58,
  Multiply                                  = 59,
  InplaceMultiply                           = 60,
  Divide                                    = 61,
  InplaceDivide                             = 62,
  Modulo                                    = 63,
  InplaceModulo                             = 64
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
  TypeInfo()
  {
    type_kind = TypeKind::Unknown;
  }
  TypeInfo(TypeKind type_kind__, std::string const &name__, TypeIdArray const &parameter_type_ids__)
  {
    type_kind          = type_kind__;
    name               = name__;
    parameter_type_ids = parameter_type_ids__;
  }
  TypeKind    type_kind;
  std::string name;
  TypeIdArray parameter_type_ids;
};
using TypeInfoArray = std::vector<TypeInfo>;
using TypeInfoMap   = std::unordered_map<std::string, TypeId>;

class VM;
using Handler = std::function<void(VM *)>;

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

class RegisteredTypes
{
public:
  TypeId GetTypeId(TypeIndex type_index) const
  {
    auto it = map.find(type_index);
    if (it != map.end())
    {
      return it->second;
    }
    return TypeIds::Unknown;
  }

private:
  void Add(TypeIndex type_index, TypeId type_id)
  {
    map[type_index] = type_id;
  }
  std::unordered_map<TypeIndex, TypeId> map;
  friend class Analyser;
};

}  // namespace vm
}  // namespace fetch
