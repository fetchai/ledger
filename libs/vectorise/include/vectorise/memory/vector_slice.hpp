#pragma once
#include "vectorise/memory/iterator.hpp"
#include "vectorise/memory/parallel_dispatcher.hpp"
#include "vectorise/meta/log2.hpp"
#include "vectorise/platform.hpp"

#include <cstring>

namespace fetch {
namespace memory {

template <typename T, std::size_t type_size = sizeof(T)>
class VectorSlice
{
public:
  typedef std::size_t size_type;
  typedef T *         pointer_type;
  typedef T const *   const_pointer_type;
  typedef T           type;

  typedef VectorSlice         vector_slice_type;
  typedef ForwardIterator<T>  iterator;
  typedef BackwardIterator<T> reverse_iterator;

  typedef ConstParallelDispatcher<type>                           const_parallel_dispatcher_type;
  typedef ParallelDispatcher<type>                                parallel_dispatcher_type;
  typedef typename parallel_dispatcher_type::vector_register_type vector_register_type;
  typedef typename parallel_dispatcher_type::vector_register_iterator_type
      vector_register_iterator_type;

  enum
  {
    E_TYPE_SIZE     = type_size,
    E_SIMD_SIZE     = (platform::VectorRegisterSize<type>::value >> 3),
    E_SIMD_COUNT_IM = E_SIMD_SIZE / type_size,
    E_SIMD_COUNT =
        (E_SIMD_COUNT_IM > 0 ? E_SIMD_COUNT_IM
                             : 1),  // Note that if a type is too big to fit, we pretend it can
    E_LOG_SIMD_COUNT = fetch::meta::Log2<E_SIMD_COUNT>::value,
    IS_SHARED        = 0
  };

  static_assert(E_SIMD_COUNT == (1ull << E_LOG_SIMD_COUNT), "type does not fit in SIMD");

  VectorSlice(pointer_type ptr = nullptr, std::size_t const &n = 0) : pointer_(ptr), size_(n) {}

  ConstParallelDispatcher<type> in_parallel() const
  {
    return ConstParallelDispatcher<type>(pointer_, size());
  }
  ParallelDispatcher<type> in_parallel() { return ParallelDispatcher<type>(pointer(), size()); }

  iterator         begin() { return iterator(pointer_, pointer_ + size()); }
  iterator         end() { return iterator(pointer_ + size(), pointer_ + size()); }
  reverse_iterator rbegin() { return reverse_iterator(pointer_ + size() - 1, pointer_ - 1); }
  reverse_iterator rend() { return reverse_iterator(pointer_ - 1, pointer_ - 1); }

  template <typename R = T>
  typename std::enable_if<std::is_pod<R>::value>::type SetAllZero()
  {
#if 0
        assert(pointer_ != nullptr);
#else
    if (pointer_)
    {
      std::memset(pointer_, 0, padded_size() * sizeof(type));
    }
#endif
  }

  template <typename R = T>
  typename std::enable_if<(!std::is_pod<R>::value) && (std::is_default_constructible<R>::value && std::is_copy_assignable<R>::value)>::type
  SetAllZero()
  {
    if (pointer_)
    {
      R const initial_value;
      for (std::size_t i = 0; i < size(); ++i)
      {
        operator[](i) = initial_value;
      }
    }
  }

  void SetPaddedZero()
  {
    assert(pointer_ != nullptr);

    std::memset(pointer_ + size(), 0, (padded_size() - size()) * sizeof(type));
  }

  void SetZeroAfter(std::size_t const &n)
  {
    std::memset(pointer_ + n, 0, (padded_size() - n) * sizeof(type));
  }

  // TODO(unknown): THis is ugly. The right way to do this would be to have a separate
  // constant class that is return for slice(...) const
  vector_slice_type slice(std::size_t const &offset, std::size_t const &length) const
  {
    assert(std::size_t(offset / E_SIMD_COUNT) * E_SIMD_COUNT == offset);
    assert(std::size_t(length / E_SIMD_COUNT) * E_SIMD_COUNT == length);
    assert((length + offset) <= size_);
    return vector_slice_type(pointer_ + offset, length);
  }

  T &operator[](std::size_t const &n)
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  T const &operator[](std::size_t const &n) const
  {
    assert(pointer_ != nullptr);

    assert(n < padded_size());
    return pointer_[n];
  }

  T &At(std::size_t const &n)
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  T const &At(std::size_t const &n) const
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    return pointer_[n];
  }

  T const &Set(std::size_t const &n, T const &v)
  {
    assert(pointer_ != nullptr);
    assert(n < padded_size());
    pointer_[n] = v;
    return v;
  }

  std::size_t simd_size() const { return (size_) >> E_LOG_SIMD_COUNT; }
  std::size_t size() const { return size_; }

  std::size_t padded_size() const
  {
    std::size_t padded = std::size_t((size_) >> E_LOG_SIMD_COUNT) << E_LOG_SIMD_COUNT;
    if (padded < size_) padded += E_SIMD_COUNT;
    return padded;
  }

  pointer_type       pointer() { return pointer_; }
  const_pointer_type pointer() const { return pointer_; }
  size_type          size() { return size_; }

protected:
  pointer_type pointer_;
  size_type    size_;
};

}  // namespace memory
}  // namespace fetch
