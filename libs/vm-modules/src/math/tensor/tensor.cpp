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

#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/tensor/tensor_estimator.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/use_estimator.hpp"

#include <cstdint>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

using ArrayType  = fetch::math::Tensor<VMTensor::DataType>;
using SizeType   = ArrayType::SizeType;
using SizeVector = ArrayType::SizeVector;

VMTensor::VMTensor(VM *vm, TypeId type_id, std::vector<uint64_t> const &shape)
  : Object(vm, type_id)
  , tensor_(shape)
  , estimator_(*this)
{}

VMTensor::VMTensor(VM *vm, TypeId type_id, ArrayType tensor)
  : Object(vm, type_id)
  , tensor_(std::move(tensor))
  , estimator_(*this)
{}

VMTensor::VMTensor(VM *vm, TypeId type_id)
  : Object(vm, type_id)
  , estimator_(*this)
{}

Ptr<VMTensor> VMTensor::Constructor(VM *vm, TypeId type_id, Ptr<Array<SizeType>> const &shape)
{
  return Ptr<VMTensor>{new VMTensor(vm, type_id, shape->elements)};
}

void VMTensor::Bind(Module &module, bool const enable_experimental)
{
  using Index = fetch::math::SizeType;

  auto interface =
      module.CreateClassType<VMTensor>("Tensor")
          .CreateConstructor(&VMTensor::Constructor)
          .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMTensor> {
            return Ptr<VMTensor>{new VMTensor(vm, type_id)};
          })
          .CreateMemberFunction("at", &VMTensor::At<Index>, UseEstimator(&TensorEstimator::AtOne))
          .CreateMemberFunction("at", &VMTensor::At<Index, Index>,
                                UseEstimator(&TensorEstimator::AtTwo))
          .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index>,
                                UseEstimator(&TensorEstimator::AtThree))
          .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index, Index>,
                                UseEstimator(&TensorEstimator::AtFour))
          .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, DataType>,
                                UseEstimator(&TensorEstimator::SetAtOne))
          .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, DataType>,
                                UseEstimator(&TensorEstimator::SetAtTwo))
          .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, Index, DataType>,
                                UseEstimator(&TensorEstimator::SetAtThree))
          .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, Index, Index, DataType>,
                                UseEstimator(&TensorEstimator::SetAtFour))
          .CreateMemberFunction("size", &VMTensor::size, UseEstimator(&TensorEstimator::size))
          .CreateMemberFunction("fill", &VMTensor::Fill, UseEstimator(&TensorEstimator::Fill))
          .CreateMemberFunction("fillRandom", &VMTensor::FillRandom,
                                UseEstimator(&TensorEstimator::FillRandom))
          .CreateMemberFunction("min", &VMTensor::Min, UseEstimator(&TensorEstimator::Min))
          .CreateMemberFunction("max", &VMTensor::Max, UseEstimator(&TensorEstimator::Max))
          .CreateMemberFunction("reshape", &VMTensor::Reshape,
                                UseEstimator(&TensorEstimator::Reshape))
          .CreateMemberFunction("squeeze", &VMTensor::Squeeze,
                                UseEstimator(&TensorEstimator::Squeeze))
          .CreateMemberFunction("sum", &VMTensor::Sum, UseEstimator(&TensorEstimator::Sum))
          // TODO(ML-340) - rework operator bindings when it becomes possible to add estimators to
          // operators
          .CreateMemberFunction("negate", &VMTensor::NegateOperator,
                                UseEstimator(&TensorEstimator::NegateOperator))
          .CreateMemberFunction("isEqual", &VMTensor::IsEqualOperator,
                                UseEstimator(&TensorEstimator::EqualOperator))
          .CreateMemberFunction("isNotEqual", &VMTensor::IsNotEqualOperator,
                                UseEstimator(&TensorEstimator::NotEqualOperator))
          .CreateMemberFunction("add", &VMTensor::AddOperator,
                                UseEstimator(&TensorEstimator::AddOperator))
          .CreateMemberFunction("subtract", &VMTensor::SubtractOperator,
                                UseEstimator(&TensorEstimator::SubtractOperator))
          .CreateMemberFunction("multiply", &VMTensor::MultiplyOperator,
                                UseEstimator(&TensorEstimator::MultiplyOperator))
          .CreateMemberFunction("divide", &VMTensor::DivideOperator,
                                UseEstimator(&TensorEstimator::DivideOperator))
          //          .EnableOperator(Operator::Negate)
          //          .EnableOperator(Operator::Equal)
          //          .EnableOperator(Operator::NotEqual)
          //          .EnableOperator(Operator::Add)
          //          .EnableOperator(Operator::Subtract)
          //          .EnableOperator(Operator::InplaceAdd)
          //          .EnableOperator(Operator::InplaceSubtract)
          //          .EnableOperator(Operator::Multiply)
          //          .EnableOperator(Operator::Divide)
          //          .EnableOperator(Operator::InplaceMultiply)
          //          .EnableOperator(Operator::InplaceDivide)
          .CreateMemberFunction("transpose", &VMTensor::Transpose,
                                UseEstimator(&TensorEstimator::Transpose))
          .CreateMemberFunction("unsqueeze", &VMTensor::Unsqueeze,
                                UseEstimator(&TensorEstimator::Unsqueeze))
          .CreateMemberFunction("fromString", &VMTensor::FromString,
                                UseEstimator(&TensorEstimator::FromString))
          .CreateMemberFunction("toString", &VMTensor::ToString,
                                UseEstimator(&TensorEstimator::ToString));

  if (enable_experimental)
  {
    // no tensor features are experimental
    FETCH_UNUSED(interface);
  }

  // Add support for Array of Tensors
  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<VMTensor>>>();
}

SizeVector VMTensor::shape() const
{
  return tensor_.shape();
}

SizeType VMTensor::size() const
{
  return tensor_.size();
}

////////////////////////////////////
/// ACCESSING AND SETTING VALUES ///
////////////////////////////////////

template <typename... Indices>
VMTensor::DataType VMTensor::At(Indices... indices) const
{
  VMTensor::DataType result(0.0);
  try
  {
    result = tensor_.At(indices...);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
  return result;
}

template <typename... Args>
void VMTensor::SetAt(Args... args)
{
  try
  {
    tensor_.Set(args...);
  }
  catch (std::exception const &e)
  {
    RuntimeError(std::string(e.what()));
  }
}

void VMTensor::Copy(ArrayType const &other)
{
  tensor_.Copy(other);
}

void VMTensor::Fill(DataType const &value)
{
  tensor_.Fill(value);
}

void VMTensor::FillRandom()
{
  tensor_.FillUniformRandom();
}

Ptr<VMTensor> VMTensor::Squeeze()
{
  auto squeezed_tensor = tensor_.Copy();
  try
  {
    squeezed_tensor.Squeeze();
  }
  catch (std::exception const &e)
  {
    RuntimeError("Squeeze failed: " + std::string(e.what()));
  }
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, squeezed_tensor));
}

Ptr<VMTensor> VMTensor::Unsqueeze()
{
  auto unsqueezed_tensor = tensor_.Copy();
  unsqueezed_tensor.Unsqueeze();
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, unsqueezed_tensor));
}

bool VMTensor::Reshape(Ptr<Array<SizeType>> const &new_shape)
{
  return tensor_.Reshape(new_shape->elements);
}

void VMTensor::Transpose()
{
  tensor_.Transpose();
}

/////////////////////////
/// BASIC COMPARATOR  ///
/////////////////////////

bool VMTensor::IsEqualOperator(vm::Ptr<VMTensor> const &other)
{
  return (GetTensor() == other->GetTensor());
}

bool VMTensor::IsNotEqualOperator(vm::Ptr<VMTensor> const &other)
{
  return (GetTensor() != other->GetTensor());
}

Ptr<VMTensor> VMTensor::NegateOperator()
{
  Ptr<VMTensor> t = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Multiply(GetTensor(), DataType(-1), t->GetTensor());
  return t;
}

// TODO (ML-340) - Below Operators should be bound and above operators removed when operators can
// take estimators

bool VMTensor::IsEqual(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left   = lhso;
  Ptr<VMTensor> right  = rhso;
  bool          result = (left->GetTensor() == right->GetTensor());
  return result;
}

bool VMTensor::IsNotEqual(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left   = lhso;
  Ptr<VMTensor> right  = rhso;
  bool          result = (left->GetTensor() != right->GetTensor());
  return result;
}

void VMTensor::Negate(fetch::vm::Ptr<Object> &object)
{
  Ptr<VMTensor> operand = object;
  Ptr<VMTensor> t       = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Multiply(operand->GetTensor(), DataType(-1), t->GetTensor());
  object = std::move(t);
}

/////////////////////////
/// BASIC ARITHMETIC  ///
/////////////////////////

vm::Ptr<VMTensor> VMTensor::AddOperator(vm::Ptr<VMTensor> const &other)
{
  Ptr<VMTensor> t = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Add(GetTensor(), other->GetTensor(), t->GetTensor());
  return t;
}

vm::Ptr<VMTensor> VMTensor::SubtractOperator(vm::Ptr<VMTensor> const &other)
{
  Ptr<VMTensor> t = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Subtract(GetTensor(), other->GetTensor(), t->GetTensor());
  return t;
}

vm::Ptr<VMTensor> VMTensor::MultiplyOperator(vm::Ptr<VMTensor> const &other)
{
  Ptr<VMTensor> t = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Multiply(GetTensor(), other->GetTensor(), t->GetTensor());
  return t;
}

vm::Ptr<VMTensor> VMTensor::DivideOperator(vm::Ptr<VMTensor> const &other)
{
  Ptr<VMTensor> t = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Divide(GetTensor(), other->GetTensor(), t->GetTensor());
  return t;
}

// TODO (ML-340) - replace above arithmetic with these operator arithmetic after operators take
// estimators
void VMTensor::Add(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() + right->GetTensor());
}

void VMTensor::Subtract(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() - right->GetTensor());
}

void VMTensor::InplaceAdd(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineAdd(right->GetTensor());
}

void VMTensor::InplaceSubtract(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineSubtract(right->GetTensor());
}

void VMTensor::Multiply(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() * right->GetTensor());
}

void VMTensor::Divide(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() / right->GetTensor());
}

void VMTensor::InplaceMultiply(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineMultiply(right->GetTensor());
}

void VMTensor::InplaceDivide(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineDivide(right->GetTensor());
}

/////////////////////////
/// MATRIX OPERATIONS ///
/////////////////////////

DataType VMTensor::Min()
{
  return fetch::math::Min(tensor_);
}

DataType VMTensor::Max()
{
  return fetch::math::Max(tensor_);
}

DataType VMTensor::Sum()
{
  return fetch::math::Sum(tensor_);
}

//////////////////////////////
/// PRINTING AND EXPORTING ///
//////////////////////////////

void VMTensor::FromString(fetch::vm::Ptr<fetch::vm::String> const &string)
{
  try
  {
    tensor_.Assign(fetch::math::Tensor<DataType>::FromString(string->string()));
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
}

Ptr<String> VMTensor::ToString() const
{
  std::string as_string;
  try
  {
    as_string = tensor_.ToString();
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
  return Ptr<String>{new String(vm_, as_string)};
}

ArrayType &VMTensor::GetTensor()
{
  return tensor_;
}

ArrayType const &VMTensor::GetConstTensor()
{
  return tensor_;
}

bool VMTensor::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << tensor_;
  return true;
}

bool VMTensor::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> tensor_;
  return true;
}

TensorEstimator &VMTensor::Estimator()
{
  return estimator_;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
