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

#include "core/string/trim.hpp"
#include "vm/array.hpp"
#include "vm/string.hpp"

#include "utf8.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

namespace {

int32_t Utf8StringLength(std::string const &str)
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
      // a == 0b00000000 or 0b01000000 if the current byte represents a single-byte UTF-8 code unit.
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

String::String(VM *vm, std::string str__)
  : Object(vm, TypeIds::String)
  , utf8_str_(std::move(str__))
{}

Ptr<String> String::Trim()
{
  if (IsTemporary())
  {
    fetch::string::Trim(utf8_str_.str_);

    return Ptr<String>::PtrFromThis(this);
  }

  std::string new_string{utf8_str_.string()};
  fetch::string::Trim(new_string);

  return Ptr<String>{new String(vm_, new_string)};
}

int32_t String::Find(Ptr<String> const &substring) const
{
  constexpr int32_t NOT_FOUND = -1;

  // No string contains the empty string (incl. the empty string itself)
  if (utf8_str_.empty() || substring->utf8_str_.empty())
  {
    return NOT_FOUND;
  }

  auto const first = utf8_str_.string().find(substring->utf8_str_.string());
  if (first == std::string::npos)
  {
    return NOT_FOUND;
  }

  auto distance{utf8::distance(
      utf8_str_.string().begin(),
      utf8_str_.string().begin() + static_cast<std::string::difference_type>(first))};

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
  if (static_cast<std::size_t>(end_index) > utf8_str_.string().size())
  {
    RuntimeError("substring end index exceeds length of string");
    return {};
  }

  auto start{utf8_str_.string().begin()};
  // using *unchecked* version to enable index to point at the str.end()
  utf8::unchecked::advance(start, start_index);

  auto end{utf8_str_.string().begin()};
  // using *unchecked* version to enable index to point at the str.end()
  utf8::unchecked::advance(end, end_index);  // unchecked

  return Ptr<String>{new String(vm_, std::string{start, end})};
}

Ptr<String> String::Reverse()
{
  if (utf8_str_.size() < 2)
  {
    return Ptr<String>::PtrFromThis(this);
  }

  if (IsTemporary())
  {
    reverse_utf8_string_in_place(utf8_str_.str_);

    return Ptr<String>::PtrFromThis(this);
  }

  auto const reversed = reverse_utf8_string_by_copy(utf8_str_.string());

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

  if (separator->utf8_str_.empty())
  {
    vm_->RuntimeError("split: argument must not be the empty string");
    return Ptr<Array<Ptr<String>>>{
        new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 0)};
  }

  std::vector<std::size_t> segment_boundaries{0};

  for (std::size_t i = utf8_str_.string().find(separator->utf8_str_.string());
       i != std::string::npos;
       i = utf8_str_.string().find(separator->utf8_str_.string(),
                                   i + separator->utf8_str_.string().size()))
  {
    segment_boundaries.emplace_back(i + separator->utf8_str_.string().size());
  }

  // separator not found
  if (segment_boundaries.size() == 1)
  {
    auto ret = Ptr<Array<Ptr<String>>>{
        new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_, 1)};
    ret->elements[0] = Ptr<String>{new String(vm_, utf8_str_.string())};

    return ret;
  }

  segment_boundaries.push_back(utf8_str_.string().size() + separator->utf8_str_.string().size());

  assert(segment_boundaries.size() > 2u);

  auto ret = Ptr<Array<Ptr<String>>>{
      new Array<Ptr<String>>(vm_, vm_->GetTypeId<Array<Ptr<String>>>(), type_id_,
                             static_cast<int32_t>(segment_boundaries.size() - 1))};

  for (std::size_t i = 0; i + 1 < segment_boundaries.size(); ++i)
  {
    std::size_t begin = segment_boundaries[i];
    std::size_t len   = segment_boundaries[i + 1] - begin - separator->utf8_str_.string().size();
    ret->elements[i]  = Ptr<String>{new String(vm_, utf8_str_.string().substr(begin, len))};
  }

  return ret;
}

int32_t String::Length() const
{
  return utf8_str_.size();
}

int32_t String::SizeInBytes() const
{
  return static_cast<int32_t>(utf8_str_.string().size());
}

std::size_t String::GetHashCode()
{
  return std::hash<std::string>()(utf8_str_.string());
}

bool String::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ == rhs->utf8_str_;
}

bool String::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ != rhs->utf8_str_;
}

bool String::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ < rhs->utf8_str_;
}

bool String::IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ <= rhs->utf8_str_;
}

bool String::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ > rhs->utf8_str_;
}

bool String::IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;
  return lhs->utf8_str_ >= rhs->utf8_str_;
}

void String::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<String> lhs = lhso;
  Ptr<String> rhs = rhso;

  if (lhs->IsTemporary())
  {
    lhs->utf8_str_ += rhs->utf8_str_;
  }
  else
  {
    Ptr<String> s(new String(vm_, lhs->utf8_str_.string() + rhs->utf8_str_.string()));
    lhso = std::move(s);
  }
}

ChargeAmount String::IsEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                            Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::IsNotEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                               Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::IsLessThanChargeEstimator(Ptr<Object> const & /*lhso*/,
                                               Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::IsLessThanOrEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                      Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::IsGreaterThanChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                  Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::IsGreaterThanOrEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                         Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount String::AddChargeEstimator(Ptr<Object> const & /*lhso*/, Ptr<Object> const & /*rhso*/)
{
  return 1;
}

bool String::SerializeTo(MsgPackSerializer &buffer)
{
  buffer << utf8_str_.string();
  return true;
}

bool String::DeserializeFrom(MsgPackSerializer &buffer)
{
  std::string str;
  buffer >> str;

  UpdateString(str);

  return true;
}

void String::UpdateString(std::string str)
{
  utf8_str_ = Utf8String{std::move(str)};
}

std::string const &String::string() const
{
  return utf8_str_.string();
}

Utf8String::Utf8String(std::string str)
  : str_{std::move(str)}
  , size_{Utf8StringLength(str_)}
{}

Utf8String &Utf8String::operator+=(Utf8String const &other)
{
  str_ += other.str_;
  size_ += other.size_;

  return *this;
}

bool Utf8String::operator==(Utf8String const &other) const
{
  if (size_ != other.size_)
  {
    return false;
  }

  return str_ == other.str_;
}

bool Utf8String::operator!=(Utf8String const &other) const
{
  return !operator==(other);
}

bool Utf8String::operator>=(Utf8String const &other) const
{
  return str_ >= other.str_;
}

bool Utf8String::operator<=(Utf8String const &other) const
{
  return str_ <= other.str_;
}

bool Utf8String::operator>(Utf8String const &other) const
{
  return str_ > other.str_;
}

bool Utf8String::operator<(Utf8String const &other) const
{
  return str_ < other.str_;
}

int32_t Utf8String::size() const
{
  return size_;
}

bool Utf8String::empty() const
{
  return size_ == 0;
}

std::string const &Utf8String::string() const
{
  return str_;
}

}  // namespace vm
}  // namespace fetch
