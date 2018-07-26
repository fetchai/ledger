#ifndef CONTAINERS_VECTOR_HPP
#define CONTAINERS_VECTOR_HPP

#include <vector>

namespace fetch {
namespace containers {

template <typename T>
class Vector : private std::vector<T>
{
public:
  typedef std::vector<T> super_type;
  typedef T              type;

  type &At(std::size_t const i) { return super_type::at(i); }

  type const &At(std::size_t const i) const { return super_type::at(i); }

  type &      operator[](std::size_t const i) { return super_type::at(i); }
  type const &operator[](std::size_t const i) const
  {
    return super_type::at(i);
  }

  type &      Front() { return super_type::front(); }
  type const &Front() const { return super_type::front(); }

  type &      Back() { return super_type::back(); }
  type const &Back() const { return super_type::back(); }

  void Clear() { return super_type::clear(); }

  void Insert(type const &element) {}

  void Erase(std::size_t const &pos) {}

  void PushBack(type const &element) { super_type::push_back(element); }

  void PopBack() { super_type::pop_back(); }

  void Resize(std::size_t const &n) { super_type::resize(n); }

  void Reserve(std::size_t const n) { return super_type::reserve(n); }

  bool empty() const { return super_type::empty(); }

  std::size_t size() const { return super_type::size(); }

  std::size_t capacity() const { return super_type::capacity(); }

  void swap(Vector &other) {}
};
}  // namespace containers
}  // namespace fetch
#endif
