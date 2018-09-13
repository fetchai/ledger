//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/script/variant.hpp"

namespace fetch {
namespace script {

Variant::Variant(std::initializer_list<Variant> const &lst)
{
  type_ = ARRAY;
  VariantArray data(lst.size());
  std::size_t  i = 0;
  for (auto const &a : lst) data[i++] = a;

  *array_ = data;
}

Variant &Variant::operator=(char const *data)
{
  if (data == nullptr)
    type_ = NULL_VALUE;
  else
  {
    type_   = STRING;
    string_ = ConstByteArray(data);
  }

  return *this;
}

VariantProxy Variant::operator[](ConstByteArray const &key)
{
  assert(type_ == OBJECT);
  std::size_t i = 0;

  // locate the desired entry
  for (; i < array_->size(); i += 2)
  {
    if (key == (*array_)[i].as_byte_array()) break;
  }

  // new entry
  if (i == array_->size())
  {
    return VariantProxy(key, this);
  }

  // existing entry
  return VariantProxy(key, this, &(*array_)[i + 1]);
}

Variant const &Variant::operator[](ConstByteArray const &key) const
{
  static Variant undefined_variant;
  assert(type_ == OBJECT);
  std::size_t i = FindKeyIndex(key);

  if (i == array_->size())
  {
    return undefined_variant;
  }
  return (*array_)[i + 1];
}

bool Variant::Append(ConstByteArray const &key, Variant const &val)
{
  std::size_t i = FindKeyIndex(key);

  if (i == array_->size())
  {
    LazyAppend(key, val);

    return true;
  }

  return false;
}

void Variant::SetArray(VariantArray const &data, std::size_t offset, std::size_t size)
{
  type_ = ARRAY;
  array_->SetData(data, offset, size);
}

void Variant::SetObject(VariantArray const &data, std::size_t offset, std::size_t size)
{
  type_ = OBJECT;
  array_->SetData(data, offset, size);
}

std::size_t Variant::FindKeyIndex(ConstByteArray const &key) const
{
  std::size_t i = 0;
  for (; i < array_->size(); i += 2)
  {
    if (key == (*array_)[i].as_byte_array()) break;
  }
  return i;
}

void Variant::LazyAppend(ConstByteArray const &key, Variant const &val)
{
  assert(type_ == OBJECT);
  array_->Resize(array_->size() + 2);

  (*array_)[array_->size() - 2] = key;
  (*array_)[array_->size() - 1] = val;
}

// Variant Array

VariantArray::VariantArray(std::size_t const &size) { Resize(size); }

VariantArray::VariantArray(VariantArray const &other, std::size_t offset, std::size_t size)
  : size_(size), offset_(offset), data_(other.data_)
{
  pointer_ = data_->data() + offset_;
}

Variant const &VariantArray::operator[](std::size_t const &i) const { return pointer_[i]; }

Variant &VariantArray::operator[](std::size_t const &i) { return pointer_[i]; }

void VariantArray::Resize(std::size_t const &n)
{
  if (size_ == n) return;
  Reserve(n);
  size_ = n;
}

void VariantArray::Reserve(std::size_t const &n)
{
  std::size_t const data_size = (data_) ? data_->size() : 0;

  if (offset_ + n < data_size) return;

  ContainerPtr new_data = std::make_shared<Container>(n);

  for (std::size_t i = 0; i < size_; ++i)
  {
    (*new_data)[i] = (*data_)[i];
  }

  std::swap(data_, new_data);
//  data_    = new_data;
  offset_  = 0;
  pointer_ = data_->data();
}

void VariantArray::SetData(VariantArray const &other, std::size_t offset, std::size_t size)
{
  data_    = other.data_;
  size_    = size;
  offset_  = offset;
  pointer_ = data_->data() + offset;
}

Variant &Variant::operator[](std::size_t const &i)
{
  assert(type_ == ARRAY);
  assert(i < size());
  return (*array_)[i];
}

Variant const &Variant::operator[](std::size_t const &i) const { return (*array_)[i]; }

std::size_t Variant::size() const
{
  if (type_ == ARRAY) return array_->size();
  if (type_ == STRING) return string_.size();
  return 0;
}

}  // namespace script
}  // namespace fetch
