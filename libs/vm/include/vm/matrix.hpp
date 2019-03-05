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

#include "math/free_functions/free_functions.hpp"
#include "math/tensor.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IMatrix : public Object
{
public:
  IMatrix()          = delete;
  virtual ~IMatrix() = default;
  static Ptr<IMatrix> Constructor(VM *vm, TypeId type_id, int32_t rows, int32_t columns);

protected:
  IMatrix(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

template <typename T>
struct Matrix : public IMatrix
{
  Matrix()          = delete;
  virtual ~Matrix() = default;

  Matrix(VM *vm, TypeId type_id, size_t rows, size_t columns)
    : IMatrix(vm, type_id)
    , matrix(std::vector<typename fetch::math::Tensor<T>::SizeType>(columns, rows))
  {}

  static Ptr<Matrix> AcquireMatrix(VM *vm, TypeId type_id, size_t rows, size_t columns)
  {
    return Ptr<Matrix>(new Matrix(vm, type_id, rows, columns));
  }

  virtual void RightAdd(Variant &lhsv, Variant &rhsv) override
  {
    bool const   lhs_matrix_is_modifiable = lhsv.object.RefCount() == 1;
    Ptr<Matrix>  lhs                      = lhsv.object;
    T            rhs                      = rhsv.primitive.Get<T>();
    size_t const lhs_rows                 = lhs->matrix.shape()[0];
    size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineAdd(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Add(lhs->matrix, rhs, m->matrix);
    lhsv.Assign(std::move(m), lhsv.type_id);
  }

  virtual void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    bool const   lhs_matrix_is_modifiable = lhso.RefCount() == 1;
    bool const   rhs_matrix_is_modifiable = rhso.RefCount() == 1;
    Ptr<Matrix>  lhs                      = lhso;
    Ptr<Matrix>  rhs                      = rhso;
    size_t const lhs_rows                 = lhs->matrix.shape()[0];
    size_t const lhs_columns              = lhs->matrix.shape()[1];
    size_t const rhs_rows                 = rhs->matrix.shape()[0];
    size_t const rhs_columns              = rhs->matrix.shape()[1];
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
      lhso = std::move(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Add(lhs->matrix, rhs->matrix, m->matrix);
    lhso = std::move(m);
  }

  virtual void RightAddAssign(Ptr<Object> &lhso, Variant &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineAdd(rhs);
  }

  virtual void AddAssign(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<Matrix>  lhs         = lhso;
    Ptr<Matrix>  rhs         = rhso;
    size_t const lhs_rows    = lhs->matrix.shape()[0];
    size_t const lhs_columns = lhs->matrix.shape()[1];
    size_t const rhs_rows    = rhs->matrix.shape()[0];
    size_t const rhs_columns = rhs->matrix.shape()[1];
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineAdd(rhs->matrix);
  }

  virtual void RightSubtract(Variant &lhsv, Variant &rhsv) override
  {
    bool const   lhs_matrix_is_modifiable = lhsv.object.RefCount() == 1;
    Ptr<Matrix>  lhs                      = lhsv.object;
    T            rhs                      = rhsv.primitive.Get<T>();
    size_t const lhs_rows                 = lhs->matrix.shape()[0];
    size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineSubtract(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Subtract(lhs->matrix, rhs, m->matrix);
    lhsv.Assign(std::move(m), lhsv.type_id);
  }

  virtual void Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    bool const   lhs_matrix_is_modifiable = lhso.RefCount() == 1;
    bool const   rhs_matrix_is_modifiable = rhso.RefCount() == 1;
    Ptr<Matrix>  lhs                      = lhso;
    Ptr<Matrix>  rhs                      = rhso;
    size_t const lhs_rows                 = lhs->matrix.shape()[0];
    size_t const lhs_columns              = lhs->matrix.shape()[1];
    size_t const rhs_rows                 = rhs->matrix.shape()[0];
    size_t const rhs_columns              = rhs->matrix.shape()[1];
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
      lhso = std::move(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Subtract(lhs->matrix, rhs->matrix, m->matrix);
    lhso = std::move(m);
  }

  virtual void RightSubtractAssign(Ptr<Object> &lhso, Variant &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineSubtract(rhs);
  }

  virtual void SubtractAssign(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<Matrix>  lhs         = lhso;
    Ptr<Matrix>  rhs         = rhso;
    size_t const lhs_rows    = lhs->matrix.shape()[0];
    size_t const lhs_columns = lhs->matrix.shape()[1];
    size_t const rhs_rows    = rhs->matrix.shape()[0];
    size_t const rhs_columns = rhs->matrix.shape()[1];
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineSubtract(rhs->matrix);
  }

  virtual void LeftMultiply(Variant &lhsv, Variant &rhsv) override
  {
    bool const   rhs_matrix_is_modifiable = rhsv.object.RefCount() == 1;
    T            lhs                      = lhsv.primitive.Get<T>();
    Ptr<Matrix>  rhs                      = rhsv.object;
    size_t const rhs_rows                 = rhs->matrix.shape()[0];
    size_t const rhs_columns              = rhs->matrix.shape()[1];
    if (rhs_matrix_is_modifiable)
    {
      rhs->matrix.InlineMultiply(lhs);
      lhsv = std::move(rhsv);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, rhs_rows, rhs_columns);
    fetch::math::Multiply(rhs->matrix, lhs, m->matrix);
    lhsv.Assign(std::move(m), rhsv.type_id);
  }

  virtual void RightMultiply(Variant &lhsv, Variant &rhsv) override
  {
    bool const   lhs_matrix_is_modifiable = lhsv.object.RefCount() == 1;
    Ptr<Matrix>  lhs                      = lhsv.object;
    T            rhs                      = rhsv.primitive.Get<T>();
    size_t const lhs_rows                 = lhs->matrix.shape()[0];
    size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineMultiply(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Multiply(lhs->matrix, rhs, m->matrix);
    lhsv.Assign(std::move(m), lhsv.type_id);
  }

  virtual void Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<Matrix>  lhs         = lhso;
    Ptr<Matrix>  rhs         = rhso;
    size_t const lhs_rows    = lhs->matrix.shape()[0];
    size_t const lhs_columns = lhs->matrix.shape()[1];
    size_t const rhs_rows    = rhs->matrix.shape()[0];
    size_t const rhs_columns = rhs->matrix.shape()[1];
    if (lhs_columns != rhs_rows)
    {
      RuntimeError("invalid operation");
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, rhs_columns);
    // TODO(tfr): use blas
    TODO_FAIL("Use BLAS TODO");
    lhso = std::move(m);
  }

  virtual void RightMultiplyAssign(Ptr<Object> &lhso, Variant &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineMultiply(rhs);
  }

  virtual void MultiplyAssign(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<Matrix>  lhs         = lhso;
    Ptr<Matrix>  rhs         = rhso;
    size_t const lhs_rows    = lhs->matrix.shape()[0];
    size_t const lhs_columns = lhs->matrix.shape()[1];
    size_t const rhs_rows    = rhs->matrix.shape()[0];
    size_t const rhs_columns = rhs->matrix.shape()[1];
    if (lhs_columns != rhs_rows)
    {
      RuntimeError("invalid operation");
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, rhs_columns);
    // TODO(tfr): Use blas
    TODO_FAIL("Use BLAS");
    lhso = std::move(m);
  }

  virtual void RightDivide(Variant &lhsv, Variant &rhsv) override
  {
    bool const  lhs_matrix_is_modifiable = lhsv.object.RefCount() == 1;
    Ptr<Matrix> lhs                      = lhsv.object;
    T           rhs                      = rhsv.primitive.Get<T>();
    if (math::IsZero(rhs))
    {
      RuntimeError("division by zero");
      return;
    }
    size_t const lhs_rows    = lhs->matrix.shape()[0];
    size_t const lhs_columns = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineDivide(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, lhs_rows, lhs_columns);
    fetch::math::Divide(lhs->matrix, rhs, m->matrix);
    lhsv.Assign(std::move(m), lhsv.type_id);
  }

  virtual void RightDivideAssign(Ptr<Object> &lhso, Variant &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    if (math::IsNonZero(rhs))
    {
      lhs->matrix.InlineDivide(rhs);
      return;
    }
    RuntimeError("division by zero");
  }

  virtual void UnaryMinus(Ptr<Object> &object) override
  {
    bool const   matrix_is_modifiable = object.RefCount() == 1;
    Ptr<Matrix>  operand              = object;
    size_t const rows                 = operand->matrix.shape()[0];
    size_t const columns              = operand->matrix.shape()[1];

    // TODO(tfr): implement unary minus
    if (matrix_is_modifiable)
    {
      operand->matrix.InlineMultiply(T(-1));
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, rows, columns);
    fetch::math::Multiply(operand->matrix, T(-1), m->matrix);
    object = std::move(m);
  }

  T *Find()
  {
    Variant &columnv = Pop();
    size_t   column;
    if (GetNonNegativeInteger(columnv, column) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }
    columnv.Reset();
    Variant &rowv = Pop();
    size_t   row;
    if (GetNonNegativeInteger(rowv, row) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }
    rowv.Reset();
    size_t const rows    = matrix.shape()[0];
    size_t const columns = matrix.shape()[1];
    if ((row >= rows) || (column >= columns))
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    return &matrix.At(std::vector<typename fetch::math::Tensor<T>::SizeType>(column, row));
  }

  virtual void *FindElement() override
  {
    return Find();
  }

  virtual void PushElement(TypeId element_type_id) override
  {
    T *ptr = Find();
    if (ptr)
    {
      Variant &top = Push();
      top.Construct(*ptr, element_type_id);
    }
  }

  virtual void PopToElement() override
  {
    T *ptr = Find();
    if (ptr)
    {
      Variant &top = Pop();
      *ptr         = top.Move<T>();
    }
  }

  fetch::math::Tensor<T> matrix;
};

inline Ptr<IMatrix> IMatrix::Constructor(VM *vm, TypeId type_id, int32_t rows, int32_t columns)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.parameter_type_ids[0];
  if ((rows < 0) || (columns < 0))
  {
    vm->RuntimeError("negative size");
    return Ptr<IMatrix>();
  }
  if (element_type_id == TypeIds::Float32)
  {
    return Ptr<IMatrix>(new Matrix<float>(vm, type_id, size_t(rows), size_t(columns)));
  }
  else
  {
    return Ptr<IMatrix>(new Matrix<double>(vm, type_id, size_t(rows), size_t(columns)));
  }
}

}  // namespace vm
}  // namespace fetch
