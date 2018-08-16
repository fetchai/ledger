#ifndef TYPEIDS__HPP
#define TYPEIDS__HPP
#include <cstdint>

namespace fetch {
namespace vm {

typedef uint16_t Index;

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
  IntPair
};

inline bool IsIntegralType(const TypeId id)
{
  return ((id == TypeId::Int8) || (id == TypeId::Byte) || (id == TypeId::Int16) ||
          (id == TypeId::UInt16) || (id == TypeId::Int32) || (id == TypeId::UInt32) ||
          (id == TypeId::Int64) || (id == TypeId::UInt64));
}

inline bool IsNumericType(const TypeId id)
{
  if (IsIntegralType(id)) return true;
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

#endif
