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

#include "core/string/trim.hpp"
#include "vm/array.hpp"
#include "vm/string.hpp"

#include "utf8.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

namespace {

int32_t utf8_length(std::string const &str)
{
  return static_cast<int32_t>(utf8::distance(str.cbegin(), str.cend()));
}

std::string reverse_utf8_string_by_copy(std::string const &str)
{
  utf8::iterator<std::string::const_iterator> it{str.cend(), str.cbegin(), str.cend()};
  utf8::iterator<std::string::const_iterator> end{str.cbegin(), str.cbegin(), str.cend()};

  std::string reversed;
  reversed.reserve(str.size());

  for (; it != end;)
  {
    utf8::append(*(--it), reversed);
  }

  return reversed;
}

void reverse_utf8_string_in_place(std::string &str)
{
  // Reverse all bytes in place
  std::reverse(str.begin(), str.end());

  // Repair the mangled UTF-8 code units
  auto start = str.begin();
  auto end   = str.begin();

  while (end != str.end())
  {
    end = std::find_if(start, str.end(), [](auto c) {
      // a == 0b00000000 if the current byte represents a single-byte UTF-8 code unit.
      // a == 0b11000000 if the byte belongs at the beginning of a multibyte UTF-8 code unit.
      // Otherwise, a == 0b10000000
      auto const a = c & 0b11000000;
      return a != 0b10000000;
    });
    if (start == end)
    {
      // Skipping single-byte character
      ++start;
    }
    else
    {
      ++end;
      std::reverse(start, end);
      start = end;
    }
  }
}

}  // namespace

String::String(VM *vm, std::string str__, bool is_literal__)
  : Object(vm, TypeIds::String)
  , str(std::move(str__))
  , is_literal(is_literal__)
  , length{utf8_length(str)}
{}

Ptr<String> String::Trim()
{
  if (IsTemporary())
  {
    fetch::string::Trim(str);

    return Ptr<String>::PtrFromThis(this);
  }

  std::string new_string{str};
  fetch::string::Trim(new_string);

  return Ptr<String>{new String(vm_, new_string)};
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

  auto distance{
      utf8::distance(str.begin(), str.begin() + static_cast<std::string::difference_type>(first))};

  return static_cast<int32_t>(distance);
}

Ptr<String> String::Substring(int32_t start_index, int32_t end_index)
{
  if (start_index < 0)
  {
    RuntimeError("substring start index must be non-negative");
    return {};
  }
  if (end_index < start_index)
  {
    RuntimeError("substring start index must not exceed end index");
    return {};
  }
  if (static_cast<std::size_t>(end_index) > str.size())
  {
    RuntimeError("substring end index exceeds length of string");
    return {};
  }

  auto start{str.begin()};
  // using *unchecked* version to enable index to point at the str.end()
  utf8::unchecked::advance(start, start_index);

  auto end{str.begin()};
  // using *unchecked* version to enable index to point at the str.end()
  utf8::unchecked::advance(end, end_index);  // unchecked

  return Ptr<String>{new String(vm_, std::string{start, end})};
}

Ptr<String> String::Reverse()
{
  if (length < 2)
  {
    return Ptr<String>::PtrFromThis(this);
  }

  if (IsTemporary())
  {
    reverse_utf8_string_in_place(str);

    return Ptr<String>::PtrFromThis(this);
  }

  auto const reversed = reverse_utf8_string_by_copy(str);

  return Ptr<String>{new String(vm_, reversed)};
}

Ptr<Array<Ptr<String>>> String::Split(Ptr<String> const &separator) const
{
  if (separator == nullptr)
  {
    vm_->RuntimeError("split: argument must not be null");
    return Ptr<Array<Ptr<String>>>{
        new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 0)};
  }

  if (separator->str.empty())
  {
    vm_->RuntimeError("split: argument must not be the empty string");
    return Ptr<Array<Ptr<String>>>{
        new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 0)};
  }

  std::vector<std::size_t> segment_boundaries{0};

  for (std::size_t i = str.find(separator->str); i != std::string::npos;
       i             = str.find(separator->str, i + separator->str.length()))
  {
    segment_boundaries.emplace_back(i + separator->str.length());
  }

  // separator not found
  if (segment_boundaries.size() == 1)
  {
    auto ret = Ptr<Array<Ptr<String>>>{
        new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 1)};
    ret->elements[0] = Ptr<String>{new String(vm_, str)};

    return ret;
  }

  segment_boundaries.push_back(str.length() + separator->str.length());

  assert(segment_boundaries.size() > 2u);

  auto ret = Ptr<Array<Ptr<String>>>{
      new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_,
                             static_cast<int32_t>(segment_boundaries.size() - 1))};

  for (std::size_t i = 0; i + 1 < segment_boundaries.size(); ++i)
  {
    std::size_t begin = segment_boundaries[i];
    std::size_t len   = segment_boundaries[i + 1] - begin - separator->str.length();
    ret->elements[i]  = Ptr<String>{new String(vm_, str.substr(begin, len))};
  }

  return ret;
}

int32_t String::Length() const
{
  return length;
}

int32_t String::SizeInBytes() const
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
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;

  if (lhs->IsTemporary())
  {
    lhs->str += rhs->str;
  }
  else
  {
    Ptr<String> s(new String(vm_, lhs->str + rhs->str));
    lhso = std::move(s);
  }
}

bool String::SerializeTo(MsgPackSerializer &buffer)
{
  buffer << str;
  return true;
}

bool String::DeserializeFrom(MsgPackSerializer &buffer)
{
  buffer >> str;
  return true;
}

bool String::IsTemporary() const
{
  return RefCount() == 1;
}

}  // namespace vm
}  // namespace fetch
