#ifndef SCRIPT_ARRAY_HPP
#define SCRIPT_ARRAY_HPP
#include "byte_array/byte_array.hpp"
#include "crypto/fnv.hpp"
#include "memory/shared_hashtable.hpp"

#include <vector>

namespace fetch {
namespace script {

template <typename T>
class Array {
 public:
  typedef T type;
  typedef std::vector<type> container_type;
  typedef std::shared_ptr<container_type> shared_container_type;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;

  Array() { data_ = std::make_shared<container_type>(); }

  Array(Array const& dict) { data_ = dict.data_; }

  Array(Array&& dict) { std::swap(data_, dict.data_); }

  Array const& operator=(Array const& dict) {
    data_ = dict.data_;
    return *this;
  }

  Array const& operator=(Array&& dict) {
    std::swap(data_, dict.data_);
    return *this;
  }

  void Reserve(std::size_t const& n) { data_->reserve(n); }

  void Resize(std::size_t const& n) { data_->resize(n); }

  Array<T> Copy() const {
    Array<T> ret;

    ret.Resize(size());
    std::size_t i = 0;

    for (auto const& a : *data_) {
      ret[i++] = a;
    }

    return ret;
  }

  type& operator[](std::size_t const& key) { return (*data_)[key]; }

  type const& operator[](std::size_t const& key) const { return (*data_)[key]; }

  iterator begin() { return data_->begin(); }

  iterator end() { return data_->end(); }

  const_iterator begin() const { return data_->begin(); }

  const_iterator end() const { return data_->end(); }

  std::size_t size() const { return data_->size(); }

 private:
  shared_container_type data_;
};
}
}

#endif
