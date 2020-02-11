//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/tensor/tensor.hpp"

#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/pair.hpp"
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

VMTensor::VMTensor(VM *vm, TypeId type_id, std::string const &str)
  : Object(vm, type_id)
  , estimator_(*this)
{
  try
  {
    tensor_ = TensorType::FromString(str);
  }
  catch (std::exception const &ex)
  {
    // TODO(issue 1094): Support for nested runtime error(s) and/or exception(s)
    vm_->RuntimeError("An exception has been thrown from State<...>::FlushIO(). Desc.: " +
                      std::string(ex.what()));
  }
  catch (...)
  {
    // TODO(issue 1094): Support for nested runtime error(s) and/or exception(s)
    vm_->RuntimeError("An exception has been thrown from State<...>::FlushIO().");
  }
}

Ptr<VMTensor> VMTensor::Constructor(VM *vm, TypeId type_id, Ptr<Array<SizeType>> const &shape)
{
  for (SizeType axis_size : shape->elements)
  {
    if (axis_size == 0)
    {
      vm->RuntimeError("Can not create a Tensor : axis of size 0 found in new shape!");
      return Ptr<VMTensor>{new VMTensor(vm, type_id)};
    }
  }
  return Ptr<VMTensor>{new VMTensor(vm, type_id, shape->elements)};
}

Ptr<VMTensor> VMTensor::StringConstructor(VM *vm, TypeId type_id, Ptr<String> const &str)
{
  return Ptr<VMTensor>{new VMTensor(vm, type_id, str->string())};
}

Ptr<VMTensor> VMTensor::EmptyConstructor(VM *vm, TypeId type_id)
{
  return Ptr<VMTensor>{new VMTensor(vm, type_id)};
}

void VMTensor::Bind(Module &module, bool const enable_experimental)
{
  using Index = fetch::math::SizeType;

  auto const tensor_constructor_charge_estimate =
      [](Ptr<Array<SizeType>> const &shape) -> ChargeAmount {
    DataType padded_size =
        static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(shape->elements));

    return static_cast<ChargeAmount>(CONSTRUCTION_PADDED_SIZE_COEF * padded_size +
                                     CONSTRUCTION_CONST_COEF) *
           COMPUTE_CHARGE_COST;
  };

  auto const tensor_string_constructor_charge_estimate =
      [](Ptr<String> const &str) -> ChargeAmount {
    auto size = static_cast<DataType>(str->string().size());

    return static_cast<ChargeAmount>(CONSTRUCTION_STRING_SIZE_COEF * size +
                                     CONSTRUCTION_STRING_CONST_COEF) *
           COMPUTE_CHARGE_COST;
  };

  // Non-experimental features
  auto interface =
      module.CreateClassType<VMTensor>("Tensor")
          .CreateConstructor(&VMTensor::Constructor, tensor_constructor_charge_estimate)
          .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMTensor> {
            return Ptr<VMTensor>{new VMTensor(vm, type_id)};
          })
          .CreateMemberFunction("copy", &VMTensor::Copy, UseEstimator(&TensorEstimator::Copy))
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
          .CreateMemberFunction("shape", &VMTensor::VMShape,
                                UseEstimator(&TensorEstimator::VMShape))
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
          .CreateMemberFunction("argMax", &VMTensor::ArgMax, UseEstimator(&TensorEstimator::ArgMax))
          .CreateMemberFunction("argMax", &VMTensor::ArgMaxNoIndices,
                                UseEstimator(&TensorEstimator::ArgMaxNoIndices))
          .CreateMemberFunction("dot", &VMTensor::Dot, UseEstimator(&TensorEstimator::Dot))
          .EnableOperator(Operator::Negate)
          .EnableOperator(Operator::Equal)
          .EnableOperator(Operator::NotEqual)
          .EnableOperator(Operator::Add)
          .EnableOperator(Operator::Subtract)
          .EnableOperator(Operator::InplaceAdd)
          .EnableOperator(Operator::InplaceSubtract)
          .EnableOperator(Operator::Multiply)
          .EnableOperator(Operator::Divide)
          .EnableOperator(Operator::InplaceMultiply)
          .EnableOperator(Operator::InplaceDivide)
          .CreateMemberFunction("transpose", &VMTensor::Transpose,
                                UseEstimator(&TensorEstimator::Transpose))
          .CreateMemberFunction("unsqueeze", &VMTensor::Unsqueeze,
                                UseEstimator(&TensorEstimator::Unsqueeze))
          .CreateMemberFunction("fromString", &VMTensor::FromString,
                                UseEstimator(&TensorEstimator::FromString))
          .CreateMemberFunction("toString", &VMTensor::ToString,
                                UseEstimator(&TensorEstimator::ToString));

  // experimental features are bound only if the VMFactory given the flag to do so
  if (enable_experimental)
  {
    interface.CreateConstructor(&VMTensor::StringConstructor,
                                tensor_string_constructor_charge_estimate);
    interface.CreateConstructor(&VMTensor::EmptyConstructor, []() -> ChargeAmount {
      return static_cast<ChargeAmount>(CONSTRUCTION_CONST_COEF);
    });
  }

  // Add support for Array of Tensors
  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<VMTensor>>>();

  // Add support for training pair
  module.GetClassInterface<IPair>()
      .CreateInstantiationType<Pair<Ptr<VMTensor>, Ptr<Array<Ptr<VMTensor>>>>>();
}

SizeVector VMTensor::shape() const
{
  return tensor_.shape();
}

SizeType VMTensor::size() const
{
  return tensor_.size();
}

vm::Ptr<vm::Array<SizeType>> VMTensor::VMShape() const
{
  auto array = this->vm_->CreateNewObject<Array<SizeType>>(
      this->vm_->GetTypeId<SizeType>(), static_cast<int32_t>(tensor_.shape().size()));

  for (std::size_t i = 0; i < tensor_.shape().size(); ++i)
  {
    array->elements.at(i) = static_cast<SizeType>(tensor_.shape().at(i));
  }

  return array;
}

////////////////////////////////////
/// ACCESSING AND SETTING VALUES ///
////////////////////////////////////

template <typename... Indices>
VMTensor::DataType VMTensor::At(Indices... indices) const
{
  VMTensor::DataType result{0};
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

vm::Ptr<VMTensor> VMTensor::Copy()
{
  Ptr<VMTensor> ret = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  (ret->GetTensor()).Copy(GetTensor());
  return ret;
}

void VMTensor::Fill(DataType const &value)
{
  tensor_.Fill(value);
}

void VMTensor::FillRandom()
{
  tensor_.FillUniformRandom();
}

Ptr<VMTensor> VMTensor::Squeeze() const
{
  auto squeezed_tensor = tensor_.Copy();
  try
  {
    squeezed_tensor.Squeeze();
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Squeeze failed: " + std::string(e.what()));
  }
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, squeezed_tensor));
}

Ptr<VMTensor> VMTensor::Unsqueeze() const
{
  auto unsqueezed_tensor = tensor_.Copy();
  unsqueezed_tensor.Unsqueeze();
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, unsqueezed_tensor));
}

bool VMTensor::Reshape(Ptr<Array<SizeType>> const &new_shape)
{
  if (new_shape->elements.empty())
  {
    RuntimeError("Can not reshape a Tensor : new shape is empty!");
    return false;
  }
  std::size_t total_new_elements = 1;
  for (SizeType axis_size : new_shape->elements)
  {
    if (axis_size == 0)
    {
      RuntimeError("Can not reshape a Tensor : axis of size 0 found in new shape!");
      return false;
    }
    total_new_elements *= axis_size;
  }
  if (total_new_elements != tensor_.size())
  {
    RuntimeError("Can not reshape a Tensor : total elements count in the new shape (" +
                 std::to_string(total_new_elements) +
                 ") mismatch. Expected : " + std::to_string(tensor_.size()));
    return false;
  }
  return tensor_.Reshape(new_shape->elements);
}

Ptr<VMTensor> VMTensor::Transpose() const
{
  if (tensor_.shape().size() != RECTANGULAR_SHAPE_SIZE)
  {
    vm_->RuntimeError("Can not transpose a Tensor which is not 2-dimensional!");
    return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, tensor_.Copy()));
  }
  auto transposed = tensor_.Transpose();
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, transposed));
}

/////////////////////////
/// BASIC COMPARATOR  ///
/////////////////////////

bool VMTensor::IsEqual(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left   = lhso;
  Ptr<VMTensor> right  = rhso;
  bool          result = (left->GetTensor() == right->GetTensor());
  return result;
}

ChargeAmount VMTensor::IsEqualChargeEstimator(vm::Ptr<Object> const &lhso,
                                              vm::Ptr<Object> const &rhso)
{
  return estimator_.IsEqualChargeEstimator(lhso, rhso);
}

bool VMTensor::IsNotEqual(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left   = lhso;
  Ptr<VMTensor> right  = rhso;
  bool          result = (left->GetTensor() != right->GetTensor());
  return result;
}

ChargeAmount VMTensor::IsNotEqualChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  return estimator_.IsNotEqualChargeEstimator(lhso, rhso);
}

void VMTensor::Negate(fetch::vm::Ptr<Object> &object)
{
  Ptr<VMTensor> operand = object;
  Ptr<VMTensor> t       = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, shape())};
  fetch::math::Multiply(operand->GetTensor(), DataType(-1), t->GetTensor());
  object = std::move(t);
}

ChargeAmount VMTensor::NegateChargeEstimator(vm::Ptr<vm::Object> const &object)
{
  return estimator_.NegateChargeEstimator(object);
}

/////////////////////////
/// BASIC ARITHMETIC  ///
/////////////////////////

void VMTensor::Add(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() + right->GetTensor());
}

ChargeAmount VMTensor::AddChargeEstimator(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  return estimator_.AddChargeEstimator(lhso, rhso);
}

void VMTensor::Subtract(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() - right->GetTensor());
}

ChargeAmount VMTensor::SubtractChargeEstimator(vm::Ptr<Object> const &lhso,
                                               vm::Ptr<Object> const &rhso)
{
  return estimator_.SubtractChargeEstimator(lhso, rhso);
}

void VMTensor::InplaceAdd(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineAdd(right->GetTensor());
}

ChargeAmount VMTensor::InplaceAddChargeEstimator(vm::Ptr<Object> const &lhso,
                                                 vm::Ptr<Object> const &rhso)
{
  return estimator_.InplaceAddChargeEstimator(lhso, rhso);
}

void VMTensor::InplaceSubtract(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineSubtract(right->GetTensor());
}

ChargeAmount VMTensor::InplaceSubtractChargeEstimator(vm::Ptr<Object> const &lhso,
                                                      vm::Ptr<Object> const &rhso)
{
  return estimator_.InplaceSubtractChargeEstimator(lhso, rhso);
}

void VMTensor::Multiply(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() * right->GetTensor());
}

ChargeAmount VMTensor::MultiplyChargeEstimator(vm::Ptr<Object> const &lhso,
                                               vm::Ptr<Object> const &rhso)
{
  return estimator_.MultiplyChargeEstimator(lhso, rhso);
}

void VMTensor::Divide(vm::Ptr<Object> &lhso, vm::Ptr<Object> &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  this->GetTensor()   = (left->GetTensor() / right->GetTensor());
}

ChargeAmount VMTensor::DivideChargeEstimator(vm::Ptr<Object> const &lhso,
                                             vm::Ptr<Object> const &rhso)
{
  return estimator_.DivideChargeEstimator(lhso, rhso);
}

void VMTensor::InplaceMultiply(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineMultiply(right->GetTensor());
}

ChargeAmount VMTensor::InplaceMultiplyChargeEstimator(vm::Ptr<Object> const &lhso,
                                                      vm::Ptr<Object> const &rhso)
{
  return estimator_.InplaceMultiplyChargeEstimator(lhso, rhso);
}

void VMTensor::InplaceDivide(vm::Ptr<Object> const &lhso, vm::Ptr<Object> const &rhso)
{
  Ptr<VMTensor> left  = lhso;
  Ptr<VMTensor> right = rhso;
  left->GetTensor().InlineDivide(right->GetTensor());
}

ChargeAmount VMTensor::InplaceDivideChargeEstimator(vm::Ptr<Object> const &lhso,
                                                    vm::Ptr<Object> const &rhso)
{
  return estimator_.InplaceDivideChargeEstimator(lhso, rhso);
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

vm::Ptr<VMTensor> VMTensor::ArgMaxNoIndices()
{
  return ArgMax();
}

vm::Ptr<VMTensor> VMTensor::ArgMax(SizeType const &indices)
{
  auto          ret_tensor = fetch::math::ArgMax(GetTensor(), indices);
  Ptr<VMTensor> ret        = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, ret_tensor)};
  return ret;
}

vm::Ptr<VMTensor> VMTensor::Dot(vm::Ptr<VMTensor> const &other)
{
  auto          ret_tensor = fetch::math::Dot(GetTensor(), other->GetTensor());
  Ptr<VMTensor> ret        = Ptr<VMTensor>{new VMTensor(this->vm_, this->type_id_, ret_tensor)};
  return ret;
}

//////////////////////////////
/// PRINTING AND EXPORTING ///
//////////////////////////////

void VMTensor::FromString(fetch::vm::Ptr<fetch::vm::String> const &string)
{
  try
  {
    auto input_tensor = fetch::math::Tensor<DataType>::FromString(string->string());
    tensor_.Reshape(input_tensor.shape());
    tensor_.Assign(input_tensor);
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

fixed_point::fp64_t const VMTensor::CONSTRUCTION_PADDED_SIZE_COEF = fixed_point::fp64_t("0.0028");
fixed_point::fp64_t const VMTensor::CONSTRUCTION_CONST_COEF       = fixed_point::fp64_t("22");

fixed_point::fp64_t const VMTensor::CONSTRUCTION_STRING_SIZE_COEF  = fixed_point::fp64_t("0.12");
fixed_point::fp64_t const VMTensor::CONSTRUCTION_STRING_CONST_COEF = fixed_point::fp64_t("25");

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
