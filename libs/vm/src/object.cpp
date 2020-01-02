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

#include "vm/variant.hpp"
#include "vm/vm.hpp"

#include <cstddef>
#include <string>

namespace fetch {
namespace vm {

void Object::RuntimeError(std::string const &message)
{
  vm_->RuntimeError(message);
}

TypeInfo Object::GetTypeInfo(TypeId type_id)
{
  return vm_->GetTypeInfo(type_id);
}

bool Object::GetNonNegativeInteger(Variant const &v, std::size_t &index)
{
  bool ok = true;
  switch (v.type_id)
  {
  case TypeIds::Int8:
  {
    index = std::size_t(v.primitive.i8);
    ok    = v.primitive.i8 >= 0;
    break;
  }
  case TypeIds::UInt8:
  {
    index = std::size_t(v.primitive.ui8);
    break;
  }
  case TypeIds::Int16:
  {
    index = std::size_t(v.primitive.i16);
    ok    = v.primitive.i16 >= 0;
    break;
  }
  case TypeIds::UInt16:
  {
    index = std::size_t(v.primitive.ui16);
    break;
  }
  case TypeIds::Int32:
  {
    index = std::size_t(v.primitive.i32);
    ok    = v.primitive.i32 >= 0;
    break;
  }
  case TypeIds::UInt32:
  {
    index = std::size_t(v.primitive.ui32);
    break;
  }
  case TypeIds::Int64:
  {
    index = std::size_t(v.primitive.i64);
    ok    = v.primitive.i64 >= 0;
    break;
  }
  case TypeIds::UInt64:
  {
    index = std::size_t(v.primitive.ui64);
    break;
  }
  default:
  {
    ok = false;
    break;
  }
  }  // switch
  return ok;
}

std::size_t Object::GetHashCode()
{
  return std::hash<const void *>()(this);
}

bool Object::IsEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

bool Object::IsNotEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

bool Object::IsLessThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

bool Object::IsLessThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

bool Object::IsGreaterThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

bool Object::IsGreaterThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
  return false;
}

void Object::Negate(Ptr<Object> & /* object */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::Add(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::LeftAdd(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::RightAdd(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceAdd(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceRightAdd(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::Subtract(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::LeftSubtract(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::RightSubtract(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceSubtract(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceRightSubtract(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::Multiply(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::LeftMultiply(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::RightMultiply(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceMultiply(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceRightMultiply(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::Divide(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::LeftDivide(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::RightDivide(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceDivide(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

void Object::InplaceRightDivide(Ptr<Object> const & /* lhso*/, Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": operator not implemented");
}

ChargeAmount Object::IsEqualChargeEstimator(Ptr<Object> const & /* lhso */,
                                            Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::IsNotEqualChargeEstimator(Ptr<Object> const & /* lhso */,
                                               Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::IsLessThanChargeEstimator(Ptr<Object> const & /* lhso */,
                                               Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::IsLessThanOrEqualChargeEstimator(Ptr<Object> const & /* lhso */,
                                                      Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::IsGreaterThanChargeEstimator(Ptr<Object> const & /* lhso */,
                                                  Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::IsGreaterThanOrEqualChargeEstimator(Ptr<Object> const & /* lhso */,
                                                         Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::NegateChargeEstimator(Ptr<Object> const & /* object */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::AddChargeEstimator(Ptr<Object> const & /* lhso */,
                                        Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::LeftAddChargeEstimator(Variant const & /* lhsv */,
                                            Variant const & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::RightAddChargeEstimator(Variant const & /* objectv */,
                                             Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceAddChargeEstimator(Ptr<Object> const & /* lhso */,
                                               Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceRightAddChargeEstimator(Ptr<Object> const & /* lhso */,
                                                    Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::SubtractChargeEstimator(Ptr<Object> const & /* lhso */,
                                             Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::LeftSubtractChargeEstimator(Variant const & /* lhsv */,
                                                 Variant const & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::RightSubtractChargeEstimator(Variant const & /* objectv */,
                                                  Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceSubtractChargeEstimator(Ptr<Object> const & /* lhso */,
                                                    Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceRightSubtractChargeEstimator(Ptr<Object> const & /* lhso */,
                                                         Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::MultiplyChargeEstimator(Ptr<Object> const & /* lhso */,
                                             Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::LeftMultiplyChargeEstimator(Variant const & /* lhsv */,
                                                 Variant const & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::RightMultiplyChargeEstimator(Variant const & /* objectv */,
                                                  Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceMultiplyChargeEstimator(Ptr<Object> const & /* lhso */,
                                                    Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceRightMultiplyChargeEstimator(Ptr<Object> const & /* lhso */,
                                                         Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::DivideChargeEstimator(Ptr<Object> const & /* lhso */,
                                           Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::LeftDivideChargeEstimator(Variant const & /* lhsv */,
                                               Variant const & /* objectv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::RightDivideChargeEstimator(Variant const & /* objectv */,
                                                Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceDivideChargeEstimator(Ptr<Object> const & /* lhso */,
                                                  Ptr<Object> const & /* rhso */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

ChargeAmount Object::InplaceRightDivideChargeEstimator(Ptr<Object> const & /* lhso*/,
                                                       Variant const & /* rhsv */)
{
  RuntimeError(std::string(__func__) + ": estimator not implemented");
  return 1;
}

bool Object::SerializeTo(MsgPackSerializer & /*buffer*/)
{
  vm_->RuntimeError("Serializer for " + GetTypeName() + " is not defined.");
  return false;
}

bool Object::DeserializeFrom(MsgPackSerializer & /*buffer*/)
{
  vm_->RuntimeError("Deserializer for " + GetTypeName() + " is not defined.");
  return false;
}

std::string Object::GetTypeName() const
{
  return vm_->GetTypeName(type_id_);
}

bool Object::ToJSON(JSONVariant &variant)
{
  variant = "JSON serializer for " + GetTypeName() + " is not defined.";
  vm_->RuntimeError("JSON serializer for " + GetTypeName() + " is not defined.");
  return false;
}

bool Object::FromJSON(JSONVariant const & /*variant*/)
{
  vm_->RuntimeError("JSON deserializer for " + GetTypeName() + " is not defined.");
  return false;
}

}  // namespace vm
}  // namespace fetch
