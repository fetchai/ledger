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

#include <utility>

#include "defs.hpp"
#include "math/arithmetic/comparison.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/linalg/matrix.hpp"

namespace fetch {
namespace vm {

struct String : public Object
{
  std::string str;
  bool        is_literal;
  String()
  {}
  String(VM *vm, std::string str__, const bool is_literal__)
    : Object(TypeId::String, vm)
    , str(std::move(str__))
    , is_literal(is_literal__)
  {}
  virtual ~String()
  {}
};

template <typename T>
struct Matrix : public Object
{
  fetch::math::linalg::Matrix<T, fetch::memory::Array<T>> matrix;
  Matrix(const TypeId type_id, VM *vm, const size_t rows, const size_t columns)
    : Object(type_id, vm)
    , matrix(rows, columns)
  {}
  Matrix(const TypeId type_id, VM *vm,
         fetch::math::linalg::Matrix<T, fetch::memory::Array<T>> &&matrix__)
    : Object(type_id, vm)
    , matrix(matrix__)
  {}
  virtual ~Matrix()
  {}
};
using MatrixFloat32 = Matrix<float>;
using MatrixFloat64 = Matrix<double>;

template <typename T>
struct Array : public Object
{
  std::vector<T> elements;
  Array(const TypeId type_id, VM *vm, const size_t size)
    : Object(type_id, vm)
    , elements(size, T(0))
  {}
  template <typename U, typename std::enable_if<std::is_pointer<U>::value>::type * = nullptr>
  void Release()
  {
    for (int i = 0; i < (int)elements.size(); ++i)
    {
      Object *object = elements[std::size_t(i)];
      if (object)
      {
        object->Release();
      }
    }
  }
  template <typename U, typename std::enable_if<!std::is_pointer<U>::value>::type * = nullptr>
  void Release()
  {}
  virtual ~Array()
  {
    Release<T>();
  }
};

template <typename T>
struct IsMatrix : public std::false_type
{
};
template <>
struct IsMatrix<MatrixFloat32> : public std::true_type
{
};
template <>
struct IsMatrix<MatrixFloat64> : public std::true_type
{
};

/// Forward declaration for class and function export
/// @{
class Module;

namespace details {
template <typename T>
struct LoaderClass;

template <typename T, int N>
struct StorerClass;

template <int N>
struct Resetter;
}  // namespace details
/// }

class VM
{
public:
  VM(Module *module = nullptr)
    : module_(module)
  {}
  ~VM()
  {}
  bool        Execute(const Script &script, const std::string &name);
  std::string error() const
  {
    return error_;
  }
  std::size_t error_line() const
  {
    return error_line_;
  }

private:
  friend struct Object;

  /// Friends and objects that allow dynamic module export
  /// @{
  Module *module_ = nullptr;

  template <typename T>
  friend class ClassInterface;
  friend class Module;
  friend class BaseModule;

  template <int N>
  friend struct details::Resetter;
  template <typename T>
  friend struct details::LoaderClass;

  template <typename T, int N>
  friend struct details::StorerClass;
  /// }

  static const int FRAME_STACK_SIZE = 50;
  static const int STACK_SIZE       = 5000;
  static const int MAX_LIVE_OBJECTS = 200;
  static const int MAX_RANGE_LOOPS  = 50;

  struct Frame
  {
    const Script::Function *function;
    int                     bsp;
    int                     pc;
  };

  struct ForRangeLoop
  {
    Index   variable_index;
    Variant current;
    Variant target;
    Variant delta;
  };

  struct LiveObjectInfo
  {
    int   frame_sp;
    Index variable_index;
    int   scope_number;
  };

  const Script *             script_;
  const Script::Function *   function_;
  std::vector<String>        pool_;
  std::vector<String *>      strings_;
  Frame                      frame_stack_[FRAME_STACK_SIZE];
  int                        frame_sp_;
  int                        bsp_;
  Value                      stack_[STACK_SIZE];
  int                        sp_;
  ForRangeLoop               range_loop_stack_[MAX_RANGE_LOOPS];
  int                        range_loop_sp_;
  LiveObjectInfo             live_object_stack_[MAX_LIVE_OBJECTS];
  int                        live_object_sp_;
  int                        pc_;
  const Script::Instruction *instruction_;
  bool                       stop_;
  std::string                error_;
  std::size_t                error_line_;

  Value &GetVariable(const Index variable_index)
  {
    return stack_[bsp_ + variable_index];
  }

  void Destruct(const int scope_number)
  {
    // Destruct all live objects in the current frame and with scope >= scope_number
    while (live_object_sp_ >= 0)
    {
      const LiveObjectInfo &info = live_object_stack_[live_object_sp_];
      if ((info.frame_sp != frame_sp_) || (info.scope_number < scope_number))
      {
        break;
      }
      Value &variable = GetVariable(info.variable_index);
      variable.Reset();
      --live_object_sp_;
    }
  }

  void InvokeUserFunction(const Index index)
  {
    // Note: the parameters are already on the stack
    Frame frame;
    frame.function = function_;
    frame.bsp      = bsp_;
    frame.pc       = pc_;
    if (frame_sp_ >= FRAME_STACK_SIZE - 1)
    {
      RuntimeError("frame stack overflow");
      return;
    }
    frame_stack_[++frame_sp_] = frame;
    function_                 = &(script_->functions[index]);
    bsp_                      = sp_ - function_->num_parameters + 1;  // first parameter
    pc_                       = 0;
    const int num_locals      = function_->num_variables - function_->num_parameters;
    sp_ += num_locals;
  }

  void ReleaseObject(Object *object, const TypeId /*type_id*/)
  {
    delete object;
  }

  void RuntimeError(const std::string &message);
  void AcquireMatrix(const size_t rows, const size_t columns, MatrixFloat32 *&m);
  void AcquireMatrix(const size_t rows, const size_t columns, MatrixFloat64 *&m);
  void ForRangeInit();
  void ForRangeIterate();
  void CreateMatrix();
  void CreateArray();

  //
  // Casting
  //

  template <typename From, typename To>
  void Cast(From &from, To &to)
  {
    to = static_cast<To>(from);
  }

  template <typename To>
  void HandleCast(Value &value, const TypeId to_type_id, To &to)
  {
    const TypeId from_type_id = value.type_id;
    value.type_id             = to_type_id;
    switch (from_type_id)
    {
    case TypeId::Int8:
    {
      Cast(value.variant.i8, to);
      break;
    }
    case TypeId::Byte:
    {
      Cast(value.variant.ui8, to);
      break;
    }
    case TypeId::Int16:
    {
      Cast(value.variant.i16, to);
      break;
    }
    case TypeId::UInt16:
    {
      Cast(value.variant.ui16, to);
      break;
    }
    case TypeId::Int32:
    {
      Cast(value.variant.i32, to);
      break;
    }
    case TypeId::UInt32:
    {
      Cast(value.variant.ui32, to);
      break;
    }
    case TypeId::Int64:
    {
      Cast(value.variant.i64, to);
      break;
    }
    case TypeId::UInt64:
    {
      Cast(value.variant.ui64, to);
      break;
    }
    case TypeId::Float32:
    {
      Cast(value.variant.f32, to);
      break;
    }
    case TypeId::Float64:
    {
      Cast(value.variant.f64, to);
      break;
    }
    default:
    {
      break;
    }
    }
  }

  //
  // Equality operators
  //

  template <typename Op>
  void HandleEqualityOp(const TypeId type_id, Value &lhsv, Value &rhsv)
  {
    switch (type_id)
    {
    case TypeId::Bool:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui8, rhsv.variant.ui8);
      break;
    }
    case TypeId::Int8:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i8, rhsv.variant.i8);
      break;
    }
    case TypeId::Byte:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui8, rhsv.variant.ui8);
      break;
    }
    case TypeId::Int16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i16, rhsv.variant.i16);
      break;
    }
    case TypeId::UInt16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui16, rhsv.variant.ui16);
      break;
    }
    case TypeId::Int32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i32, rhsv.variant.i32);
      break;
    }
    case TypeId::UInt32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui32, rhsv.variant.ui32);
      break;
    }
    case TypeId::Int64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i64, rhsv.variant.i64);
      break;
    }
    case TypeId::UInt64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui64, rhsv.variant.ui64);
      break;
    }
    case TypeId::Float32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f32, rhsv.variant.f32);
      break;
    }
    case TypeId::Float64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f64, rhsv.variant.f64);
      break;
    }
    case TypeId::String:
    {
      String *lhs = static_cast<String *>(lhsv.variant.object);
      String *rhs = static_cast<String *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    default:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.object, rhsv.variant.object);
      break;
    }
    }
  }

  //
  // Relational operators
  //

  template <typename Op>
  void HandleRelationalOp(const TypeId type_id, Value &lhsv, Value &rhsv)
  {
    switch (type_id)
    {
    case TypeId::Int8:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i8, rhsv.variant.i8);
      break;
    }
    case TypeId::Byte:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui8, rhsv.variant.ui8);
      break;
    }
    case TypeId::Int16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i16, rhsv.variant.i16);
      break;
    }
    case TypeId::UInt16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui16, rhsv.variant.ui16);
      break;
    }
    case TypeId::Int32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i32, rhsv.variant.i32);
      break;
    }
    case TypeId::UInt32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui32, rhsv.variant.ui32);
      break;
    }
    case TypeId::Int64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i64, rhsv.variant.i64);
      break;
    }
    case TypeId::UInt64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui64, rhsv.variant.ui64);
      break;
    }
    case TypeId::Float32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f32, rhsv.variant.f32);
      break;
    }
    case TypeId::Float64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f64, rhsv.variant.f64);
      break;
    }
    default:
    {
      break;
    }
    }
  }

  //
  // Arithmetic operators
  //

  template <typename Op>
  void HandleArithmeticOp(const TypeId type_id, Value &lhsv, Value &rhsv)
  {
    switch (type_id)
    {
    case TypeId::Int8:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i8, rhsv.variant.i8);
      break;
    }
    case TypeId::Byte:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui8, rhsv.variant.ui8);
      break;
    }
    case TypeId::Int16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i16, rhsv.variant.i16);
      break;
    }
    case TypeId::UInt16:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui16, rhsv.variant.ui16);
      break;
    }
    case TypeId::Int32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i32, rhsv.variant.i32);
      break;
    }
    case TypeId::UInt32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui32, rhsv.variant.ui32);
      break;
    }
    case TypeId::Int64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.i64, rhsv.variant.i64);
      break;
    }
    case TypeId::UInt64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.ui64, rhsv.variant.ui64);
      break;
    }
    case TypeId::Float32:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f32, rhsv.variant.f32);
      break;
    }
    case TypeId::Float64:
    {
      Op::Apply(this, lhsv, rhsv, lhsv.variant.f64, rhsv.variant.f64);
      break;
    }
    case TypeId::String:
    {
      String *lhs = static_cast<String *>(lhsv.variant.object);
      String *rhs = static_cast<String *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float32:
    {
      MatrixFloat32 *lhs = static_cast<MatrixFloat32 *>(lhsv.variant.object);
      MatrixFloat32 *rhs = static_cast<MatrixFloat32 *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float64:
    {
      MatrixFloat64 *lhs = static_cast<MatrixFloat64 *>(lhsv.variant.object);
      MatrixFloat64 *rhs = static_cast<MatrixFloat64 *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float32__Float32:
    {
      MatrixFloat32 *lhs = static_cast<MatrixFloat32 *>(lhsv.variant.object);
      if (lhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhsv.variant.f32);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float64__Float64:
    {
      MatrixFloat64 *lhs = static_cast<MatrixFloat64 *>(lhsv.variant.object);
      if (lhs)
      {
        Op::Apply(this, lhsv, rhsv, lhs, rhsv.variant.f64);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Float32__Matrix_Float32:
    {
      MatrixFloat32 *rhs = static_cast<MatrixFloat32 *>(rhsv.variant.object);
      if (rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhsv.variant.f32, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Float64__Matrix_Float64:
    {
      MatrixFloat64 *rhs = static_cast<MatrixFloat64 *>(rhsv.variant.object);
      if (rhs)
      {
        Op::Apply(this, lhsv, rhsv, lhsv.variant.f64, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    default:
    {
      break;
    }
    }
  }

  //
  // Arithmetic assignment operators
  //

  template <typename Op>
  void HandleArithmeticAssignmentOp(const TypeId type_id, Value &lhsv, Value &rhsv)
  {
    switch (type_id)
    {
    case TypeId::Int8:
    {
      Op::Apply(this, lhsv.variant.i8, rhsv.variant.i8);
      break;
    }
    case TypeId::Byte:
    {
      Op::Apply(this, lhsv.variant.ui8, rhsv.variant.ui8);
      break;
    }
    case TypeId::Int16:
    {
      Op::Apply(this, lhsv.variant.i16, rhsv.variant.i16);
      break;
    }
    case TypeId::UInt16:
    {
      Op::Apply(this, lhsv.variant.ui16, rhsv.variant.ui16);
      break;
    }
    case TypeId::Int32:
    {
      Op::Apply(this, lhsv.variant.i32, rhsv.variant.i32);
      break;
    }
    case TypeId::UInt32:
    {
      Op::Apply(this, lhsv.variant.ui32, rhsv.variant.ui32);
      break;
    }
    case TypeId::Int64:
    {
      Op::Apply(this, lhsv.variant.i64, rhsv.variant.i64);
      break;
    }
    case TypeId::UInt64:
    {
      Op::Apply(this, lhsv.variant.ui64, rhsv.variant.ui64);
      break;
    }
    case TypeId::Float32:
    {
      Op::Apply(this, lhsv.variant.f32, rhsv.variant.f32);
      break;
    }
    case TypeId::Float64:
    {
      Op::Apply(this, lhsv.variant.f64, rhsv.variant.f64);
      break;
    }
    case TypeId::Matrix_Float32:
    {
      MatrixFloat32 *lhs = static_cast<MatrixFloat32 *>(lhsv.variant.object);
      MatrixFloat32 *rhs = static_cast<MatrixFloat32 *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float64:
    {
      MatrixFloat64 *lhs = static_cast<MatrixFloat64 *>(lhsv.variant.object);
      MatrixFloat64 *rhs = static_cast<MatrixFloat64 *>(rhsv.variant.object);
      if (lhs && rhs)
      {
        Op::Apply(this, lhs, rhs);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float32__Float32:
    {
      MatrixFloat32 *lhs = static_cast<MatrixFloat32 *>(lhsv.variant.object);
      if (lhs)
      {
        Op::Apply(this, lhs, rhsv.variant.f32);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    case TypeId::Matrix_Float64__Float64:
    {
      MatrixFloat64 *lhs = static_cast<MatrixFloat64 *>(lhsv.variant.object);
      if (lhs)
      {
        Op::Apply(this, lhs, rhsv.variant.f64);
      }
      else
      {
        RuntimeError("null reference");
      }
      break;
    }
    default:
      break;
    }
  }

  //
  // Indexed assignment
  //

  void HandleIndexedAssignment(const TypeId type_id)
  {
    switch (type_id)
    {
    case TypeId::Matrix_Float32:
    {
      HandleMatrixIndexedAssignment<float>(TypeId::Float32);
      break;
    }
    case TypeId::Matrix_Float64:
    {
      HandleMatrixIndexedAssignment<double>(TypeId::Float64);
      break;
    }
    case TypeId::Array_Bool:
    {
      HandlePrimitiveArrayIndexedAssignment<uint8_t>(TypeId::Bool);
      break;
    }
    case TypeId::Array_Int8:
    {
      HandlePrimitiveArrayIndexedAssignment<int8_t>(TypeId::Int8);
      break;
    }
    case TypeId::Array_Byte:
    {
      HandlePrimitiveArrayIndexedAssignment<uint8_t>(TypeId::Byte);
      break;
    }
    case TypeId::Array_Int16:
    {
      HandlePrimitiveArrayIndexedAssignment<int16_t>(TypeId::Int16);
      break;
    }
    case TypeId::Array_UInt16:
    {
      HandlePrimitiveArrayIndexedAssignment<uint16_t>(TypeId::UInt16);
      break;
    }
    case TypeId::Array_Int32:
    {
      HandlePrimitiveArrayIndexedAssignment<int32_t>(TypeId::Int32);
      break;
    }
    case TypeId::Array_UInt32:
    {
      HandlePrimitiveArrayIndexedAssignment<uint32_t>(TypeId::UInt32);
      break;
    }
    case TypeId::Array_Int64:
    {
      HandlePrimitiveArrayIndexedAssignment<int64_t>(TypeId::Int64);
      break;
    }
    case TypeId::Array_UInt64:
    {
      HandlePrimitiveArrayIndexedAssignment<uint64_t>(TypeId::UInt64);
      break;
    }
    case TypeId::Array_Float32:
    {
      HandlePrimitiveArrayIndexedAssignment<float>(TypeId::Float32);
      break;
    }
    case TypeId::Array_Float64:
    {
      HandlePrimitiveArrayIndexedAssignment<double>(TypeId::Float64);
      break;
    }
    case TypeId::Array_String:
    {
      HandleObjectArrayIndexedAssignment(TypeId::String);
      break;
    }
    case TypeId::Array_Matrix_Float32:
    {
      HandleObjectArrayIndexedAssignment(TypeId::Matrix_Float32);
      break;
    }
    case TypeId::Array_Matrix_Float64:
    {
      HandleObjectArrayIndexedAssignment(TypeId::Matrix_Float64);
      break;
    }
    case TypeId::Array:
    {
      HandleObjectArrayIndexedAssignment(TypeId::Array);
      break;
    }
    default:
    {
      break;
    }
    }
  }

  template <typename ElementType>
  void HandleMatrixIndexedAssignment(const TypeId /*type_id*/)
  {
    ElementType *ptr;
    if (GetMatrixElement(ptr) == false)
    {
      return;
    }
    Value &matrixv = stack_[sp_--];
    Value &rhsv    = stack_[sp_--];
    rhsv.variant.Get(*ptr);
    matrixv.Reset();
    rhsv.PrimitiveReset();
  }

  template <typename ElementType>
  void HandlePrimitiveArrayIndexedAssignment(const TypeId /*type_id*/)
  {
    ElementType *ptr;
    if (GetArrayElement<ElementType>(ptr) == false)
    {
      return;
    }
    Value &arrayv = stack_[sp_--];
    Value &rhsv   = stack_[sp_--];
    rhsv.variant.Get(*ptr);
    arrayv.Reset();
    rhsv.PrimitiveReset();
  }

  // Move RHS to LHS
  void Move(Object *&lhs, Object *&rhs)
  {
    if (lhs != rhs)
    {
      if (lhs)
      {
        lhs->Release();
      }
      lhs = rhs;
    }
  }

  void HandleObjectArrayIndexedAssignment(const TypeId /*type_id*/)
  {
    Object **ptr;
    if (GetArrayElement<Object *>(ptr) == false)
    {
      return;
    }
    Value & arrayv = stack_[sp_--];
    Value & rhsv   = stack_[sp_--];
    Object *rhs;
    rhsv.variant.Get(rhs);
    Move(*ptr, rhs);
    arrayv.Reset();
    rhsv.type_id = TypeId::Unknown;
    rhsv.variant.Zero();
  }

  //
  // Index
  //

  void HandleIndexOp(const TypeId type_id)
  {
    switch (type_id)
    {
    case TypeId::Matrix_Float32:
    {
      HandleMatrixIndexOp<float>(TypeId::Float32);
      break;
    }
    case TypeId::Matrix_Float64:
    {
      HandleMatrixIndexOp<double>(TypeId::Float64);
      break;
    }
    case TypeId::Array_Bool:
    {
      HandlePrimitiveArrayIndexOp<uint8_t>(TypeId::Bool);
      break;
    }
    case TypeId::Array_Int8:
    {
      HandlePrimitiveArrayIndexOp<int8_t>(TypeId::Int8);
      break;
    }
    case TypeId::Array_Byte:
    {
      HandlePrimitiveArrayIndexOp<uint8_t>(TypeId::Byte);
      break;
    }
    case TypeId::Array_Int16:
    {
      HandlePrimitiveArrayIndexOp<int16_t>(TypeId::Int16);
      break;
    }
    case TypeId::Array_UInt16:
    {
      HandlePrimitiveArrayIndexOp<uint16_t>(TypeId::UInt16);
      break;
    }
    case TypeId::Array_Int32:
    {
      HandlePrimitiveArrayIndexOp<int32_t>(TypeId::Int32);
      break;
    }
    case TypeId::Array_UInt32:
    {
      HandlePrimitiveArrayIndexOp<uint32_t>(TypeId::UInt32);
      break;
    }
    case TypeId::Array_Int64:
    {
      HandlePrimitiveArrayIndexOp<int64_t>(TypeId::Int64);
      break;
    }
    case TypeId::Array_UInt64:
    {
      HandlePrimitiveArrayIndexOp<uint64_t>(TypeId::UInt64);
      break;
    }
    case TypeId::Array_Float32:
    {
      HandlePrimitiveArrayIndexOp<float>(TypeId::Float32);
      break;
    }
    case TypeId::Array_Float64:
    {
      HandlePrimitiveArrayIndexOp<double>(TypeId::Float64);
      break;
    }
    case TypeId::Array_String:
    {
      HandleObjectArrayIndexOp(TypeId::Array_String);
      break;
    }
    case TypeId::Array_Matrix_Float32:
    {
      HandleObjectArrayIndexOp(TypeId::Matrix_Float32);
      break;
    }
    case TypeId::Array_Matrix_Float64:
    {
      HandleObjectArrayIndexOp(TypeId::Matrix_Float64);
      break;
    }
    case TypeId::Array:
    {
      HandleObjectArrayIndexOp(TypeId::Array);
      break;
    }
    default:
    {
      break;
    }
    }
  }

  template <typename ElementType>
  void HandleMatrixIndexOp(const TypeId type_id)
  {
    ElementType *ptr;
    if (GetMatrixElement(ptr) == false)
    {
      return;
    }
    ElementType element = *ptr;
    Value &     matrixv = stack_[sp_];
    matrixv.Release();
    matrixv.type_id = type_id;
    matrixv.variant.Set(element);
  }

  template <typename ElementType>
  void HandlePrimitiveArrayIndexOp(const TypeId type_id)
  {
    ElementType *ptr;
    if (GetArrayElement<ElementType>(ptr) == false)
    {
      return;
    }
    ElementType element = *ptr;
    Value &     arrayv  = stack_[sp_];
    arrayv.Release();
    arrayv.type_id = type_id;
    arrayv.variant.Set(element);
  }

  void HandleObjectArrayIndexOp(const TypeId type_id)
  {
    Object **ptr;
    if (GetArrayElement<Object *>(ptr) == false)
    {
      return;
    }
    Object *object = *ptr;
    if (object)
    {
      object->AddRef();
    }
    Value &arrayv = stack_[sp_];
    arrayv.Release();
    arrayv.type_id        = type_id;
    arrayv.variant.object = object;
  }

  //
  // Indexed arithmetic assignment
  //

  // matrix[i, j] += number
  // intarray[i] += number
  // matrixarray[i] += matrix
  // matrixarray[i] += number
  template <typename Op>
  void HandleIndexedArithmeticAssignmentOp(const TypeId type_id)
  {
    switch (type_id)
    {
    case TypeId::Matrix_Float32:
    {
      HandleMatrixIndexedArithmeticAssignmentOp<Op, float>();
      break;
    }
    case TypeId::Matrix_Float64:
    {
      HandleMatrixIndexedArithmeticAssignmentOp<Op, double>();
      break;
    }
    case TypeId::Array_Int8:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, int8_t>();
      break;
    }
    case TypeId::Array_Byte:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, uint8_t>();
      break;
    }
    case TypeId::Array_Int16:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, int16_t>();
      break;
    }
    case TypeId::Array_UInt16:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, uint16_t>();
      break;
    }
    case TypeId::Array_Int32:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, int32_t>();
      break;
    }
    case TypeId::Array_UInt32:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, uint32_t>();
      break;
    }
    case TypeId::Array_Int64:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, int64_t>();
      break;
    }
    case TypeId::Array_UInt64:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, uint64_t>();
      break;
    }
    case TypeId::Array_Float32:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, float>();
      break;
    }
    case TypeId::Array_Float64:
    {
      HandlePrimitiveArrayIndexedArithmeticAssignmentOp<Op, double>();
      break;
    }
    case TypeId::Array_Matrix_Float32:
    {
      HandleObjectArrayIndexedArithmeticAssignmentOp<Op, MatrixFloat32 *, Object *,
                                                     MatrixFloat32 *>();
      break;
    }
    case TypeId::Array_Matrix_Float64:
    {
      HandleObjectArrayIndexedArithmeticAssignmentOp<Op, MatrixFloat64 *, Object *,
                                                     MatrixFloat64 *>();
      break;
    }
    case TypeId::Array_Matrix_Float32__Float32:
    {
      HandleObjectArrayIndexedArithmeticAssignmentOp<Op, MatrixFloat32 *, float, float>();
      break;
    }
    case TypeId::Array_Matrix_Float64__Float64:
    {
      HandleObjectArrayIndexedArithmeticAssignmentOp<Op, MatrixFloat64 *, double, double>();
      break;
    }
    default:
      break;
    }
  }

  template <typename Op, typename ElementType>
  void HandleMatrixIndexedArithmeticAssignmentOp()
  {
    ElementType *ptr;
    if (GetMatrixElement(ptr) == false)
    {
      return;
    }
    Value &     matrixv = stack_[sp_--];
    Value &     rhsv    = stack_[sp_--];
    ElementType rhs;
    rhsv.variant.Get(rhs);
    Op::Apply(this, *ptr, rhs);
    matrixv.Reset();
    rhsv.Reset();
  }

  template <typename Op, typename ElementType>
  void HandlePrimitiveArrayIndexedArithmeticAssignmentOp()
  {
    ElementType *ptr;
    if (GetArrayElement<ElementType>(ptr) == false)
    {
      return;
    }
    Value &     arrayv = stack_[sp_--];
    Value &     rhsv   = stack_[sp_--];
    ElementType rhs;
    rhsv.variant.Get(rhs);
    Op::Apply(this, *ptr, rhs);
    arrayv.Reset();
    rhsv.Reset();
  }

  template <typename Op, typename ElementType, typename RHSVariantType, typename RHSElementType>
  void HandleObjectArrayIndexedArithmeticAssignmentOp()
  {
    ElementType *ptr;
    if (GetArrayElement<ElementType>(ptr) == false)
    {
      return;
    }
    Value &        arrayv = stack_[sp_--];
    Value &        rhsv   = stack_[sp_--];
    RHSVariantType xx;
    rhsv.variant.Get(xx);
    RHSElementType rhs = static_cast<RHSElementType>(xx);
    Op::Apply(this, *ptr, rhs);
    arrayv.Reset();
    rhsv.Reset();
  }

  //
  // Prefix/postfix index operations
  //

  template <typename Op>
  void HandleIndexedPrefixPostfixOp(const TypeId type_id)
  {
    switch (type_id)
    {
    case TypeId::Array_Int8:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, int8_t>(TypeId::Int8);
      break;
    }
    case TypeId::Array_Byte:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, uint8_t>(TypeId::Byte);
      break;
    }
    case TypeId::Array_Int16:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, int16_t>(TypeId::Int16);
      break;
    }
    case TypeId::Array_UInt16:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, uint16_t>(TypeId::UInt16);
      break;
    }
    case TypeId::Array_Int32:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, int32_t>(TypeId::Int32);
      break;
    }
    case TypeId::Array_UInt32:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, uint32_t>(TypeId::UInt32);
      break;
    }
    case TypeId::Array_Int64:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, int64_t>(TypeId::Int64);
      break;
    }
    case TypeId::Array_UInt64:
    {
      HandleIndexedPrefixPostfixOpHelper<Op, uint64_t>(TypeId::UInt64);
      break;
    }
    default:
      break;
    }
  }

  template <typename Op, typename ElementType>
  void HandleIndexedPrefixPostfixOpHelper(const TypeId type_id)
  {
    ElementType *ptr;
    if (GetArrayElement<ElementType>(ptr) == false)
    {
      return;
    }
    ElementType element;
    Op::Apply(this, element, *ptr);  // what if fails?
    Value &arrayv = stack_[sp_];
    arrayv.Release();
    arrayv.type_id = type_id;
    arrayv.variant.Set(element);
  }

  //
  // Indexing helpers
  //

  bool GetIndex(const Value &value, uint64_t &index)
  {
    bool ok = true;
    switch (value.type_id)
    {
    case TypeId::Int8:
    {
      index = uint64_t(value.variant.i8);
      ok    = value.variant.i8 >= 0;
      break;
    }
    case TypeId::Byte:
    {
      index = uint64_t(value.variant.ui8);
      break;
    }
    case TypeId::Int16:
    {
      index = uint64_t(value.variant.i16);
      ok    = value.variant.i16 >= 0;
      break;
    }
    case TypeId::UInt16:
    {
      index = uint64_t(value.variant.ui16);
      break;
    }
    case TypeId::Int32:
    {
      index = uint64_t(value.variant.i32);
      ok    = value.variant.i32 >= 0;
      break;
    }
    case TypeId::UInt32:
    {
      index = uint64_t(value.variant.ui32);
      break;
    }
    case TypeId::Int64:
    {
      index = uint64_t(value.variant.i64);
      ok    = value.variant.i64 >= 0;
      break;
    }
    case TypeId::UInt64:
    {
      index = value.variant.ui64;
      break;
    }
    default:
    {
      ok = false;
      break;
    }
    }
    return ok;
  }

  template <typename ElementType>
  bool GetMatrixElement(ElementType *&ptr)
  {
    Value &  columnv = stack_[sp_--];
    uint64_t column;
    if (GetIndex(columnv, column) == false)
    {
      RuntimeError("negative index");
      return false;
    }
    columnv.PrimitiveReset();
    Value &  rowv = stack_[sp_--];
    uint64_t row;
    if (GetIndex(rowv, row) == false)
    {
      RuntimeError("negative index");
      return false;
    }
    rowv.PrimitiveReset();
    Value &              matrixv = stack_[sp_];
    Matrix<ElementType> *m       = static_cast<Matrix<ElementType> *>(matrixv.variant.object);
    if (m == nullptr)
    {
      RuntimeError("null reference");
      return false;
    }
    const uint64_t rows    = m->matrix.height();
    const uint64_t columns = m->matrix.width();
    if ((row >= rows) || (column >= columns))
    {
      RuntimeError("index out of bounds");
      return false;
    }
    ptr = &(m->matrix.At(row, column));
    return true;
  }

  template <typename ElementType>
  bool GetArrayElement(ElementType *&ptr)
  {
    Value &  positionv = stack_[sp_--];
    uint64_t position;
    if (GetIndex(positionv, position) == false)
    {
      RuntimeError("negative index");
      return false;
    }
    positionv.PrimitiveReset();
    Value &             arrayv = stack_[sp_];
    Array<ElementType> *array  = static_cast<Array<ElementType> *>(arrayv.variant.object);
    if (array == nullptr)
    {
      RuntimeError("null reference");
      return false;
    }
    if (position >= array->elements.size())
    {
      RuntimeError("index out of bounds");
      return false;
    }
    ptr = static_cast<ElementType *>(&array->elements[position]);
    return true;
  }

  //
  // Matrix operations
  //

  template <typename M>
  void MatrixMatrixAdd(Value &lhsv, Value &rhsv, M *lhs, M *rhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    const size_t rhs_rows                 = rhs->matrix.height();
    const size_t rhs_columns              = rhs->matrix.width();
    const bool   rhs_matrix_is_modifiable = rhs->count == 1;
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineAdd(rhs->matrix);
      return;
    }
    if (rhs_matrix_is_modifiable)
    {
      rhs->matrix.InlineAdd(lhs->matrix);
      lhsv = std::move(rhsv);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Add(lhs->matrix, rhs->matrix, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M, typename T>
  void MatrixNumberAdd(Value &lhsv, M *lhs, T rhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineAdd(rhs);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Add(lhs->matrix, rhs, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M>
  void MatrixMatrixSubtract(Value &lhsv, Value &rhsv, M *lhs, M *rhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    const size_t rhs_rows                 = rhs->matrix.height();
    const size_t rhs_columns              = rhs->matrix.width();
    const bool   rhs_matrix_is_modifiable = rhs->count == 1;
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineSubtract(rhs->matrix);
      return;
    }
    if (rhs_matrix_is_modifiable)
    {
      rhs->matrix.InlineReverseSubtract(lhs->matrix);
      lhsv = std::move(rhsv);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Subtract(lhs->matrix, rhs->matrix, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M, typename T>
  void MatrixNumberSubtract(Value &lhsv, M *lhs, T rhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineSubtract(rhs);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Subtract(lhs->matrix, rhs, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M>
  void MatrixMatrixMultiply(Value &lhsv, Value & /*rhsv*/, M *lhs, M *rhs)
  {
    const size_t lhs_rows    = lhs->matrix.height();
    const size_t lhs_columns = lhs->matrix.width();
    const size_t rhs_rows    = rhs->matrix.height();
    const size_t rhs_columns = rhs->matrix.width();
    if (lhs_columns != rhs_rows)
    {
      RuntimeError("invalid operation");
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, rhs_columns, m);
    // TODO(tfr): use blas
    TODO_FAIL("Use BLAS TODO");
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M, typename T>
  void MatrixNumberMultiply(Value &lhsv, M *lhs, T rhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineMultiply(rhs);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Multiply(lhs->matrix, rhs, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename T, typename M>
  void NumberMatrixMultiply(Value &lhsv, Value &rhsv, T lhs, M *rhs)
  {
    const size_t rhs_rows                 = rhs->matrix.height();
    const size_t rhs_columns              = rhs->matrix.width();
    const bool   rhs_matrix_is_modifiable = rhs->count == 1;
    if (rhs_matrix_is_modifiable)
    {
      rhs->matrix.InlineMultiply(lhs);
      lhsv = std::move(rhsv);
      return;
    }
    M *m;
    AcquireMatrix(rhs_rows, rhs_columns, m);
    Multiply(rhs->matrix, lhs, m->matrix);
    lhsv.SetObject(m, rhsv.type_id);
  }

  template <typename M, typename T>
  void MatrixNumberDivide(Value &lhsv, M *lhs, T rhs)
  {
    if (math::IsZero(rhs))
    {
      RuntimeError("division by zero");
      return;
    }
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineDivide(rhs);
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    Divide(lhs->matrix, rhs, m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M, typename T>
  void MatrixUnaryMinus(Value &lhsv, M *lhs)
  {
    const size_t lhs_rows                 = lhs->matrix.height();
    const size_t lhs_columns              = lhs->matrix.width();
    const bool   lhs_matrix_is_modifiable = lhs->count == 1;
    if (lhs_matrix_is_modifiable)
    {
      // TODO(tfr): implement unary minus
      // is there an inplace op for this?
      lhs->matrix.InlineMultiply(T(-1));
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, lhs_columns, m);
    // is there a call for this?
    Multiply(lhs->matrix, T(-1), m->matrix);
    lhsv.SetObject(m, lhsv.type_id);
  }

  template <typename M>
  void MatrixMatrixAddAssign(M *lhs, M *rhs)
  {
    const size_t lhs_rows    = lhs->matrix.height();
    const size_t lhs_columns = lhs->matrix.width();
    const size_t rhs_rows    = rhs->matrix.height();
    const size_t rhs_columns = rhs->matrix.width();
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineAdd(rhs->matrix);
  }

  template <typename M, typename T>
  void MatrixNumberAddAssign(M *lhs, T rhs)
  {
    lhs->matrix.InlineAdd(rhs);
  }

  template <typename M>
  void MatrixMatrixSubtractAssign(M *lhs, M *rhs)
  {
    const size_t lhs_rows    = lhs->matrix.height();
    const size_t lhs_columns = lhs->matrix.width();
    const size_t rhs_rows    = rhs->matrix.height();
    const size_t rhs_columns = rhs->matrix.width();
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineSubtract(rhs->matrix);
  }

  template <typename M, typename T>
  void MatrixNumberSubtractAssign(M *lhs, T rhs)
  {
    lhs->matrix.InlineSubtract(rhs);
  }

  template <typename M>
  void MatrixMatrixMultiplyAssign(M *lhs, M *rhs)
  {
    const size_t lhs_rows    = lhs->matrix.height();
    const size_t lhs_columns = lhs->matrix.width();
    const size_t rhs_rows    = rhs->matrix.height();
    const size_t rhs_columns = rhs->matrix.width();
    if (lhs_columns != rhs_rows)
    {
      RuntimeError("invalid operation");
      return;
    }
    M *m;
    AcquireMatrix(lhs_rows, rhs_columns, m);
    // TODO(tfr): Use blas
    TODO_FAIL("Use BLAS");
    lhs->Release();
    lhs = m;
  }

  template <typename M, typename T>
  void MatrixNumberMultiplyAssign(M *lhs, T rhs)
  {
    lhs->matrix.InlineMultiply(rhs);
  }

  template <typename M, typename T>
  void MatrixNumberDivideAssign(M *lhs, T rhs)
  {
    if (math::IsNonZero(rhs))
    {
      lhs->matrix.InlineDivide(rhs);
      return;
    }
    RuntimeError("division by zero");
  }

  //
  // Ops
  //

  struct EqualOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsEqual(lhs, rhs)), TypeId::Bool);
    }
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, String *lhs, String *rhs)
    {
      const uint8_t value = uint8_t(lhs->str == rhs->str);
      lhsv.SetPrimitive(value, TypeId::Bool);
    }
  };

  struct NotEqualOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsNotEqual(lhs, rhs)), TypeId::Bool);
    }
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, String *lhs, String *rhs)
    {
      const uint8_t value = uint8_t(lhs->str != rhs->str);
      lhsv.SetPrimitive(value, TypeId::Bool);
    }
  };

  struct LessThanOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsLessThan(lhs, rhs)), TypeId::Bool);
    }
  };

  struct LessThanOrEqualOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsLessThanOrEqual(lhs, rhs)), TypeId::Bool);
    }
  };

  struct GreaterThanOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsGreaterThan(lhs, rhs)), TypeId::Bool);
    }
  };

  struct GreaterThanOrEqualOp
  {
    template <typename T>
    static void Apply(VM * /*vm*/, Value &lhsv, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhsv.SetPrimitive(uint8_t(math::IsGreaterThanOrEqual(lhs, rhs)), TypeId::Bool);
    }
  };

  struct AddOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhs = T(lhs + rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value &rhsv, M *lhs, M *rhs)
    {
      vm->MatrixMatrixAdd(lhsv, rhsv, lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, M *lhs, T &rhs)
    {
      vm->MatrixNumberAdd(lhsv, lhs, rhs);
    }
    template <typename T, typename M,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr,
              typename std::enable_if<IsMatrix<M>::value>::type *           = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T & /*lhs*/, M * /*rhs*/)
    {}
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, String *lhs, String *rhs)
    {
      if (lhs->count == 1)
      {
        lhs->str += rhs->str;
      }
      else
      {
        String *s = new String(vm, lhs->str + rhs->str, false);
        lhs->Release();
        lhsv.variant.object = s;
      }
    }
  };

  struct SubtractOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhs = T(lhs - rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value &rhsv, M *lhs, M *rhs)
    {
      vm->MatrixMatrixSubtract(lhsv, rhsv, lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, M *lhs, T &rhs)
    {
      vm->MatrixNumberSubtract(lhsv, lhs, rhs);
    }
    template <typename T, typename M,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr,
              typename std::enable_if<IsMatrix<M>::value>::type *           = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T & /*lhs*/, M * /*rhs*/)
    {}
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, String * /*lhs*/,
                      String * /*rhs*/)
    {}
  };

  struct MultiplyOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      lhs = T(lhs * rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value &rhsv, M *lhs, M *rhs)
    {
      vm->MatrixMatrixMultiply(lhsv, rhsv, lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, M *lhs, T &rhs)
    {
      vm->MatrixNumberMultiply(lhsv, lhs, rhs);
    }
    template <typename T, typename M,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr,
              typename std::enable_if<IsMatrix<M>::value>::type *           = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value &rhsv, T &lhs, M *rhs)
    {
      vm->NumberMatrixMultiply(lhsv, rhsv, lhs, rhs);
    }
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, String * /*lhs*/,
                      String * /*rhs*/)
    {}
  };

  struct DivideOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value & /*lhsv*/, Value & /*rhsv*/, T &lhs, T &rhs)
    {
      if (math::IsNonZero(rhs))
      {
        lhs = T(lhs / rhs);
        return;
      }
      vm->RuntimeError("division by zero");
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, M *lhs, T &rhs)
    {
      vm->MatrixNumberDivide(lhsv, lhs, rhs);
    }
    template <typename T, typename M,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr,
              typename std::enable_if<IsMatrix<M>::value>::type *           = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T & /*lhs*/, M * /*rhs*/)
    {}
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, String * /*lhs*/,
                      String * /*rhs*/)
    {}
  };

  struct UnaryMinusOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T &lhs, T & /*rhs*/)
    {
      lhs = T(-lhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, Value &lhsv, Value & /*rhsv*/, M *lhs, T & /*rhs*/)
    {
      vm->MatrixUnaryMinus<M, T>(lhsv, lhs);
    }
    template <typename T, typename M,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr,
              typename std::enable_if<IsMatrix<M>::value>::type *           = nullptr>
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, T & /*lhs*/, M * /*rhs*/)
    {}
    static void Apply(VM * /*vm*/, Value & /*lhsv*/, Value & /*rhsv*/, String * /*lhs*/,
                      String * /*rhs*/)
    {}
  };

  struct AddAssignOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = T(lhs + rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, M *rhs)
    {
      vm->MatrixMatrixAddAssign(lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, T &rhs)
    {
      vm->MatrixNumberAddAssign(lhs, rhs);
    }
  };

  struct SubtractAssignOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = T(lhs - rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, M *rhs)
    {
      vm->MatrixMatrixSubtractAssign(lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, T &rhs)
    {
      vm->MatrixNumberSubtractAssign(lhs, rhs);
    }
  };

  struct MultiplyAssignOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = T(lhs * rhs);
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, M *rhs)
    {
      vm->MatrixMatrixMultiplyAssign(lhs, rhs);
    }
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, T &rhs)
    {
      vm->MatrixNumberMultiplyAssign(lhs, rhs);
    }
  };

  struct DivideAssignOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, T &lhs, T &rhs)
    {
      if (math::IsNonZero(rhs))
      {
        lhs = T(lhs / rhs);
        return;
      }
      vm->RuntimeError("division by zero");
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM *vm, M *lhs, T &rhs)
    {
      vm->MatrixNumberDivideAssign(lhs, rhs);
    }
  };

  struct PrefixIncOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = ++rhs;
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, T & /*rhs*/)
    {}
  };

  struct PrefixDecOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = --rhs;
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, T & /*rhs*/)
    {}
  };

  struct PostfixIncOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = rhs++;
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, T & /*rhs*/)
    {}
  };

  struct PostfixDecOp
  {
    template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, T &lhs, T &rhs)
    {
      lhs = rhs--;
    }
    template <typename M, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, M * /*rhs*/)
    {}
    template <typename M, typename T, typename std::enable_if<IsMatrix<M>::value>::type * = nullptr,
              typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    static void Apply(VM * /*vm*/, M * /*lhs*/, T & /*rhs*/)
    {}
  };
};

}  // namespace vm
}  // namespace fetch
