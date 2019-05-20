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

#include "vm/string.hpp"

namespace fetch {
namespace vm {

size_t String::GetHashCode()
{
  return std::hash<std::string>()(str);
}

bool String::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str == rhs->str;
}

bool String::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str != rhs->str;
}

bool String::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str < rhs->str;
}

bool String::IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str <= rhs->str;
}

bool String::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str > rhs->str;
}

bool String::IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->str >= rhs->str;
}

void String::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  bool const  lhs_is_modifiable = lhso.RefCount() == 1;
  Ptr<String> lhs               = lhso;
  Ptr<String> rhs               = rhso;
  if (lhs_is_modifiable)
  {
    lhs->str += rhs->str;
  }
  else
  {
    Ptr<String> s(new String(vm_, lhs->str + rhs->str));
    lhso = std::move(s);
  }
}

bool String::SerializeTo(ByteArrayBuffer &buffer)
{
  buffer << str;
  return true;
}

bool String::DeserializeFrom(ByteArrayBuffer &buffer)
{
  buffer >> str;
  return true;
}

}  // namespace vm
}  // namespace fetch
