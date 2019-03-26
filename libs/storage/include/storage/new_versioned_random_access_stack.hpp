#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "storage/key.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/storage_exception.hpp"
#include "storage/variant_stack.hpp"

#include "core/byte_array/encoders.hpp"

#include <cstring>


namespace fetch {
namespace storage {

// defined in key.hpp
using DefaultKey = Key<256>;

/**
 * This header is at the beginning of the main RAS and keeps track of the final bookmark in the
 * history stack
 */
struct NewBookmarkHeader
{
  uint64_t header;
  uint64_t bookmark;  // aim to remove this.
};

/**
 * NewVersionedRandomAccessStack implements a random access stack that can revert to a previous
 * state. It does this by having a random access stack, and also keeping a stack recording all of
 * the state-changing operations made to the stack (in the history). The user can place bookmarks
 * which allow reverting the stack to it's state at that point in time.
 *
 * The history is a variant stack so as to allow different operations to be saved. However note that
 * the stack itself has elements of constant width, so no dynamically allocated memory.
 *
 */
template <typename T, typename S = RandomAccessStack<T, NewBookmarkHeader>>
class NewVersionedRandomAccessStack
{
private:
  using stack_type        = S;
  using header_extra_type = uint64_t;
  using header_type       = NewBookmarkHeader;

  static constexpr char const *LOGGING_NAME = "NewVersionedRandomAccessStack";

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

    HistoryBookmark(uint64_t const &val, DefaultKey const &key_in)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
      bookmark = val;
      key      = key_in;
    }

    enum
    {
      value = 0
    };

    uint64_t   bookmark = 0;  // Internal index
    DefaultKey key{};         // User supplied key
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

    HistorySwap(uint64_t const &i_, uint64_t const &j_)
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

    HistorySet(uint64_t const &i_, T const &d)
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
   * 'extra' data that can be stored  there.
   */
  struct HistoryHeader
  {
    HistoryHeader()
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));
    }

    HistoryHeader(uint64_t const &d)
    {
      // Clear the whole structure (including padded regions) are zeroed
      memset(this, 0, sizeof(decltype(*this)));

      data = d;
    }

    enum
    {
      value = 5
    };

    uint64_t data = 0;
  };

public:
  using type               = T;
  using event_handler_type = std::function<void()>;

  NewVersionedRandomAccessStack()
  {
    stack_.OnFileLoaded([this]() { SignalFileLoaded(); });
    stack_.OnBeforeFlush([this]() { SignalBeforeFlush(); });
  }

  ~NewVersionedRandomAccessStack()
  {
    stack_.ClearEventHandlers();
  }

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f)
  {
    on_file_loaded_ = f;
  }

  void OnBeforeFlush(event_handler_type const &f)
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
    return stack_type::DirectWrite();
  }

  void Load(std::string const &filename, std::string const &history,
            bool const &create_if_not_exist = true)
  {
    stack_.Load(filename, create_if_not_exist);
    history_.Load(history, create_if_not_exist);

    std::cerr << "after history load, size is : " << history_.size() << std::endl; // DELETEME_NH
    std::cerr << "Loading: " << history << std::endl; // DELETEME_NH

    hash_history_.Load("hash_history_"+history, create_if_not_exist);
    internal_bookmark_index_ = stack_.header_extra().bookmark;
  }

  void New(std::string const &filename, std::string const &history)
  {
    stack_.New(filename);
    history_.New(history);
    hash_history_.New("hash_history_"+history);
    internal_bookmark_index_ = stack_.header_extra().bookmark;

    std::cerr << "after history new, size is : " << history_.size() << std::endl; // DELETEME_NH
    std::cerr << "New: " << history << std::endl; // DELETEME_NH
  }

  void Clear()
  {
    stack_.Clear();
    history_.Clear();
    hash_history_.Clear();

    std::cerr << "Cleared!!!" << std::endl; // DELETEME_NH
    ERROR_BACKTRACE;

    internal_bookmark_index_ = stack_.header_extra().bookmark;
  }

  type Get(std::size_t const &i) const
  {
    type object;
    stack_.Get(i, object);
    return object;
  }

  void Get(std::size_t const &i, type &object) const
  {
    stack_.Get(i, object);
  }

  void Set(std::size_t const &i, type const &object)
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

  void Swap(std::size_t const &i, std::size_t const &j)
  {
    history_.Push(HistorySwap{i, j}, HistorySwap::value);
    stack_.Swap(i, j);
  }

  void SetExtraHeader(header_extra_type const &b)
  {
    header_type h = stack_.header_extra();
    history_.Push(HistoryHeader{h.header}, HistoryHeader::value);

    h.header = b;
    stack_.SetExtraHeader(h);
  }

  header_extra_type const &header_extra() const
  {
    return stack_.header_extra().header;
  }

  uint64_t Commit(DefaultKey const &key)
  {
    // The flush here is vitally important since we must ensure the all flush handlers successfully
    // execute. Failure to do this results in an incorrectly ordered difference / history stack
    // which in turn means that the state can not be reverted
    Flush(false);

    // Create a bookmark with our key, push it to the history stack
    HistoryBookmark history_bookmark{internal_bookmark_index_, key};

    history_.Push(history_bookmark, HistoryBookmark::value);
    hash_history_.Push(history_bookmark);

    // Update our header with this information (the bookmark index)
    header_type h = stack_.header_extra();
    h.bookmark    = internal_bookmark_index_;
    stack_.SetExtraHeader(h);

    internal_bookmark_index_++;

    // Optionally flush since this is a checkpoint
    Flush(false);

    return internal_bookmark_index_ - 1;
  }

  bool HashExists(DefaultKey const & key) const
  {
    if(hash_history_.empty())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Attempted to find if hash exists, but history is empty!");
      return false;
    }

    uint64_t hash_history_size = hash_history_.size();
    HistoryBookmark book;

    for (std::size_t i = 0; i < hash_history_size; ++i)
    {
      uint64_t reverse_index = hash_history_size - i - 1;
      hash_history_.Get(reverse_index, book);

      if(book.key == key)
      {
        std::cerr << "Hash exists: return true" << std::endl; // DELETEME_NH
        return true;
      }
    }

    std::cerr << "Hash exists: return false" << std::endl; // DELETEME_NH
    return false;
  }

  /**
   * Revert the main stack to the point at bookmark b by continually popping off changes from the
   * history, inspecting their type, and applying a revert with that change. Unsafe if the key
   * doesn't exist!
   *
   * @param: b The bookmark to revert to
   *
   */
  void RevertToHash(DefaultKey const &key)
  {
    bool bookmark_found = false;

    std::cerr << "history0 : " << history_.size() << std::endl; // DELETEME_NH
    std::cerr << "history1 : " << hash_history_.size() << std::endl; // DELETEME_NH
    std::cerr << "history2 : " << stack_.size() << std::endl; // DELETEME_NH

    while (!bookmark_found)
    {
      if (history_.empty())
      {
        throw StorageException(
            "Attempt to revert to key failed, leaving stack in undefined state.");
      }

      // Find the type of the top of the history
      uint64_t t = history_.Type();

      switch (t)
      {
      case HistoryBookmark::value:
        bookmark_found = RevertBookmark(key);
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
      case HistoryHeader::value:
        RevertHeader();
        break;
      default:
        throw StorageException("Undefined type found when reverting in versioned history");
      }
    }
  }

  void Flush(bool lazy = true)
  {
    // Note that the variant stack (history) does not need flushing
    stack_.Flush(lazy);
    history_.Flush(lazy);
    hash_history_.Flush(lazy);
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
  RandomAccessStack<HistoryBookmark> hash_history_;
  uint64_t     internal_bookmark_index_{0};

  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;

  stack_type stack_;

  bool RevertBookmark(DefaultKey const &key_to_compare)
  {
    // Get bookmark from history
    HistoryBookmark book;
    history_.Top(book);

    internal_bookmark_index_ = book.bookmark;

    // Update header
    header_type h = stack_.header_extra();
    h.bookmark    = internal_bookmark_index_;
    stack_.SetExtraHeader(h);

    // If we are reverting to a state, we want this bookmark to stay - this
    // will make reverting to the same hash twice in a row valid.
    if (!(key_to_compare == book.key))
    {
      std::cerr << "reverting!" << std::endl; // DELETEME_NH
      history_.Pop();

      // Sanity check, the hash history matches
      if(!(hash_history_.Top().key == book.key) || hash_history_.size() == 0)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Hash history top does not match bookmark being removed!");
      }

      hash_history_.Pop();
    }
    {
      std::cerr << "popping" << std::endl; // DELETEME_NH
    }

    std::cerr << "reverting bookmark! key: " << byte_array::ToHumanReadable(key_to_compare.ToByteArray()) << std::endl; // DELETEME_NH
    std::cerr << "reverting bookmark! book: " << byte_array::ToHumanReadable(book.key.ToByteArray()) << std::endl; // DELETEME_NH
    std::cerr << "reverting bookmark! this: " << this << std::endl; // DELETEME_NH

    return key_to_compare == book.key;
  }

  void RevertSwap()
  {
    HistorySwap swap;
    history_.Top(swap);
    stack_.Swap(swap.i, swap.j);
    history_.Pop();
  }

  void RevertPop()
  {
    HistoryPop pop;
    history_.Top(pop);
    stack_.Push(pop.data);
    history_.Pop();
  }

  void RevertPush()
  {
    HistoryPush push;
    history_.Top(push);
    stack_.Pop();
    history_.Pop();
  }

  void RevertSet()
  {
    HistorySet set;
    history_.Top(set);
    stack_.Set(set.i, set.data);
    history_.Pop();
  }

  void RevertHeader()
  {
    HistoryHeader header;
    history_.Top(header);

    header_type h = stack_.header_extra();

    h.header = header.data;
    stack_.SetExtraHeader(h);
    assert(this->header_extra() == h.header);
    history_.Pop();
  }
};

}  // namespace storage
}  // namespace fetch
