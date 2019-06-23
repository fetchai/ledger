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

#include "math/fundamental_operators.hpp"
#include "math/tensor.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IMatrix : public Object
{
public:
  IMatrix()          = delete;
  virtual ~IMatrix() = default;
  static Ptr<IMatrix> Constructor(VM *vm, TypeId type_id, int32_t num_rows, int32_t num_columns);
  virtual TemplateParameter1 GetIndexedValue(AnyInteger const &row, AnyInteger const &column) = 0;
  virtual void               SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                                             TemplateParameter1 const &value)                 = 0;

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

  Matrix(VM *vm, TypeId type_id, TypeId element_type_id__, std::size_t num_rows,
         std::size_t num_columns)
    : IMatrix(vm, type_id)
    , matrix(std::vector<typename fetch::math::Tensor<T>::SizeType>(num_columns, num_rows))
  {
    element_type_id_ = element_type_id__;
  }

  static Ptr<Matrix> AcquireMatrix(VM *vm, TypeId type_id, TypeId element_type_id,
                                   std::size_t num_rows, std::size_t num_columns)
  {
    return Ptr<Matrix>(new Matrix(vm, type_id, element_type_id, num_rows, num_columns));
  }

  virtual void Negate(Ptr<Object> &object) override
  {
    bool const        matrix_is_modifiable = object.RefCount() == 1;
    Ptr<Matrix>       operand              = object;
    std::size_t const rows                 = operand->matrix.shape()[0];
    std::size_t const columns              = operand->matrix.shape()[1];

    // TODO(tfr): implement negate
    if (matrix_is_modifiable)
    {
      operand->matrix.InlineMultiply(T(-1));
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, rows, columns);
    fetch::math::Multiply(operand->matrix, T(-1), m->matrix);
    object = std::move(m);
  }

  virtual void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    bool const        lhs_matrix_is_modifiable = lhso.RefCount() == 1;
    bool const        rhs_matrix_is_modifiable = rhso.RefCount() == 1;
    Ptr<Matrix>       lhs                      = lhso;
    Ptr<Matrix>       rhs                      = rhso;
    std::size_t const lhs_rows                 = lhs->matrix.shape()[0];
    std::size_t const lhs_columns              = lhs->matrix.shape()[1];
    std::size_t const rhs_rows                 = rhs->matrix.shape()[0];
    std::size_t const rhs_columns              = rhs->matrix.shape()[1];
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
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Add(lhs->matrix, rhs->matrix, m->matrix);
    lhso = std::move(m);
  }

  virtual void RightAdd(Variant &objectv, Variant &rhsv) override
  {
    bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
    Ptr<Matrix>       lhs                      = objectv.object;
    T                 rhs                      = rhsv.primitive.Get<T>();
    std::size_t const lhs_rows                 = lhs->matrix.shape()[0];
    std::size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineAdd(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Add(lhs->matrix, rhs, m->matrix);
    objectv.Assign(std::move(m), objectv.type_id);
  }

  virtual void InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<Matrix>       lhs         = lhso;
    Ptr<Matrix>       rhs         = rhso;
    std::size_t const lhs_rows    = lhs->matrix.shape()[0];
    std::size_t const lhs_columns = lhs->matrix.shape()[1];
    std::size_t const rhs_rows    = rhs->matrix.shape()[0];
    std::size_t const rhs_columns = rhs->matrix.shape()[1];
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineAdd(rhs->matrix);
  }

  virtual void InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineAdd(rhs);
  }

  virtual void Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    bool const        lhs_matrix_is_modifiable = lhso.RefCount() == 1;
    bool const        rhs_matrix_is_modifiable = rhso.RefCount() == 1;
    Ptr<Matrix>       lhs                      = lhso;
    Ptr<Matrix>       rhs                      = rhso;
    std::size_t const lhs_rows                 = lhs->matrix.shape()[0];
    std::size_t const lhs_columns              = lhs->matrix.shape()[1];
    std::size_t const rhs_rows                 = rhs->matrix.shape()[0];
    std::size_t const rhs_columns              = rhs->matrix.shape()[1];
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
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Subtract(lhs->matrix, rhs->matrix, m->matrix);
    lhso = std::move(m);
  }

  virtual void RightSubtract(Variant &objectv, Variant &rhsv) override
  {
    bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
    Ptr<Matrix>       lhs                      = objectv.object;
    T                 rhs                      = rhsv.primitive.Get<T>();
    std::size_t const lhs_rows                 = lhs->matrix.shape()[0];
    std::size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineSubtract(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Subtract(lhs->matrix, rhs, m->matrix);
    objectv.Assign(std::move(m), objectv.type_id);
  }

  virtual void InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<Matrix>       lhs         = lhso;
    Ptr<Matrix>       rhs         = rhso;
    std::size_t const lhs_rows    = lhs->matrix.shape()[0];
    std::size_t const lhs_columns = lhs->matrix.shape()[1];
    std::size_t const rhs_rows    = rhs->matrix.shape()[0];
    std::size_t const rhs_columns = rhs->matrix.shape()[1];
    if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
    {
      RuntimeError("invalid operation");
      return;
    }
    lhs->matrix.InlineSubtract(rhs->matrix);
  }

  virtual void InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineSubtract(rhs);
  }

  virtual void Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<Matrix>       lhs         = lhso;
    Ptr<Matrix>       rhs         = rhso;
    std::size_t const lhs_rows    = lhs->matrix.shape()[0];
    std::size_t const lhs_columns = lhs->matrix.shape()[1];
    std::size_t const rhs_rows    = rhs->matrix.shape()[0];
    std::size_t const rhs_columns = rhs->matrix.shape()[1];
    if (lhs_columns != rhs_rows)
    {
      RuntimeError("invalid operation");
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, rhs_columns);
    // TODO(tfr): use blas
    TODO_FAIL("Use BLAS TODO");
    lhso = std::move(m);
  }

  virtual void LeftMultiply(Variant &lhsv, Variant &objectv) override
  {
    bool const        rhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
    T                 lhs                      = lhsv.primitive.Get<T>();
    Ptr<Matrix>       rhs                      = objectv.object;
    std::size_t const rhs_rows                 = rhs->matrix.shape()[0];
    std::size_t const rhs_columns              = rhs->matrix.shape()[1];
    if (rhs_matrix_is_modifiable)
    {
      rhs->matrix.InlineMultiply(lhs);
      lhsv = std::move(objectv);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, rhs_rows, rhs_columns);
    fetch::math::Multiply(rhs->matrix, lhs, m->matrix);
    lhsv.Assign(std::move(m), objectv.type_id);
  }

  virtual void RightMultiply(Variant &objectv, Variant &rhsv) override
  {
    bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
    Ptr<Matrix>       lhs                      = objectv.object;
    T                 rhs                      = rhsv.primitive.Get<T>();
    std::size_t const lhs_rows                 = lhs->matrix.shape()[0];
    std::size_t const lhs_columns              = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineMultiply(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Multiply(lhs->matrix, rhs, m->matrix);
    objectv.Assign(std::move(m), objectv.type_id);
  }

  virtual void InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv) override
  {
    Ptr<Matrix> lhs = lhso;
    T           rhs = rhsv.primitive.Get<T>();
    lhs->matrix.InlineMultiply(rhs);
  }

  virtual void RightDivide(Variant &objectv, Variant &rhsv) override
  {
    bool const  lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
    Ptr<Matrix> lhs                      = objectv.object;
    T           rhs                      = rhsv.primitive.Get<T>();
    if (math::IsZero(rhs))
    {
      RuntimeError("division by zero");
      return;
    }
    std::size_t const lhs_rows    = lhs->matrix.shape()[0];
    std::size_t const lhs_columns = lhs->matrix.shape()[1];
    if (lhs_matrix_is_modifiable)
    {
      lhs->matrix.InlineDivide(rhs);
      return;
    }
    Ptr<Matrix> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
    fetch::math::Divide(lhs->matrix, rhs, m->matrix);
    objectv.Assign(std::move(m), objectv.type_id);
  }

  virtual void InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv) override
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

  T *Find(AnyInteger const &row, AnyInteger const &column)
  {
    std::size_t r;
    std::size_t c;
    if (!GetNonNegativeInteger(column, c))
    {
      RuntimeError("negative index");
      return nullptr;
    }
    if (!GetNonNegativeInteger(row, r))
    {
      RuntimeError("negative index");
      return nullptr;
    }
    std::size_t const num_rows    = matrix.shape()[0];
    std::size_t const num_columns = matrix.shape()[1];
    if ((r >= num_rows) || (c >= num_columns))
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    return &matrix.At(c, r);
  }

  virtual TemplateParameter1 GetIndexedValue(AnyInteger const &row,
                                             AnyInteger const &column) override
  {
    T *ptr = Find(row, column);
    if (ptr)
    {
      return TemplateParameter1(*ptr, element_type_id_);
    }
    // Not found
    return TemplateParameter1();
  }

  virtual void SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                               TemplateParameter1 const &value) override
  {
    T *ptr = Find(row, column);
    if (ptr)
    {
      *ptr = value.Get<T>();
    }
  }

  fetch::math::Tensor<T> matrix;
  TypeId                 element_type_id_;
};

inline Ptr<IMatrix> IMatrix::Constructor(VM *vm, TypeId type_id, int32_t num_rows,
                                         int32_t num_columns)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.parameter_type_ids[0];
  if ((num_rows < 0) || (num_columns < 0))
  {
    vm->RuntimeError("negative size");
    return Ptr<IMatrix>();
  }
  if (element_type_id == TypeIds::Float32)
  {
    return Ptr<IMatrix>(new Matrix<float>(vm, type_id, element_type_id, std::size_t(num_rows),
                                          std::size_t(num_columns)));
  }
  else
  {
    return Ptr<IMatrix>(new Matrix<double>(vm, type_id, element_type_id, std::size_t(num_rows),
                                           std::size_t(num_columns)));
  }
}

}  // namespace vm
}  // namespace fetch
