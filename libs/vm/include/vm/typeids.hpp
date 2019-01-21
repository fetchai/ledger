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

namespace fetch {
namespace vm {

using Index = uint16_t;

enum class TypeId : uint16_t
{
  Unknown = 0,
  TemplateInstantiation,
  TemplateParameter1,
  TemplateParameter2,
  Numeric,
  MatrixTemplate,
  ArrayTemplate,
  Void,
  Null,
  Bool,
  Int8,
  Byte,
  Int16,
  UInt16,
  Int32,
  UInt32,
  Int64,
  UInt64,
  Float32,
  Float64,

  PrimitivesObjectsDivider,

  String,
  Matrix_Float32,
  Matrix_Float64,
  Array_Bool,
  Array_Int8,
  Array_Byte,
  Array_Int16,
  Array_UInt16,
  Array_Int32,
  Array_UInt32,
  Array_Int64,
  Array_UInt64,
  Array_Float32,
  Array_Float64,
  Array_String,
  Array_Matrix_Float32,
  Array_Matrix_Float64,
  Array,
  // matrix op number
  // matrix op= number
  Matrix_Float32__Float32,
  Matrix_Float64__Float64,
  // number op matrix
  Float32__Matrix_Float32,
  Float64__Matrix_Float64,
  // matrixarray[index] op= number
  Array_Matrix_Float32__Float32,
  Array_Matrix_Float64__Float64,

  // Custom
  StartOfUserTypes =
      32000  // This Type code always need to have the highest value - preferably constant.
};

inline bool IsIntegralType(const TypeId id)
{
  return ((id == TypeId::Int8) || (id == TypeId::Byte) || (id == TypeId::Int16) ||
          (id == TypeId::UInt16) || (id == TypeId::Int32) || (id == TypeId::UInt32) ||
          (id == TypeId::Int64) || (id == TypeId::UInt64));
}

inline bool IsNumericType(const TypeId id)
{
  if (IsIntegralType(id))
  {
    return true;
  }
  return ((id == TypeId::Float32) || (id == TypeId::Float64));
}

inline bool IsMatrixType(const TypeId id)
{
  return ((id == TypeId::Matrix_Float32) || (id == TypeId::Matrix_Float64));
}

inline bool IsArrayType(const TypeId id)
{
  return ((id == TypeId::Array_Bool) || (id == TypeId::Array_Int8) || (id == TypeId::Array_Byte) ||
          (id == TypeId::Array_Int16) || (id == TypeId::Array_UInt16) ||
          (id == TypeId::Array_Int32) || (id == TypeId::Array_UInt32) ||
          (id == TypeId::Array_Int64) || (id == TypeId::Array_UInt64) ||
          (id == TypeId::Array_Float32) || (id == TypeId::Array_Float64) ||
          (id == TypeId::Array_Matrix_Float32) || (id == TypeId::Array_Matrix_Float64) ||
          (id == TypeId::Array_String) || (id == TypeId::Array));
}

inline bool IsRelationalType(const TypeId id)
{
  // should this include string?
  return IsNumericType(id);
}

}  // namespace vm
}  // namespace fetch
