#pragma once
#include "crypto/sha256.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/key.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/versioned_random_access_stack.hpp"

#include <cstring>
#include <deque>
#include <queue>

namespace fetch {
namespace storage {

/**
 * Key value pair for binary trees where the key is a byte array. The tree can
 * be traversed given
 * a key by switching on each bit of the key
 *
 * The parent of the tree will be identifiable with 0xffffffff
 *
 */
template <std::size_t S = 256, std::size_t N = 64>
struct KeyValuePair
{
  KeyValuePair()
  {
    memset(this, 0, sizeof(decltype(*this)));
    parent = uint64_t(-1);
  }

  using key_type = Key<S>;

  key_type key;
  uint8_t  hash[N];

  // The location in bits of the distance down the key this node splits on
  uint16_t split;

  // Ref to parent, left and right branches
  uint64_t parent;

  union
  {
    uint64_t value;
    uint64_t left;
  };
  uint64_t right;

  bool operator==(KeyValuePair const &kv) { return Hash() == kv.Hash(); }

  bool operator!=(KeyValuePair const &kv) { return Hash() != kv.Hash(); }

  bool is_leaf() const { return split == S; }

  bool UpdateLeaf(uint64_t const &val, byte_array::ConstByteArray const &data)
  {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update(data);
    hasher.Final(hash);
    value = val;

    return true;
  }

  bool UpdateNode(KeyValuePair const &left, KeyValuePair const &right)
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(left.hash, N);
    hasher.Update(right.hash, N);
    hasher.Final(hash);

    return true;
  }

  byte_array::ByteArray Hash() const
  {
    byte_array::ByteArray ret;
    ret.Resize(N);

    for (std::size_t i = 0; i < N; ++i)
    {
      ret[i] = hash[i];
    }

    return ret;
  }
};

template <typename KV = KeyValuePair<>, typename D = VersionedRandomAccessStack<KV, uint64_t>>
class KeyValueIndex
{
  struct UpdateTask
  {
    uint64_t priority;
    uint64_t element;

    bool operator==(UpdateTask const &other) const
    {
      return (priority == other.priority) && (other.element == element);
    }
    bool operator<(UpdateTask const &other) const { return priority < other.priority; }
  };

public:
  using self_type      = KeyValueIndex<KV, D>;
  using index_type     = uint64_t;
  using stack_type     = D;
  using key_value_pair = KeyValuePair<>;
  using key_type       = typename key_value_pair::key_type;

  KeyValueIndex()
  {
    stack_.OnFileLoaded([this]() { root_ = stack_.header_extra(); });
    stack_.OnBeforeFlush([this]() { this->BeforeFlushHandler(); });
  }

  ~KeyValueIndex()
  {
    stack_.ClearEventHandlers();
    BeforeFlushHandler();
  }

  template <typename... Args>
  void New(Args &&... args)
  {
    stack_.New(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Load(Args &&... args)
  {
    stack_.Load(std::forward<Args>(args)...);
  }

  void BeforeFlushHandler()
  {
    if (!this->is_open()) return;

    stack_.SetExtraHeader(root_);

    std::unordered_map<uint64_t, uint64_t> depths;
    std::unordered_map<uint64_t, uint64_t> parents;
    std::priority_queue<UpdateTask>        q;

    for (auto &k : schedule_update_)
    {
      auto &         kv   = k.second;
      uint64_t       last = k.first;
      uint64_t       pid  = kv.parent;
      key_value_pair parent;
      uint64_t       depth = 1;

      while (pid != uint64_t(-1))
      {
        if (parents.find(last) != parents.end())
        {
          depth += depths[last];
          break;
        }
        parents[last] = pid;

        stack_.Get(pid, parent);
        last = pid;
        pid  = parent.parent;
        ++depth;
      }

      // Adding root
      if (pid == uint64_t(-1))
      {
        parents[last] = pid;
      }

      last = k.first;
      while (parents.find(last) != parents.end())
      {
        if (depths.find(last) != depths.end()) break;
        depths[last] = depth;
        q.push({depth, last});
        --depth;
        last = parents[last];
      }
    }

    UpdateTask task;
    while (!q.empty())
    {
      task = q.top();
      q.pop();

      key_value_pair element, left, right;
      stack_.Get(task.element, element);
      if (element.is_leaf())
      {
        continue;
      }

      stack_.Get(element.left, left);
      stack_.Get(element.right, right);
      element.UpdateNode(left, right);
      stack_.Set(task.element, element);
    }

    schedule_update_.clear();
  }

  void Delete(byte_array::ConstByteArray const &key) { TODO_FAIL("Not implemented"); }

  void GetElement(uint64_t const &i, index_type &v)
  {
    key_value_pair p;
    stack_.Get(i, p);
    v = p.value;
  }

  bool GetIfExists(byte_array::ConstByteArray const &key_str, index_type &value)
  {
    key_type       key(key_str);
    bool           split      = true;
    int            pos        = 0;
    int            left_right = 0;
    index_type     depth      = 0;
    key_value_pair kv;

    FindNearest(key, kv, split, pos, left_right, depth);

    if (!split)
    {
      value = kv.value;
    }

    return !split;
  }

  index_type Get(byte_array::ConstByteArray const &key_str)
  {
    key_type       key(key_str);
    bool           split;
    int            pos;
    key_value_pair kv;
    int            left_right;
    index_type     depth;
    FindNearest(key, kv, split, pos, left_right, depth);
    assert(!split);
    return kv.value;
  }

  template <typename... Args>
  void Set(byte_array::ConstByteArray const &key_str, Args const &... args)
  {
    key_type       key(key_str);
    bool           split;
    int            pos;
    key_value_pair kv;
    int            left_right;

    index_type depth;
    index_type index = FindNearest(key, kv, split, pos, left_right, depth);

    bool update_parent = false;

    if (index == index_type(-1))
    {
      kv.key        = key;
      kv.parent     = uint64_t(-1);
      kv.split      = uint16_t(key.size());
      update_parent = kv.UpdateLeaf(args...);

      index = stack_.Push(kv);
    }
    else if (split)
    {

      key_value_pair left, right, parent;
      index_type     rid = 0, lid = 0, pid = 0, cid = 0;
      bool           update_root = (index == root_);

      switch (left_right)
      {
      case -1:
        cid = rid = index;
        right     = kv;

        pid = right.parent;

        left.key   = key;
        left.split = uint16_t(key.size());

        left.parent  = stack_.size() + 1;
        right.parent = stack_.size() + 1;

        update_parent = left.UpdateLeaf(args...);

        lid = stack_.Push(left);
        stack_.Set(rid, right);
        break;
      case 1:
        cid = lid = index;
        left      = kv;

        pid = left.parent;

        right.key   = key;
        right.split = uint16_t(key.size());

        right.parent = stack_.size() + 1;
        left.parent  = stack_.size() + 1;

        update_parent = right.UpdateLeaf(args...);

        rid = stack_.Push(right);
        stack_.Set(lid, left);
        break;
      }

      kv.split  = uint16_t(pos);
      kv.left   = lid;
      kv.right  = rid;
      kv.parent = pid;
      index     = stack_.Push(kv);

      if (update_root)
      {
        root_ = index;
      }
      else
      {
        stack_.Get(pid, parent);
        if (parent.left == cid)
        {
          parent.left = index;
        }
        else
        {
          parent.right = index;
        }
        stack_.Set(pid, parent);
      }

      switch (left_right)
      {
      case -1:
        index = kv.left;
        kv    = left;
        break;
      case 1:
        index = kv.right;
        kv    = right;
        break;
      }
    }
    else
    {
      update_parent = kv.UpdateLeaf(args...);
      stack_.Set(uint64_t(index), kv);
    }

    if ((kv.parent != index_type(-1)) && (update_parent))
    {
      if (stack_.DirectWrite())
      {
        UpdateParents(kv.parent, index, kv);
      }
      else
      {
        schedule_update_[index] = kv;
      }
    }
  }

  byte_array::ByteArray Hash()
  {
    stack_.Flush();
    key_value_pair kv;
    if (stack_.size() > 0)
    {
      stack_.Get(root_, kv);
    }

    return kv.Hash();
  }

  std::size_t size() const { return stack_.size(); }

  void Flush() { stack_.Flush(); }

  bool is_open() const { return stack_.is_open(); }

  bool empty() const { return stack_.empty(); }

  void Close() { stack_.Close(); }

  using bookmark_type = uint64_t;
  bookmark_type Commit() { return stack_.Commit(); }

  bookmark_type Commit(bookmark_type const &b) { return stack_.Commit(b); }

  void Revert(bookmark_type const &b)
  {
    stack_.Revert(b);

    root_ = stack_.header_extra();
  }

  uint64_t const &root_element() const { return root_; }

  class iterator
  {
  public:
    iterator(self_type *self, key_value_pair kv, bool node_iterator = false)
      : kv_{kv}, kv_node_{kv}, node_iterator_{node_iterator}, self_{self}
    {
      if (node_iterator)
      {
        self->GetLeftLeaf(kv_);
      }
    }

    iterator()                    = default;
    iterator(iterator const &rhs) = default;
    iterator(iterator &&rhs)      = default;
    iterator &operator=(iterator const &rhs) = default;
    iterator &operator=(iterator &&rhs) = default;

    bool operator==(iterator const &rhs) { return kv_ == rhs.kv_; }

    bool operator!=(iterator const &rhs) { return !(kv_ == rhs.kv_); }

    void operator++()
    {
      if (node_iterator_)
      {
        self_->GetNext(kv_, kv_node_.parent);
      }
      else
      {
        self_->GetNext(kv_);
      }
    }

    std::pair<byte_array::ByteArray, uint64_t> operator*() const
    {
      return std::make_pair(kv_.key.ToByteArray(), kv_.value);
    }

  protected:
    key_value_pair kv_;
    key_value_pair kv_node_;
    bool           node_iterator_ = false;
    self_type *    self_;
  };

  self_type::iterator begin()
  {
    if (this->empty()) return end();

    key_value_pair kv;
    stack_.Get(root_, kv);

    GetLeftLeaf(kv);

    assert(iterator(this, kv) != end());

    return iterator(this, kv);
  }

  self_type::iterator end() { return iterator(this, key_value_pair()); }

  // STL-like functionality
  self_type::iterator Find(byte_array::ConstByteArray const &key_str)
  {

    key_type       key(key_str);
    bool           split      = true;
    int            pos        = 0;
    int            left_right = 0;
    index_type     depth      = 0;
    key_value_pair kv;
    FindNearest(key, kv, split, pos, left_right, depth);

    if (split)
    {
      return end();
    }

    return iterator(this, kv);
  }

  self_type::iterator GetSubtree(byte_array::ConstByteArray const &key_str, uint64_t bits)
  {
    if (this->empty()) return end();

    key_type       key(key_str);
    bool           split      = true;
    int            pos        = 0;
    int            left_right = 0;
    index_type     depth      = 0;
    key_value_pair kv;

    FindNearest(key, kv, split, pos, left_right, depth, bits);

    pos = 0;
    kv.key.Compare(key_str, pos, 0, 64);

    if (uint64_t(pos) < bits)
    {
      return end();
    }

    return iterator(this, kv, true);
  }

private:
  stack_type stack_;

  uint64_t                                     root_ = 0;
  std::unordered_map<uint64_t, key_value_pair> schedule_update_;

  void UpdateParents(index_type pid, index_type cid, key_value_pair child)
  {
    key_value_pair parent, left, right;

    while (pid != index_type(-1))
    {
      stack_.Get(pid, parent);
      if (cid == parent.left)
      {
        left = child;
        stack_.Get(parent.right, right);
      }
      else
      {
        right = child;
        stack_.Get(parent.left, left);
      }

      parent.UpdateNode(left, right);
      stack_.Set(pid, parent);

      child = parent;
      cid   = pid;
      pid   = child.parent;
    }
  }

  index_type FindNearest(key_type const &key, key_value_pair &kv, bool &split, int &pos,
                         int &left_right, uint64_t &depth,
                         uint64_t max_bits = std::numeric_limits<uint64_t>::max())
  {
    depth = 0;
    if (this->empty()) return index_type(-1);

    std::size_t next = root_;
    std::size_t index;
    do
    {
      ++depth;
      index = next;

      pos = int(key.size());

      stack_.Get(next, kv);

      left_right = key.Compare(kv.key, pos, kv.split >> 8, kv.split & 63);

      switch (left_right)
      {
      case -1:
        next = kv.left;
        break;
      case 1:
        next = kv.right;
        break;
      }

    } while ((left_right != 0) && (pos >= int(kv.split)) && uint64_t(pos) < max_bits);

    split = (left_right != 0) && (pos < int(kv.split));

    return index;
  }

  // given KV, find nearest parent we are a left branch of, AND has a right
  // KV will be set to that node
  // Optionally specify a forbidden parent
  bool GetLeftParent(key_value_pair &kv, uint64_t forbidden_parent) const
  {
    assert(kv.parent != uint64_t(-1));

    if (kv.parent == forbidden_parent)
    {
      return false;
    }

    key_value_pair parent;
    key_value_pair parent_right;
    stack_.Get(kv.parent, parent);
    stack_.Get(parent.right, parent_right);

    while (kv == parent_right)
    {
      // Root condition
      if (parent.parent == uint64_t(-1) || parent.parent == forbidden_parent)
      {
        return false;
        break;
      }

      kv = parent;
      stack_.Get(parent.parent, parent);
      stack_.Get(parent.right, parent_right);
    }
    kv = parent;
    return true;
  }

  void GetLeftLeaf(key_value_pair &kv) const
  {
    while (!kv.is_leaf())
    {
      stack_.Get(kv.left, kv);
    }
  }

  void GetNext(key_value_pair &kv, uint64_t forbidden_parent = uint64_t(-1))
  {
    assert(kv.is_leaf());
    assert(kv.parent != stack_.size() + 1);

    // Get parent so we can check which branch we were on. Assume
    // we were on a leaf. Check we're not root.
    if (kv.parent == uint64_t(-1) || kv.parent == forbidden_parent)
    {
      kv = key_value_pair();
      return;
    }

    key_value_pair parent;
    stack_.Get(kv.parent, parent);

    // We're in a binary tree, going left to right
    key_value_pair parent_right;
    stack_.Get(parent.right, parent_right);

    if (parent_right != kv)
    {
      GetLeftLeaf(parent_right);
      kv = parent_right;
    }
    else if (parent.parent == uint64_t(-1))
    {
      kv = key_value_pair();
    }
    else
    {
      bool gotParent = GetLeftParent(parent, forbidden_parent);

      if (!gotParent)
      {
        kv = key_value_pair();
      }
      else
      {
        // Switch to rhs branch since we travelled up to find a node we were the
        // left of
        stack_.Get(parent.right, parent);

        GetLeftLeaf(parent);
        kv = parent;
      }
    }
  }
};

}  // namespace storage
}  // namespace fetch
