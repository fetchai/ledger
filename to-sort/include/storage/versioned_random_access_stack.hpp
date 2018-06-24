#ifndef STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#define STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#include "storage/random_access_stack.hpp"
#include "storage/variant_stack.hpp"

#include <cstring>
namespace fetch {
namespace storage {

template <typename T, typename B = uint64_t>
class VersionedRandomAccessStack : private RandomAccessStack<T, B> {
 private:
  typedef RandomAccessStack<T, B> super_type;

  struct HistoryBookmark {
    HistoryBookmark() { memset(this, 0, sizeof(decltype(*this))); }
    HistoryBookmark(B val) {
      memset(this, 0, sizeof(decltype(*this)));
      bookmark = val;
    }

    enum { value = 0 };
    B bookmark = 0;
  };

  struct HistorySwap {
    HistorySwap() { memset(this, 0, sizeof(decltype(*this))); }

    HistorySwap(uint64_t const &i_, uint64_t const &j_) {
      memset(this, 0, sizeof(decltype(*this)));
      i = i_;
      j = j_;
    }

    enum { value = 1 };
    uint64_t i = 0;
    uint64_t j = 0;
  };

  struct HistoryPop {
    HistoryPop() { memset(this, 0, sizeof(decltype(*this))); }

    HistoryPop(T const &d) {
      memset(this, 0, sizeof(decltype(*this)));
      data = d;
    }

    enum { value = 2 };
    T data;
  };

  struct HistoryPush {
    HistoryPush() { memset(this, 0, sizeof(decltype(*this))); }

    enum { value = 3 };
  };

  struct HistorySet {
    HistorySet() { memset(this, 0, sizeof(decltype(*this))); }

    HistorySet(uint64_t const &i_, T const &d) {
      memset(this, 0, sizeof(decltype(*this)));
      i = i_;
      data = d;
    }

    enum { value = 4 };
    uint64_t i = 0;
    T data;
  };

 public:
  typedef typename RandomAccessStack<T>::type type;
  typedef B bookmark_type;

  void Load(std::string const &filename, std::string const &history) {
    super_type::Load(filename);
    history_.Load(history);
    bookmark_ = super_type::header_extra();
  }

  void New(std::string const &filename, std::string const &history) {
    super_type::New(filename);
    history_.New(history);
    bookmark_ = super_type::header_extra();
  }

  void Clear() {
    super_type::Clear();
    history_.Clear();
    ResetBookmark();
    bookmark_ = super_type::header_extra();
  }

  type Get(std::size_t const &i) const {
    type object;
    super_type::Get(i, object);
    return object;
  }

  void Get(std::size_t const &i, type &object) const {
    super_type::Get(i, object);
  }

  void Set(std::size_t const &i, type const &object) {
    type old_data;
    super_type::Get(i, old_data);
    history_.Push(HistorySet(i, old_data), HistorySet::value);
    super_type::Set(i, object);
  }

  void Push(type const &object) {
    history_.Push(HistoryPush(), HistoryPush::value);
    super_type::Push(object);
  }

  void Pop() {
    type old_data = super_type::Top();
    history_.Push(HistoryPop(old_data), HistoryPop::value);
    super_type::Pop();
  }

  type Top() const { return super_type::Top(); }

  void Swap(std::size_t const &i, std::size_t const &j) {
    history_.Push(HistorySwap(i, j), HistorySwap::value);
    super_type::Swap(i, j);
  }

  void Revert(bookmark_type const &b) {
    while ((!empty()) && (b != bookmark_)) {
      uint64_t t = history_.Type();
      switch (t) {
        case HistoryBookmark::value:
          RevertBookmark();
          break;
        case HistorySwap::value:
          RevertSwap();
          break;
        case HistoryPop::value:
          RevertPop();
          break;
        case HistoryPush::value:
          RevertPush();
          break;
        case HistorySet::value:
          RevertSet();
          break;
        default:
          std::cerr << "Problem: undefined type" << std::endl;
          exit(-1);
      }
    }
    super_type::SetExtraHeader(bookmark_);
  }

  bookmark_type Commit() {
    bookmark_type b = bookmark_;
    NextBookmark();
    history_.Push(HistoryBookmark(b), HistoryBookmark::value);
    super_type::SetExtraHeader(bookmark_);
    return b;
  }

  virtual void ResetBookmark() { bookmark_ = 0; }

  virtual void NextBookmark() { ++bookmark_; }

  virtual void PreviousBookmark() { ++bookmark_; }

  std::size_t size() const { return super_type::size(); }

  std::size_t empty() const { return super_type::empty(); }

 private:
  VariantStack history_;
  bookmark_type bookmark_;

  void RevertBookmark() {
    HistoryBookmark book;
    history_.Top(book);
    history_.Pop();
    bookmark_ = book.bookmark;
  }

  void RevertSwap() {
    HistorySwap swap;
    history_.Top(swap);
    history_.Pop();
    super_type::Swap(swap.i, swap.j);
  }

  void RevertPop() {
    HistoryPop pop;
    history_.Top(pop);
    history_.Pop();
    super_type::Push(pop.data);
  }

  void RevertPush() {
    HistoryPush push;
    history_.Top(push);
    history_.Pop();
    super_type::Pop();
  }

  void RevertSet() {
    HistorySet set;
    history_.Top(set);
    history_.Pop();
    super_type::Set(set.i, set.data);
  }
};
}
}

#endif
