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

#include "vm/array.hpp"
#include "vm/string.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

namespace {

bool is_not_whitespace(int ch)
{
  return !std::isspace(ch);
}

void trim_left(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), is_not_whitespace));
}

void trim_right(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), is_not_whitespace).base(), s.end());
}

}  // namespace

void String::Trim()
{
  trim_left(str);
  trim_right(str);
}

int32_t String::Find(Ptr<String> const &substring) const
{
  constexpr int32_t NOT_FOUND = -1;

  // No string contains the empty string (incl. the empty string itself)
  if (str.empty() || substring->str.empty())
  {
    return NOT_FOUND;
  }

  auto const first = str.find(substring->str);
  if (first == std::string::npos)
  {
    return NOT_FOUND;
  }

  return static_cast<int32_t>(first);
}

Ptr<String> String::Substring(int32_t start_index, int32_t end_index)
{
  if (start_index < 0)
  {
    RuntimeError("substring start index must be non-negative");
    return nullptr;
  }
  if (end_index < start_index)
  {
    RuntimeError("substring start index must not exceed end index");
    return nullptr;
  }
  if (static_cast<std::size_t>(end_index) > str.size())
  {
    RuntimeError("substring end index exceeds length of string");
    return nullptr;
  }

  auto const substring_length =
      static_cast<std::size_t>(end_index) - static_cast<std::size_t>(start_index);

  return new String(vm_, str.substr(static_cast<std::size_t>(start_index), substring_length));
}

void String::Reverse()
{
  std::reverse(str.begin(), str.end());
}

Ptr<Array<Ptr<String>>> String::Split(Ptr<String> const &separator) const
{
  if (separator == nullptr)
  {
    vm_->RuntimeError("split: argument must not be null");
    return new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 0);
  }

  if (separator->str.empty())
  {
    vm_->RuntimeError("split: argument must not be the empty string");
    return new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 0);
  }

  std::vector<std::size_t> segment_boundaries{0};

  for (std::size_t i = str.find(separator->str); i != std::string::npos;
       i             = str.find(separator->str, i + separator->str.length()))
  {
    segment_boundaries.push_back(i + separator->str.length());
  }

  // separator not found
  if (segment_boundaries.size() == 1)
  {
    auto ret = new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 1);
    ret->elements[0] = new String(vm_, str);

    return ret;
  }

  segment_boundaries.push_back(str.length() + separator->str.length());

  assert(segment_boundaries.size() > 2u);

  auto ret = new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_,
                                    static_cast<int32_t>(segment_boundaries.size() - 1));

  for (std::size_t i = 0; i + 1 < segment_boundaries.size(); ++i)
  {
    std::size_t begin  = segment_boundaries[i];
    std::size_t length = segment_boundaries[i + 1] - begin - separator->str.length();
    ret->elements[i]   = new String(vm_, str.substr(begin, length));
  }

  return ret;
}

int32_t String::Length() const
{
  return static_cast<int32_t>(str.size());
}

std::size_t String::GetHashCode()
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

Ptr<String> String::Constructor(VM *vm, TypeId)
{
  return new String(vm, "");
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
