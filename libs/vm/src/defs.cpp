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

#include "vm/defs.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

Variant &Object::Push()
{
  return vm_->Push();
}

Variant &Object::Pop()
{
  return vm_->Pop();
}

Variant &Object::Top()
{
  return vm_->Top();
}

void Object::RuntimeError(std::string const &message)
{
  vm_->RuntimeError(message);
}

TypeInfo const &Object::GetTypeInfo(TypeId type_id)
{
  return vm_->GetTypeInfo(type_id);
}

bool Object::GetNonNegativeInteger(Variant const &v, size_t &index)
{
  bool ok = true;
  switch (v.type_id)
  {
  case TypeIds::Int8:
  {
    index = size_t(v.primitive.i8);
    ok    = v.primitive.i8 >= 0;
    break;
  }
  case TypeIds::Byte:
  {
    index = size_t(v.primitive.ui8);
    break;
  }
  case TypeIds::Int16:
  {
    index = size_t(v.primitive.i16);
    ok    = v.primitive.i16 >= 0;
    break;
  }
  case TypeIds::UInt16:
  {
    index = size_t(v.primitive.ui16);
    break;
  }
  case TypeIds::Int32:
  {
    index = size_t(v.primitive.i32);
    ok    = v.primitive.i32 >= 0;
    break;
  }
  case TypeIds::UInt32:
  {
    index = size_t(v.primitive.ui32);
    break;
  }
  case TypeIds::Int64:
  {
    index = size_t(v.primitive.i64);
    ok    = v.primitive.i64 >= 0;
    break;
  }
  case TypeIds::UInt64:
  {
    index = size_t(v.primitive.ui64);
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

size_t Object::GetHashCode()
{
  return std::hash<const void *>()(this);
}

bool Object::IsEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsEqual operator not implemented");
  return false;
}

bool Object::IsNotEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsNotEqual operator not implemented");
  return false;
}

bool Object::IsLessThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsLessThan operator not implemented");
  return false;
}

bool Object::IsLessThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsLessThanOrEqual operator not implemented");
  return false;
}

bool Object::IsGreaterThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsGreaterThan operator not implemented");
  return false;
}

bool Object::IsGreaterThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("IsGreaterThanOrEqual operator not implemented");
  return false;
}

void Object::UnaryMinus(Ptr<Object> & /* object */)
{
  RuntimeError("UnaryMinus operator not implemented");
}

void Object::Add(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("Add operator not implemented");
}

void Object::LeftAdd(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("LeftAdd operator not implemented");
}

void Object::RightAdd(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("RightAdd operator not implemented");
}

void Object::AddAssign(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("AddAssign operator not implemented");
}

void Object::RightAddAssign(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("RightAddAssign operator not implemented");
}

void Object::Subtract(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("Subtract operator not implemented");
}

void Object::LeftSubtract(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("LeftSubtract operator not implemented");
}

void Object::RightSubtract(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("RightSubtract operator not implemented");
}

void Object::SubtractAssign(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("SubtractAssign operator not implemented");
}

void Object::RightSubtractAssign(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("RightSubtractAssign operator not implemented");
}

void Object::Multiply(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("Multiply operator not implemented");
}

void Object::LeftMultiply(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("LeftMultiply operator not implemented");
}

void Object::RightMultiply(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("RightMultiply operator not implemented");
}

void Object::MultiplyAssign(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("MultiplyAssign operator not implemented");
}

void Object::RightMultiplyAssign(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("RightMultiplyAssign operator not implemented");
}

void Object::Divide(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("Divide operator not implemented");
}

void Object::LeftDivide(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("LeftDivide operator not implemented");
}

void Object::RightDivide(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("RightDivide operator not implemented");
}

void Object::DivideAssign(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("DivideAssign operator not implemented");
}

void Object::RightDivideAssign(Ptr<Object> & /* lhso*/, Variant & /* rhsv */)
{
  RuntimeError("RightDivideAssign operator not implemented");
}

void *Object::FindElement()
{
  RuntimeError("FindElement operator not implemented");
  return nullptr;
}

void Object::PushElement(TypeId /* element_type_id */)
{
  RuntimeError("PushElement operator not implemented");
}

void Object::PopToElement()
{
  RuntimeError("PopToElement operator not implemented");
}

}  // namespace vm
}  // namespace fetch
