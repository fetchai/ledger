#pragma once

#include "vectorise/register.hpp"

#include <cassert>

namespace fetch {
namespace vectorize {

template <typename T, std::size_t N = sizeof(T)>
class VectorRegisterIterator
{
public:
  typedef T                                               type;
  typedef VectorRegister<T, N>                            vector_register_type;
  typedef typename vector_register_type::mm_register_type mm_register_type;

  VectorRegisterIterator() : ptr_(nullptr), end_(nullptr) {}
  /*
  VectorRegisterIterator(memory::Array< type > const &arr)
    : ptr_((mm_register_type *)arr.pointer()),
      end_((mm_register_type *)(arr.pointer() + arr.size())) {}

  VectorRegisterIterator(memory::SharedArray< type > const &arr)
    : ptr_((mm_register_type *)arr.pointer()),
      end_((mm_register_type *)(arr.pointer() + arr.size())) {}
  */
  VectorRegisterIterator(type const *d, std::size_t size)
    : ptr_((mm_register_type *)d), end_((mm_register_type *)(d + size))
  {}

  void Next(vector_register_type &m)
  {
    assert((end_ == nullptr) || (ptr_ < end_));

    m.data() = *ptr_;
    ++ptr_;
  }

private:
  mm_register_type *ptr_;
  mm_register_type *end_;
};
}  // namespace vectorize
}  // namespace fetch
