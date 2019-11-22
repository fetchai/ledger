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

#include "vm/fixed.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

Fixed128::Fixed128(VM *vm, fixed_point::fp128_t const &data__)
  : Object(vm, TypeIds::Fixed128)
  , data{data__}
{}

bool Fixed128::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data == rhs->data;
}

bool Fixed128::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data != rhs->data;
}

bool Fixed128::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data < rhs->data;
}

bool Fixed128::IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data <= rhs->data;
}

bool Fixed128::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data > rhs->data;
}

bool Fixed128::IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data >= rhs->data;
}

void Fixed128::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  if (lhs->IsTemporary())
  {
    lhs->data += rhs->data;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->data += lhs->data;
    lhso = std::move(rhs);
    return;
  }
  Ptr<Fixed128> n(new Fixed128(vm_, lhs->data + rhs->data));
  lhso = std::move(n);
}

void Fixed128::InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  lhs->data += rhs->data;
}

}  // namespace vm
}  // namespace fetch
