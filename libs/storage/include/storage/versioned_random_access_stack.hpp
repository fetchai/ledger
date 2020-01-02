#pragma once
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

//
//                   RANDOM ACCESS STACK
//
//  ┌──────┬───────────┬───────────┬───────────┬───────────┐
//  │      │           │           │           │           │
//  │HEADER│  OBJECT   │  OBJECT   │  OBJECT   │  OBJECT   │
//  │      │           │           │           │           │......
//  │      │           │           │           │           │
//  └──────┴───────────┴───────────┴───────────┴───────────┘
//               │         ▲
//               │         │
//               │         │
//               ▼         │
//       ┌──────┬──────┬──────┬──────┬──────┐
//       │      │      │      │      │      │
// ......│ PUSH │ POP  │ SWAP │BKMARK│ PUSH │  HISTORY
//       │      │      │      │      │      │
//       └──────┴──────┴──────┴──────┴──────┘

#include "storage/cached_random_access_stack.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/storage_exception.hpp"
#include "storage/variant_stack.hpp"

#include <cstring>

namespace fetch {
namespace storage {

/**
 * Bookmark represents a position in the history which can be reverted to
 */
template <typename B>
struct BookmarkHeader
{
  B        header;
  uint64_t bookmark;
};

/**
 * VersionedRandomAccessStack implements a random access stack that can revert to a previous state.
 * It does this by having a random access stack, and also keeping a stack recording all of the
 * state-changing operations made to the stack (in the history).
 * The user can place bookmarks which allow reverting the stack to it's state at that point in time.
 *
 * The history is a variant stack so as to allow different operations to be saved. However note that
 * the stack itself has elements of constant width, so no dynamically allocated memory.
 *
 */
template <typename T, typename B = uint64_t, typename S = RandomAccessStack<T, BookmarkHeader<B>>>
class VersionedRandomAccessStack
{
private:
  using StackType       = S;
  using HeaderExtraType = B;
  using HeaderType      = BookmarkHeader<B>;

  static constexpr char const *LOGGING_NAME = "VersionedRandomAccessStack";

  /**
   * To be pushed onto the history stack as a variant.
   *
   * Represents a 'bookmark' that users can revert to.
   */
  struct HistoryBookmark
  {
    HistoryBookmark()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    explicit HistoryBookmark(B const &val)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
      bookmark = val;
    }

    enum
    {
      value = 0
    };

    B bookmark = 0;
  };

  /**
   * To be pushed onto the history stack as a variant.
   *
   * Represents a swap on the main stack, holds the information about which elements were swapped
   */
  struct HistorySwap
  {
    HistorySwap()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    HistorySwap(uint64_t i_, uint64_t j_)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));

      i = i_;
      j = j_;
    }

    enum
    {
      value = 1
    };

    uint64_t i = 0;
    uint64_t j = 0;
  };

  /**
   * To be pushed onto the history stack as a variant.
   *
   * Represents a pop on the main stack, holds the popped element T from the main stack
   */
  struct HistoryPop
  {
    HistoryPop()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    explicit HistoryPop(T const &d)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));

      data = d;
    }

    enum
    {
      value = 2
    };

    T data{};
  };

  /**
   * To be pushed onto the history stack as a variant.
   *
   * Represents a push on the main stack, doesn't need any meta data since when reverting the
   * history this corresponds to a pop
   */
  struct HistoryPush
  {
    HistoryPush()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    enum
    {
      value = 3
    };
  };

  /**
   * To be pushed onto the history stack as a variant.
   *
   * Represents setting the history at a specific index.
   *
   * @param: i The index
   * @param: d The item
   *
   */
  struct HistorySet
  {
    HistorySet()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    HistorySet(uint64_t i_, T const &d)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));

      i    = i_;
      data = d;
    }

    enum
    {
      value = 4
    };

    uint64_t i = 0;
    T        data;
  };

  /**
   * Represents a change to the header of the main stack, for example when changing the
   * 'extra' data that can be stored there.
   */
  struct HistoryHeader
  {
    HistoryHeader()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    explicit HistoryHeader(B const &d)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));

      data = d;
    }

    enum
    {
      value = 5
    };

    B data = 0;
  };

public:
  using type             = T;
  using BookmarkType     = B;
  using EventHandlerType = std::function<void()>;

  VersionedRandomAccessStack()
  {
    stack_.OnFileLoaded([this]() { SignalFileLoaded(); });
    stack_.OnBeforeFlush([this]() { SignalBeforeFlush(); });
  }

  ~VersionedRandomAccessStack()
  {
    stack_.ClearEventHandlers();
  }

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(EventHandlerType const &f)
  {
    on_file_loaded_ = f;
  }

  void OnBeforeFlush(EventHandlerType const &f)
  {
    on_before_flush_ = f;
  }

  void SignalFileLoaded()
  {
    if (on_file_loaded_)
    {
      on_file_loaded_();
    }
  }

  void SignalBeforeFlush()
  {
    if (on_before_flush_)
    {
      on_before_flush_();
    }
  }

  /**
   * Indicate whether the stack is writing directly to disk or caching writes.
   *
   * @return: Whether the stack is written straight to disk.
   */
  static constexpr bool DirectWrite()
  {
    return StackType::DirectWrite();
  }

  void Load(std::string const &filename, std::string const &history,
            bool const &create_if_not_exist = true)
  {
    stack_.Load(filename, create_if_not_exist);
    history_.Load(history, create_if_not_exist);
    bookmark_ = stack_.header_extra().bookmark;
  }

  void New(std::string const &filename, std::string const &history)
  {
    stack_.New(filename);
    history_.New(history);
    bookmark_ = stack_.header_extra().bookmark;
  }

  void Clear()
  {
    stack_.Clear();
    history_.Clear();
    ResetBookmark();
    bookmark_ = stack_.header_extra().bookmark;
  }

  type Get(std::size_t i) const
  {
    type object;
    stack_.Get(i, object);
    return object;
  }

  void Get(std::size_t i, type &object) const
  {
    stack_.Get(i, object);
  }

  void Set(std::size_t i, type const &object)
  {
    type old_data;
    stack_.Get(i, old_data);
    history_.Push(HistorySet{i, old_data}, HistorySet::value);
    stack_.Set(i, object);
  }

  uint64_t Push(type const &object)
  {
    history_.Push(HistoryPush{}, HistoryPush::value);
    return stack_.Push(object);
  }

  void Pop()
  {
    type old_data = stack_.Top();
    history_.Push(HistoryPop{old_data}, HistoryPop::value);
    stack_.Pop();
  }

  type Top() const
  {
    return stack_.Top();
  }

  void Swap(std::size_t i, std::size_t j)
  {
    history_.Push(HistorySwap{i, j}, HistorySwap::value);
    stack_.Swap(i, j);
  }

  /**
   * Revert the main stack to the point at bookmark b by continually popping off changes from the
   * history, inspecting their type, and applying a revert with that change
   *
   * @param: b The bookmark to revert to
   *
   */
  void Revert(BookmarkType const &b)
  {
    while ((!empty()) && (b != bookmark_))
    {
      if (!history_.empty())
      {

        uint64_t t = history_.Type();

        switch (t)
        {
        case HistoryBookmark::value:
          RevertBookmark();
          if (bookmark_ != b)
          {
            history_.Pop();
          }

          break;
        case HistorySwap::value:
          RevertSwap();
          history_.Pop();
          break;
        case HistoryPop::value:
          RevertPop();
          history_.Pop();
          break;
        case HistoryPush::value:

          RevertPush();
          history_.Pop();
          break;
        case HistorySet::value:

          RevertSet();
          history_.Pop();
          break;
        case HistoryHeader::value:

          RevertHeader();
          history_.Pop();
          break;
        default:
          throw StorageException("Undefined type found when reverting in versioned history");
        }
      }
    }

    HeaderType h = stack_.header_extra();
    h.bookmark   = bookmark_;
    stack_.SetExtraHeader(h);

    NextBookmark();
  }

  void SetExtraHeader(HeaderExtraType const &b)
  {
    HeaderType h = stack_.header_extra();
    history_.Push(HistoryHeader{h.header}, HistoryHeader::value);

    h.header = b;
    stack_.SetExtraHeader(h);
  }

  HeaderExtraType const &header_extra() const
  {
    return stack_.header_extra().header;
  }

  BookmarkType Commit()
  {
    BookmarkType b = bookmark_;

    return Commit(b);
  }

  BookmarkType Commit(BookmarkType const &b)
  {
    // The flush here is vitally important since we must ensure the all flush handlers successfully
    // execute. Failure to do this results in an incorrectly ordered difference / history stack
    // which in turn means that the state can not be reverted
    Flush(false);

    history_.Push(HistoryBookmark{b}, HistoryBookmark::value);

    HeaderType h = stack_.header_extra();
    h.bookmark = bookmark_ = b;
    stack_.SetExtraHeader(h);
    NextBookmark();
    return b;
  }

  void Flush(bool lazy = true)
  {
    stack_.Flush(lazy);
  }

  void ResetBookmark()
  {
    bookmark_ = 0;
  }

  void NextBookmark()
  {
    ++bookmark_;
  }

  void PreviousBookmark()
  {
    ++bookmark_;
  }

  std::size_t size() const
  {
    return stack_.size();
  }

  std::size_t empty() const
  {
    return stack_.empty();
  }

  bool is_open() const
  {
    return stack_.is_open();
  }

private:
  VariantStack history_;
  BookmarkType bookmark_;

  EventHandlerType on_file_loaded_;
  EventHandlerType on_before_flush_;

  StackType stack_;

  void RevertBookmark()
  {

    HistoryBookmark book;
    history_.Top(book);
    bookmark_ = book.bookmark;
  }

  void RevertSwap()
  {
    HistorySwap swap;
    history_.Top(swap);
    stack_.Swap(swap.i, swap.j);
  }

  void RevertPop()
  {
    HistoryPop pop;
    history_.Top(pop);
    stack_.Push(pop.data);
  }

  void RevertPush()
  {
    HistoryPush push;
    history_.Top(push);
    stack_.Pop();
  }

  void RevertSet()
  {
    HistorySet set;
    history_.Top(set);
    stack_.Set(set.i, set.data);
  }

  void RevertHeader()
  {
    HistoryHeader header;
    history_.Top(header);

    HeaderType h = stack_.header_extra();

    h.header = header.data;
    stack_.SetExtraHeader(h);
    assert(this->header_extra() == h.header);
  }
};

}  // namespace storage
}  // namespace fetch
