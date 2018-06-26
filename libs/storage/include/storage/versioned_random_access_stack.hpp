#ifndef STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#define STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#include "storage/random_access_stack.hpp"
#include "storage/variant_stack.hpp"

#include <cstring>
namespace fetch {
namespace storage {

template <typename T, typename B = uint64_t>
class VersionedRandomAccessStack {
 private:
  typedef RandomAccessStack<T, B> stack_type;

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

  void Load(std::string const &filename, std::string const &history, bool const &create_if_not_exist = false) {
    stack_.Load(filename, create_if_not_exist);
    history_.Load(history); // TODO: create_if_not_exist
    bookmark_ = stack_.header_extra();
  }

  void New(std::string const &filename, std::string const &history) {
    stack_.New(filename);
    history_.New(history);
    bookmark_ = stack_.header_extra();
  }

  void Clear() {
    stack_.Clear();
    history_.Clear();
    ResetBookmark();
    bookmark_ = stack_.header_extra();
  }

  type Get(std::size_t const &i) const {
    type object;
    stack_.Get(i, object);
    return object;
  }

  void Get(std::size_t const &i, type &object) const {
    stack_.Get(i, object);
  }

  void Set(std::size_t const &i, type const &object) {
    type old_data;
    stack_.Get(i, old_data);
    history_.Push(HistorySet(i, old_data), HistorySet::value);
    stack_.Set(i, object);
  }

  uint64_t Push(type const &object) {
    history_.Push(HistoryPush(), HistoryPush::value);
    return stack_.Push(object);
  }

  void Pop() {
    type old_data = stack_.Top();
    history_.Push(HistoryPop(old_data), HistoryPop::value);
    stack_.Pop();
  }

  type Top() const { return stack_.Top(); }

  void Swap(std::size_t const &i, std::size_t const &j) {
    history_.Push(HistorySwap(i, j), HistorySwap::value);
    stack_.Swap(i, j);
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
          TODO_FAIL("Problem: undefined type");

      }
    }
    stack_.SetExtraHeader(bookmark_);
  }

  bookmark_type Commit() {
    bookmark_type b = bookmark_;
    NextBookmark();
    history_.Push(HistoryBookmark(b), HistoryBookmark::value);
    stack_.SetExtraHeader(bookmark_);
    return b;
  }

  void ResetBookmark() { bookmark_ = 0; }

  void NextBookmark() { ++bookmark_; }

  void PreviousBookmark() { ++bookmark_; }

  std::size_t size() const { return stack_.size(); }

  std::size_t empty() const { return stack_.empty(); }

 private:
  VariantStack history_;
  bookmark_type bookmark_;

  stack_type stack_;
   
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
    stack_.Swap(swap.i, swap.j);
  }

  void RevertPop() {
    HistoryPop pop;
    history_.Top(pop);
    history_.Pop();
    stack_.Push(pop.data);
  }

  void RevertPush() {
    HistoryPush push;
    history_.Top(push);
    history_.Pop();
    stack_.Pop();
  }

  void RevertSet() {
    HistorySet set;
    history_.Top(set);
    history_.Pop();
    stack_.Set(set.i, set.data);
  }
};
}
}

#endif
