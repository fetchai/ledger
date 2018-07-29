#include "core/script/variant.hpp"

namespace fetch {
namespace script {

Variant::List::List() { pointer_ = data_.pointer(); }

Variant::List::List(std::size_t const &size) { Resize(size); }

Variant::List::List(List const &other, std::size_t offset, std::size_t size)
  : size_(size), offset_(offset), data_(other.data_)
{
  pointer_ = data_.pointer() + offset_;
}

Variant::List::List(List &&other) noexcept
{
  std::swap(size_, other.size_);
  std::swap(offset_, other.offset_);
  std::swap(data_, other.data_);
  std::swap(pointer_, other.pointer_);
}

Variant::List &Variant::List::operator=(List &&other) noexcept
{
  std::swap(size_, other.size_);
  std::swap(offset_, other.offset_);
  std::swap(data_, other.data_);
  std::swap(pointer_, other.pointer_);
  return *this;
}

Variant const &Variant::List::operator[](std::size_t const &i) const { return pointer_[i]; }

Variant &Variant::List::operator[](std::size_t const &i) { return pointer_[i]; }

void Variant::List::Resize(std::size_t const &n)
{
  if (size_ == n) return;
  Reserve(n);
  size_ = n;
}

void Variant::List::LazyResize(std::size_t const &n)
{
  if (size_ == n) return;
  LazyReserve(n);
  size_ = n;
}

void Variant::List::Reserve(std::size_t const &n)
{
  if (offset_ + n < data_.size()) return;

  memory::SharedArray<Variant> new_data(n);

  for (std::size_t i = 0; i < size_; ++i)
  {
    new_data[i] = data_[i];
  }

  data_    = new_data;
  offset_  = 0;
  pointer_ = data_.pointer();
}

void Variant::List::LazyReserve(std::size_t const &n)
{
  if (offset_ + n < data_.size()) return;

  memory::SharedArray<Variant> new_data(n);

  data_    = new_data;
  offset_  = 0;
  pointer_ = data_.pointer();
}

void Variant::List::SetData(List const &other, std::size_t offset, std::size_t size)
{
  data_    = other.data_;
  size_    = size;
  offset_  = offset;
  pointer_ = data_.pointer() + offset;
}

// Array accessors
Variant &Variant::operator[](std::size_t const &i)
{
  assert(type_ == ARRAY);
  assert(i < size());
  return array_[i];
}

Variant const &Variant::operator[](std::size_t const &i) const { return array_[i]; }

std::size_t Variant::size() const
{
  if (type_ == ARRAY) return array_.size();
  if (type_ == STRING) return string_.size();
  return 0;
}

}  // namespace script
}  // namespace fetch
