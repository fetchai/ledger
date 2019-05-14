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

#include "vm/variant.hpp"
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
  RuntimeError("operator not implemented");
  return false;
}

bool Object::IsNotEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
  return false;
}

bool Object::IsLessThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
  return false;
}

bool Object::IsLessThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
  return false;
}

bool Object::IsGreaterThan(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
  return false;
}

bool Object::IsGreaterThanOrEqual(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
  return false;
}

void Object::Negate(Ptr<Object> & /* object */)
{
  RuntimeError("operator not implemented");
}

void Object::Add(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftAdd(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightAdd(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceAdd(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceRightAdd(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::Subtract(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftSubtract(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightSubtract(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceSubtract(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceRightSubtract(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::Multiply(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftMultiply(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightMultiply(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceMultiply(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceRightMultiply(Ptr<Object> const & /* lhso */, Variant const & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::Divide(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftDivide(Variant & /* lhsv */, Variant & /* objectv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightDivide(Variant & /* objectv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceDivide(Ptr<Object> const & /* lhso */, Ptr<Object> const & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::InplaceRightDivide(Ptr<Object> const & /* lhso*/, Variant const & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

}  // namespace vm
}  // namespace fetch
