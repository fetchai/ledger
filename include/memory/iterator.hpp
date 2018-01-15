#ifndef MEMORY_ITERATOR_HPP
#define MEMORY_ITERATOR_HPP

#include <cassert>

namespace fetch {
namespace memory {

template <typename T>
class ForwardIterator {
 public:
  ForwardIterator() = delete;

  ForwardIterator(ForwardIterator const &other) = default;
  ForwardIterator(ForwardIterator &&other) = default;
  ForwardIterator &operator=(ForwardIterator const &other) = default;
  ForwardIterator &operator=(ForwardIterator &&other) = default;

  ForwardIterator(T *pos) : pos_(pos) {}
  ForwardIterator(T *pos, T *end) : pos_(pos), end_(end) {}

  ForwardIterator &operator++() { ++pos_; }

  T &operator*() {
    assert(pos_ != nullptr);
    assert((end_ != nullptr) && (pos_ < end_));
    return *pos_;
  }

  bool operator==(ForwardIterator const &other) { return other.pos_ == pos_; }
  bool operator!=(ForwardIterator const &other) { return other.pos_ != pos_; }

 private:
  T *pos_ = nullptr;
  T *end_ = nullptr;
};

template <typename T>
class BackwardIterator {
 public:
  BackwardIterator() = delete;

  BackwardIterator(BackwardIterator const &other) = default;
  BackwardIterator(BackwardIterator &&other) = default;
  BackwardIterator &operator=(BackwardIterator const &other) = default;
  BackwardIterator &operator=(BackwardIterator &&other) = default;

  BackwardIterator(T *pos) : pos_(pos) {}
  BackwardIterator(T *pos, T *begin) : pos_(pos), begin_(begin) {}

  BackwardIterator &operator++() { --pos_; }

  T &operator*() {
    assert(pos_ != nullptr);
    assert((begin_ != nullptr) && (pos_ > begin_));
    return *pos_;
  }

  bool operator==(BackwardIterator const &other) { return other.pos_ == pos_; }
  bool operator!=(BackwardIterator const &other) { return other.pos_ != pos_; }

 private:
  T *pos_ = nullptr;
  T *begin_ = nullptr;
};
};
};
#endif
