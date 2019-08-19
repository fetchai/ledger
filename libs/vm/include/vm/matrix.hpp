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

#include "math/tensor.hpp"
#include "vm/object.hpp"
#include "vm/variant.hpp"

namespace fetch {
namespace vm {

class IMatrix : public Object
{
public:
  IMatrix()           = delete;
  ~IMatrix() override = default;

  static Ptr<IMatrix> Constructor(VM *vm, TypeId type_id, int32_t num_rows, int32_t num_columns);
  virtual TemplateParameter1 GetIndexedValue(AnyInteger const &row, AnyInteger const &column) = 0;
  virtual void               SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                                             TemplateParameter1 const &value)                 = 0;

protected:
  IMatrix(VM *vm, TypeId type_id);
};

template <typename T>
struct Matrix : public IMatrix
{
  Matrix()           = delete;
  ~Matrix() override = default;

  Matrix(VM *vm, TypeId type_id, TypeId element_type_id__, std::size_t num_rows,
         std::size_t num_columns);

  static Ptr<Matrix> AcquireMatrix(VM *vm, TypeId type_id, TypeId element_type_id,
                                   std::size_t num_rows, std::size_t num_columns);

  void Negate(Ptr<Object> &object) override;

  void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void RightAdd(Variant &objectv, Variant &rhsv) override;

  void InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;

  void InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void RightSubtract(Variant &objectv, Variant &rhsv) override;

  void InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;

  void InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void LeftMultiply(Variant &lhsv, Variant &objectv) override;

  void RightMultiply(Variant &objectv, Variant &rhsv) override;

  void InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void RightDivide(Variant &objectv, Variant &rhsv) override;

  void InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv) override;

  T *Find(AnyInteger const &row, AnyInteger const &column);

  TemplateParameter1 GetIndexedValue(AnyInteger const &row, AnyInteger const &column) override;

  void SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                       TemplateParameter1 const &value) override;

  fetch::math::Tensor<T> matrix;
  TypeId                 element_type_id_;
};

}  // namespace vm
}  // namespace fetch
