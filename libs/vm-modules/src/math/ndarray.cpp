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

#include "vm_modules/math/ndarray.hpp"

#include "math/fundamental_operators.hpp"
#include "math/tensor.hpp"
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm_modules {
namespace math {

ITensor::ITensor(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

template <typename T>
NDArray<T>::NDArray(VM *vm, TypeId type_id, TypeId element_type_id__, std::size_t num_rows,
                    std::size_t num_columns)
  : ITensor(vm, type_id)
  , tensor_(std::vector<typename fetch::math::Tensor<T>::SizeType>(num_columns, num_rows))
{
  element_type_id_ = element_type_id__;
}

template <typename T>
NDArray<T>::NDArray(VM *vm, TypeId type_id, std::vector<fetch::math::SizeType> const &shape)
  : ITensor(vm, type_id)
  , tensor_(shape)
{}

template <typename T>
NDArray<T>::NDArray(VM *vm, TypeId type_id, fetch::math::Tensor<T> tensor)
  : ITensor(vm, type_id)
  , tensor_(std::move(tensor))
{}

template <typename T>
NDArray<T>::NDArray(VM *vm, TypeId type_id)
  : ITensor(vm, type_id)
{}

// template <typename T>
// void NDArray<T>::Bind(Module &module)
//{
//  ITensor::Bind(module);
//  using Index = fetch::math::SizeType;
//  module.CreateMemberFunction("at", &NDArray<T>::At<Index>)
//      .CreateMemberFunction("at", &NDArray::At<Index, Index>)
//      .CreateMemberFunction("at", &NDArray::At<Index, Index, Index>)
//      .CreateMemberFunction("at", &NDArray::At<Index, Index, Index, Index>)
//      .CreateMemberFunction("at", &NDArray::At<Index, Index, Index, Index, Index>)
//      .CreateMemberFunction("at", &NDArray::At<Index, Index, Index, Index, Index, Index>)
//      .CreateMemberFunction("setAt", &NDArray::SetAt<Index, DataType>)
//      .CreateMemberFunction("setAt", &NDArray::SetAt<Index, Index, DataType>)
//      .CreateMemberFunction("setAt", &NDArray::SetAt<Index, Index, Index, DataType>)
//      .CreateMemberFunction("setAt", &NDArray::SetAt<Index, Index, Index, Index, DataType>)
//      .CreateMemberFunction("setAt", &NDArray::SetAt<Index, Index, Index, Index, Index, DataType>)
//      .CreateMemberFunction("setAt",
//                            &NDArray::SetAt<Index, Index, Index, Index, Index, Index, DataType>)
//}

template <typename T>
fetch::vm::Ptr<NDArray<T>> NDArray<T>::Constructor(
    VM *vm, TypeId type_id, const fetch::vm::Ptr<fetch::vm::Array<fetch::math::SizeType>> &shape)
{
  using namespace fetch::vm;
  return Ptr<NDArray<T>>{new NDArray<T>(vm, type_id, shape->elements)};
}

template <typename T>
Ptr<NDArray<T>> NDArray<T>::AcquireMatrix(VM *vm, TypeId type_id, TypeId element_type_id,
                                          std::size_t num_rows, std::size_t num_columns)
{
  return Ptr<NDArray<T>>{new NDArray<T>(vm, type_id, element_type_id, num_rows, num_columns)};
}

template <typename T>
void NDArray<T>::Negate(Ptr<Object> &object)
{
  bool const        matrix_is_modifiable = object.RefCount() == 1;
  Ptr<NDArray>      operand              = object;
  std::size_t const rows                 = operand->tensor_.shape()[0];
  std::size_t const columns              = operand->tensor_.shape()[1];

  // TODO(tfr): implement negate
  if (matrix_is_modifiable)
  {
    operand->tensor_.InlineMultiply(T(-1));
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, rows, columns);
  fetch::math::Multiply(operand->tensor_, T(-1), m->tensor_);
  object = std::move(m);
}

template <typename T>
void NDArray<T>::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  bool const        lhs_matrix_is_modifiable = lhso.RefCount() == 1;
  bool const        rhs_matrix_is_modifiable = rhso.RefCount() == 1;
  Ptr<NDArray>      lhs                      = lhso;
  Ptr<NDArray>      rhs                      = rhso;
  std::size_t const lhs_rows                 = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns              = lhs->tensor_.shape()[1];
  std::size_t const rhs_rows                 = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns              = rhs->tensor_.shape()[1];
  if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
  {
    RuntimeError("invalid operation");
    return;
  }
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineAdd(rhs->tensor_);
    return;
  }
  if (rhs_matrix_is_modifiable)
  {
    rhs->tensor_.InlineAdd(lhs->tensor_);
    lhso = std::move(rhs);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Add(lhs->tensor_, rhs->tensor_, m->tensor_);
  lhso = std::move(m);
}

template <typename T>
void NDArray<T>::RightAdd(Variant &objectv, Variant &rhsv)
{
  bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
  Ptr<NDArray>      lhs                      = objectv.object;
  T                 rhs                      = rhsv.primitive.Get<T>();
  std::size_t const lhs_rows                 = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns              = lhs->tensor_.shape()[1];
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineAdd(rhs);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Add(lhs->tensor_, rhs, m->tensor_);
  objectv.Assign(std::move(m), objectv.type_id);
}

template <typename T>
void NDArray<T>::InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<NDArray>      lhs         = lhso;
  Ptr<NDArray>      rhs         = rhso;
  std::size_t const lhs_rows    = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns = lhs->tensor_.shape()[1];
  std::size_t const rhs_rows    = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns = rhs->tensor_.shape()[1];
  if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
  {
    RuntimeError("invalid operation");
    return;
  }
  lhs->tensor_.InlineAdd(rhs->tensor_);
}

template <typename T>
void NDArray<T>::InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv)
{
  Ptr<NDArray> lhs = lhso;
  T            rhs = rhsv.primitive.Get<T>();
  lhs->tensor_.InlineAdd(rhs);
}

template <typename T>
void NDArray<T>::Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  bool const        lhs_matrix_is_modifiable = lhso.RefCount() == 1;
  bool const        rhs_matrix_is_modifiable = rhso.RefCount() == 1;
  Ptr<NDArray>      lhs                      = lhso;
  Ptr<NDArray>      rhs                      = rhso;
  std::size_t const lhs_rows                 = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns              = lhs->tensor_.shape()[1];
  std::size_t const rhs_rows                 = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns              = rhs->tensor_.shape()[1];
  if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
  {
    RuntimeError("invalid operation");
    return;
  }
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineSubtract(rhs->tensor_);
    return;
  }
  if (rhs_matrix_is_modifiable)
  {
    rhs->tensor_.InlineReverseSubtract(lhs->tensor_);
    lhso = std::move(rhs);
    return;
  }
  Ptr<NDArray<T>> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Subtract(lhs->tensor_, rhs->tensor_, m->tensor_);
  lhso = std::move(m);
}

template <typename T>
void NDArray<T>::RightSubtract(Variant &objectv, Variant &rhsv)
{
  bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
  Ptr<NDArray>      lhs                      = objectv.object;
  T                 rhs                      = rhsv.primitive.Get<T>();
  std::size_t const lhs_rows                 = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns              = lhs->tensor_.shape()[1];
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineSubtract(rhs);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Subtract(lhs->tensor_, rhs, m->tensor_);
  objectv.Assign(std::move(m), objectv.type_id);
}

template <typename T>
void NDArray<T>::InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<NDArray>      lhs         = lhso;
  Ptr<NDArray>      rhs         = rhso;
  std::size_t const lhs_rows    = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns = lhs->tensor_.shape()[1];
  std::size_t const rhs_rows    = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns = rhs->tensor_.shape()[1];
  if ((lhs_rows != rhs_rows) || (lhs_columns != rhs_columns))
  {
    RuntimeError("invalid operation");
    return;
  }
  lhs->tensor_.InlineSubtract(rhs->tensor_);
}

template <typename T>
void NDArray<T>::InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv)
{
  Ptr<NDArray> lhs = lhso;
  T            rhs = rhsv.primitive.Get<T>();
  lhs->tensor_.InlineSubtract(rhs);
}

template <typename T>
void NDArray<T>::Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<NDArray>      lhs         = lhso;
  Ptr<NDArray>      rhs         = rhso;
  std::size_t const lhs_rows    = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns = lhs->tensor_.shape()[1];
  std::size_t const rhs_rows    = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns = rhs->tensor_.shape()[1];
  if (lhs_columns != rhs_rows)
  {
    RuntimeError("invalid operation");
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, rhs_columns);
  // TODO(tfr): use blas
  TODO_FAIL("Use BLAS TODO");
  lhso = std::move(m);
}

template <typename T>
void NDArray<T>::LeftMultiply(Variant &lhsv, Variant &objectv)
{
  bool const        rhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
  T                 lhs                      = lhsv.primitive.Get<T>();
  Ptr<NDArray>      rhs                      = objectv.object;
  std::size_t const rhs_rows                 = rhs->tensor_.shape()[0];
  std::size_t const rhs_columns              = rhs->tensor_.shape()[1];
  if (rhs_matrix_is_modifiable)
  {
    rhs->tensor_.InlineMultiply(lhs);
    lhsv = std::move(objectv);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, rhs_rows, rhs_columns);
  fetch::math::Multiply(rhs->tensor_, lhs, m->tensor_);
  lhsv.Assign(std::move(m), objectv.type_id);
}

template <typename T>
void NDArray<T>::RightMultiply(Variant &objectv, Variant &rhsv)
{
  bool const        lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
  Ptr<NDArray>      lhs                      = objectv.object;
  T                 rhs                      = rhsv.primitive.Get<T>();
  std::size_t const lhs_rows                 = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns              = lhs->tensor_.shape()[1];
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineMultiply(rhs);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Multiply(lhs->tensor_, rhs, m->tensor_);
  objectv.Assign(std::move(m), objectv.type_id);
}

template <typename T>
void NDArray<T>::InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv)
{
  Ptr<NDArray> lhs = lhso;
  T            rhs = rhsv.primitive.Get<T>();
  lhs->tensor_.InlineMultiply(rhs);
}

template <typename T>
void NDArray<T>::RightDivide(Variant &objectv, Variant &rhsv)
{
  bool const   lhs_matrix_is_modifiable = objectv.object.RefCount() == 1;
  Ptr<NDArray> lhs                      = objectv.object;
  T            rhs                      = rhsv.primitive.Get<T>();
  if (fetch::math::IsZero(rhs))
  {
    RuntimeError("division by zero");
    return;
  }
  std::size_t const lhs_rows    = lhs->tensor_.shape()[0];
  std::size_t const lhs_columns = lhs->tensor_.shape()[1];
  if (lhs_matrix_is_modifiable)
  {
    lhs->tensor_.InlineDivide(rhs);
    return;
  }
  Ptr<NDArray> m = AcquireMatrix(vm_, type_id_, element_type_id_, lhs_rows, lhs_columns);
  fetch::math::Divide(lhs->tensor_, rhs, m->tensor_);
  objectv.Assign(std::move(m), objectv.type_id);
}

template <typename T>
void NDArray<T>::InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv)
{
  Ptr<NDArray> lhs = lhso;
  T            rhs = rhsv.primitive.Get<T>();
  if (fetch::math::IsNonZero(rhs))
  {
    lhs->tensor_.InlineDivide(rhs);
    return;
  }
  RuntimeError("division by zero");
}

template <typename T>
T *NDArray<T>::Find(AnyInteger const &row, AnyInteger const &column)
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
  std::size_t const num_rows    = tensor_.shape()[0];
  std::size_t const num_columns = tensor_.shape()[1];
  if ((r >= num_rows) || (c >= num_columns))
  {
    RuntimeError("index out of bounds");
    return nullptr;
  }
  return &tensor_.At(c, r);
}

template <typename T>
TemplateParameter1 NDArray<T>::At(Index idx1) const
{
  if (tensor_.shape().size() != 1)
  {
    vm_->RuntimeError("Wrong 1-dimensional accessor called on tensor with dimensions : " +
                      std::to_string(tensor_.shape().size()));
    return TemplateParameter1();
  }
  T const value = tensor_.At(idx1);
  return TemplateParameter1(value, element_type_id_);
}

template <typename T>
TemplateParameter1 NDArray<T>::At(Index idx1, Index idx2) const
{
  if (tensor_.shape().size() != 2)
  {
    vm_->RuntimeError("Wrong 2-dimensional accessor called on tensor with dimensions : " +
                      std::to_string(tensor_.shape().size()));
    return TemplateParameter1();
  }

  T const value = tensor_.At(idx1, idx2);
  return TemplateParameter1(value, element_type_id_);
}
template <typename T>
TemplateParameter1 NDArray<T>::At(Index idx1, Index idx2, Index idx3) const
{
  if (tensor_.shape().size() != 3)
  {
    vm_->RuntimeError("Wrong 3-dimensional accessor called on tensor with dimensions : " +
                      std::to_string(tensor_.shape().size()));
    return TemplateParameter1();
  }

  T const value = tensor_.At(idx1, idx2, idx3);
  return TemplateParameter1(value, element_type_id_);
}

template <typename T>
TemplateParameter1 NDArray<T>::At(Index idx1, Index idx2, Index idx3, Index idx4) const
{
  if (tensor_.shape().size() != 4)
  {
    vm_->RuntimeError("Wrong 4-dimensional accessor called on tensor with dimensions : " +
                      std::to_string(tensor_.shape().size()));
    return TemplateParameter1();
  }

  T const value = tensor_.At(idx1, idx2, idx3, idx4);
  return TemplateParameter1(value, element_type_id_);
}

template <typename T>
void NDArray<T>::SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                                 TemplateParameter1 const &value)
{
  T *ptr = Find(row, column);
  if (ptr)
  {
    *ptr = value.Get<T>();
  }
}

template <typename T>
TemplateParameter1 NDArray<T>::GetIndexedValue(AnyInteger const &row, AnyInteger const &column)
{
  T *ptr = Find(row, column);
  if (ptr)
  {
    return TemplateParameter1(*ptr, element_type_id_);
  }
  // Not found
  return TemplateParameter1();
}

template <typename T>
void NDArray<T>::Fill(const TemplateParameter1 &value)
{
  // TODO: is it safe enough if a float64 value is given to a Tensor of type Fixed32?...
  T const val = value.Get<T>();
  tensor_.Fill(val);
}

template <typename T>
fetch::vm::Ptr<ITensor> NDArray<T>::Squeeze() const
{
  auto squeezed_tensor = tensor_.Copy();
  squeezed_tensor.Squeeze();
  return fetch::vm::Ptr<ITensor>(new NDArray<T>(vm_, type_id_, squeezed_tensor));
}

template <typename T>
fetch::vm::Ptr<ITensor> NDArray<T>::Unsqueeze() const
{
  auto unsqueezed_tensor = tensor_.Copy();
  unsqueezed_tensor.Unsqueeze();
  return fetch::vm::Ptr<ITensor>(new NDArray<T>(vm_, type_id_, unsqueezed_tensor));
}

template <typename T>
fetch::math::SizeVector NDArray<T>::shape() const
{
  return tensor_.shape();
}

// template <typename T>
// fetch::math::Tensor<T> const &NDArray<T>::GetTensor()
//{
//  return tensor_;
//}

// Ptr<ITensor> ITensor::Constructor(VM *vm, TypeId type_id,  // int32_t num_rows, int32_t
// num_columns)
//                                  int32_t num_rows, int32_t num_columns)
//{
//  TypeInfo const &     type_info        = vm->GetTypeInfo(type_id);
//  TypeId const         element_type_id  = type_info.template_parameter_type_ids[0];
//  static constexpr int min_allowed_size = 1;
//  for (auto const &axis : {num_rows, num_columns})
//  {
//    if (axis < min_allowed_size)
//    {
//      vm->RuntimeError("Can not construct a Tensor with axis size < 1!");
//      return Ptr<ITensor>();
//    }
//  }
//  if (element_type_id == TypeIds::Float32)
//  {
//    return Ptr<ITensor>{new NDArray<float>(vm, type_id, element_type_id, std::size_t(num_rows),
//                                           std::size_t(num_columns))};
//  }

//  return Ptr<ITensor>{new NDArray<double>(vm, type_id, element_type_id, std::size_t(num_rows),
//                                          std::size_t(num_columns))};
//}

Ptr<ITensor> ITensor::Constructor(
    VM *vm, TypeId type_id, const fetch::vm::Ptr<fetch::vm::Array<fetch::math::SizeType>> &shape)
{
  static constexpr int min_allowed_size = 1;
  for (auto const &axis : shape->elements)
  {
    if (axis < min_allowed_size)
    {
      vm->RuntimeError("Can not construct NDArray with axis size < 1!");
      return Ptr<ITensor>();
    }
  }
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.template_parameter_type_ids[0];

  static std::set<TypeId> allowed_types{TypeIds::Float32, TypeIds::Float64, TypeIds::Fixed32,
                                        TypeIds::Fixed64};
  if (allowed_types.find(element_type_id) == allowed_types.end())
  {
    vm->RuntimeError("Can not create NDArray with element TypeId " +
                     std::to_string(element_type_id));
    return Ptr<ITensor>();
  }

  switch (element_type_id)
  {
  case TypeIds::Float64:
    return Ptr<ITensor>{new NDArray<double>(vm, type_id, shape->elements)};
  case TypeIds::Float32:
    return Ptr<ITensor>{new NDArray<float>(vm, type_id, shape->elements)};
  case TypeIds::Fixed32:
    return Ptr<ITensor>{new NDArray<fixed_point::fp32_t>(vm, type_id, shape->elements)};
  case TypeIds::Fixed64:
    return Ptr<ITensor>{new NDArray<fixed_point::fp64_t>(vm, type_id, shape->elements)};
  }

  // TEMPORARY DUMMY, SHOULD BE REWORKED
  vm->RuntimeError("Can not create NDArray with element TypeId " + std::to_string(element_type_id));
  return Ptr<ITensor>();
}

void fetch::vm_modules::math::ITensor::Bind(fetch::vm::Module &module)
{
  auto at4 =
      static_cast<TemplateParameter1 (ITensor::*)(Index, Index, Index, Index) const>(&ITensor::At);
  auto at3 = static_cast<TemplateParameter1 (ITensor::*)(Index, Index, Index) const>(&ITensor::At);
  auto at2 = static_cast<TemplateParameter1 (ITensor::*)(Index, Index) const>(&ITensor::At);
  auto at1 = static_cast<TemplateParameter1 (ITensor::*)(Index) const>(&ITensor::At);
  module.CreateTemplateType<ITensor, AnyPrimitive>("NDArray")
      .CreateConstructor(&ITensor::Constructor)
      .EnableIndexOperator(&ITensor::GetIndexedValue, &ITensor::SetIndexedValue)
      .CreateMemberFunction("at", at4)
      .CreateMemberFunction("at", at3)
      .CreateMemberFunction("at", at2)
      .CreateMemberFunction("at", at1)
      .CreateMemberFunction("squeeze", &ITensor::Squeeze)
      .CreateMemberFunction("unsqueeze", &ITensor::Unsqueeze)
      .CreateMemberFunction("fill", &ITensor::Fill)
      .CreateInstantiationType<NDArray<float>>()
      .CreateInstantiationType<NDArray<double>>()
      .CreateInstantiationType<NDArray<fixed_point::fp32_t>>()
      .CreateInstantiationType<NDArray<fixed_point::fp64_t>>()
      .EnableOperator(Operator::Negate)
      .EnableOperator(Operator::Add)
      .EnableOperator(Operator::Subtract)
      .EnableOperator(Operator::Multiply)
      .EnableOperator(Operator::InplaceAdd)
      .EnableOperator(Operator::InplaceSubtract)
      .EnableLeftOperator(Operator::Multiply)
      .EnableRightOperator(Operator::Add)
      .EnableRightOperator(Operator::Subtract)
      .EnableRightOperator(Operator::Multiply)
      .EnableRightOperator(Operator::Divide)
      .EnableRightOperator(Operator::InplaceAdd)
      .EnableRightOperator(Operator::InplaceSubtract)
      .EnableRightOperator(Operator::InplaceMultiply)
      .EnableRightOperator(Operator::InplaceDivide);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
