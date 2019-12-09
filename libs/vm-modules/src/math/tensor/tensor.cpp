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
