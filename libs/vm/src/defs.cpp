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

bool Object::GetInteger(Variant const &v, size_t &index)
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

bool Object::Equals(Ptr<Object> const &lhso, Ptr<Object> const &rhso) const
{
  return lhso == rhso;
}

size_t Object::GetHashCode() const
{
  return std::hash<const void *>()(this);
}

void Object::UnaryMinusOp(Ptr<Object> & /* object */)
{
  RuntimeError("operator not implemented");
}

void Object::AddOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftAddOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightAddOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::AddAssignOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::RightAddAssignOp(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::SubtractOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftSubtractOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightSubtractOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::SubtractAssignOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::RightSubtractAssignOp(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::MultiplyOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftMultiplyOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightMultiplyOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::MultiplyAssignOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::RightMultiplyAssignOp(Ptr<Object> & /* lhso */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::DivideOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::LeftDivideOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::RightDivideOp(Variant & /* lhsv */, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void Object::DivideAssignOp(Ptr<Object> & /* lhso */, Ptr<Object> & /* rhso */)
{
  RuntimeError("operator not implemented");
}

void Object::RightDivideAssignOp(Ptr<Object> & /* lhso*/, Variant & /* rhsv */)
{
  RuntimeError("operator not implemented");
}

void *Object::FindElement()
{
  RuntimeError("operator not implemented");
  return nullptr;
}

void Object::PushElement(TypeId /* element_type_id */)
{
  RuntimeError("operator not implemented");
}

void Object::PopToElement()
{
  RuntimeError("operator not implemented");
}

}  // namespace vm
}  // namespace fetch
