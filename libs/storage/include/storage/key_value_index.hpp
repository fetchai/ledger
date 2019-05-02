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

//                                 .─────────.
//                               ,'     ''     .
//                              (    split: 0   )
//                               '─.         ,─'
//                                  `───────'
//                                      │
//                              ┌───────┴──────────┐
//                        .─────▼───.        .─────▼───.
//                      ,'     0      .    ,'     1      .
//                     (    split: 1   )  (    split: 1   )
//                      '─            '    '─            '
//                         `───┬┬──'          `────┬──'
//                ┌────────────┘└───┐              └──┬─────────────────┐
//                │                 │                 │                 │
//          .─────▼───.       ******▼****       .─────▼───.       ******▼****
//        ,'    000     .   ** 0111000... *   ,'     10     .   ** 1110011... *
//       (    split: 3   ) *   split: 256  * (    split: 2   ) *   split: 256  *
//        '─            '   **            *   '─            '   **            *
//           `────┬──'         *********         `────┬──'         *********
//                │                                   │
//              ┌─┴───────────────┐                 ┌─┴───────────────┐
//              │                 │                 │                 │
//        ******▼****       ******▼****       ******▼****       .─────▼───.
//      ** 0000010... *   ** 0001101... *   **  1001110   *   ,'    101     .
//     *   split: 256  * *   split: 256  * *   split: 256  * (    split: 3   )
//      **            *   **            *   **            *   '─            '
//         *********         *********         *********         `────┬──'
//                                                ┌─────────────────┬─┘
//                                                │                 │
//                                          ******▼****       ******▼****
//                                        ** 1010100... *   **  1011011   *
//                                       *   split: 256  * *   split: 256  *
//                                        **            *   **            *
//                                           *********         *********
//
//
// Representation of a possible configuration of the key value trie. When the split is maximal
// (256), this represents that the node is a leaf. The nodes can contain additional information
//
// Nodes in the tree above show their key and the bit the node splits on. Leaves are marked with '*'
// and match the full key.

#include "crypto/sha256.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/key.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/storage_exception.hpp"
#include "storage/versioned_random_access_stack.hpp"

#include <cstring>
#include <deque>
#include <queue>

namespace fetch {
namespace storage {

/**
 * Key value pair for binary tries where the key is a byte array. The tree can
 * be traversed given a key by switching on the split until the leaf or its nearest equivalent is
 * found.
 *
 * The parent of the tree will be identifiable with 0xffffffff.
 *
 * Additional information held by the KeyValuePair is value and hash. The hashes of each KV pair
 * include their children's hashes, ie. merkle tree which can be used to detect file corruption.
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

  bool operator==(KeyValuePair const &kv)
  {
    return Hash() == kv.Hash();
  }

  bool operator!=(KeyValuePair const &kv)
  {
    return Hash() != kv.Hash();
  }

  bool is_leaf() const
  {
    return split == S;
  }

  bool UpdateLeaf(uint64_t const &val, byte_array::ConstByteArray const &data)
  {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update(data);
    hasher.Final(hash, N);
    value = val;

    return true;
  }

  bool UpdateNode(KeyValuePair const &left, KeyValuePair const &right)
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(left.hash, N);
    hasher.Update(right.hash, N);
    hasher.Final(hash, N);

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

/**
 * Allows users to store, retrieve and create key value pairs. Byte arrays are used for the key and
 * must be the correct size. This is written to file.
 *
 * The kvi is versioned, so it includes the functionality to revert to a previous state
 */
template <typename KV = KeyValuePair<>, typename D = VersionedRandomAccessStack<KV, uint64_t>>
class KeyValueIndex
{
  /**
   * Used to keep track of the ordering of task priority, in this usage writing to the stack.
   */
  struct UpdateTask
  {
    uint64_t priority;
    uint64_t element;

    bool operator==(UpdateTask const &other) const
    {
      return (priority == other.priority) && (other.element == element);
    }
    bool operator<(UpdateTask const &other) const
    {
      return priority < other.priority;
    }
  };

public:
  using self_type      = KeyValueIndex<KV, D>;
  using index_type     = uint64_t;
  using stack_type     = D;
  using key_value_pair = KeyValuePair<>;
  using key_type       = typename key_value_pair::key_type;

  static constexpr char const *LOGGING_NAME = "KeyValueIndex";

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

    root_ = 0;
    stack_.SetExtraHeader(root_);
  }

  template <typename... Args>
  void Load(Args &&... args)
  {
    stack_.Load(std::forward<Args>(args)...);
  }

  void BeforeFlushHandler()
  {
    if (!this->is_open())
    {
      return;
    }

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
        if (depths.find(last) != depths.end())
        {
          break;
        }
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

  /**
   * Add a new key, creating a key and rearranging the tree if it does not exist already.
   *
   * @param: key_str The key
   * @param: args The associated information with the key
   *
   */
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

    // Case where the tree is empty
    if (index == index_type(-1))
    {
      kv.key        = key;
      kv.parent     = uint64_t(-1);
      kv.split      = uint16_t(key.size());
      update_parent = kv.UpdateLeaf(args...);

      index = stack_.Push(kv);
    }
    // Case where the nearest node is not a leaf, in that case the tree must be rearranged at that
    // split
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
    // Case where we are overwriting a leaf that already exists
    else
    {
      update_parent = kv.UpdateLeaf(args...);
      stack_.Set(uint64_t(index), kv);
    }

    // Depending on whether the underlying stack is caching or not, we write to it or defer writing
    // to it by scheduling updates until the next flush since changes to a leaf will propagate the
    // hash recalculation all the way to the root.
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

  /**
   * Erase the element from the tree. This will involve reversing an insertion, that is, deleting
   * the leaf and its parent. The leaf's sibling node can then be joined to that deleted parent's
   * parent.
   *
   * Note: the hashes of the tree must be recalculated in this instance, since deletion is a costly
   * operation anyway we do not schedule hash rewrites.
   */
  void Erase(byte_array::ConstByteArray const &key_str)
  {
    Flush();

    Verify(true);

    if (size() == 0)
    {
      return;
    }

    // First find the leaf we wish to delete
    key_type       key(key_str);
    bool           split      = true;
    int            pos        = 0;
    int            left_right = 0;
    index_type     depth      = 0;
    key_value_pair kv;

    index_type kv_index = FindNearest(key, kv, split, pos, left_right, depth);

    // Leaf not found
    if (split)
    {
      return;
    }

    assert(kv.is_leaf() == true);

    // Clear the tree for edge case of only node
    if (size() == 1)
    {
      stack_.Clear();
      root_ = 0;
      stack_.SetExtraHeader(root_);
      return;
    }

    // Get our sibling, and our parent
    key_value_pair parent;
    key_value_pair sibling;
    index_type     parent_index = kv.parent;
    index_type     sibling_index;
    stack_.Get(parent_index, parent);

    assert(parent_index != uint64_t(-1));

    // Determine the sibling left/right from parent left/right
    if (kv_index == parent.left)
    {
      sibling_index = parent.right;
      stack_.Get(sibling_index, sibling);
    }
    else if (kv_index == parent.right)
    {
      sibling_index = parent.left;
      stack_.Get(sibling_index, sibling);
    }
    else
    {
      throw StorageException("Failed to find element in parent left/right references: tree broken");
    }

    // Set the sibling in the place of that parent, the split etc. should be able to stay the same
    sibling.parent = parent.parent;

    // Update that sibling's parents if sibling is not root
    if (sibling.parent != uint64_t(-1))
    {
      key_value_pair new_sibling_parent;
      stack_.Get(sibling.parent, new_sibling_parent);

      // Update parent left/right
      if (new_sibling_parent.left == parent_index)
      {
        new_sibling_parent.left = sibling_index;
      }
      else if (new_sibling_parent.right == parent_index)
      {
        new_sibling_parent.right = sibling_index;
      }
      else
      {
        throw StorageException("Failed to erase element in key value index: tree broken");
      }

      stack_.Set(sibling.parent, new_sibling_parent);
    }
    // If sibling is root, need to update
    else
    {
      root_ = sibling_index;
    }

    stack_.Set(sibling_index, sibling);
    UpdateParents(sibling.parent, sibling_index, sibling);

    Verify(true);

    // Erase our node and its parent, important to do this at the end since it might shuffle indexes
    if (parent_index > kv_index)
    {
      Erase(parent_index);
      Erase(kv_index);
    }
    else
    {
      Erase(kv_index);
      Erase(parent_index);
    }

    Verify(true);
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

  /**
   * Calculate number of leaves from stack size (contains all nodes). The tree is complete so is
   * calculable. Leaves + (Leaves - 1) = Nodes
   *
   * Note this should always be correct even with the key value index scheduling writes to the stack
   * since these scheduled operations are never to change the stack size.
   *
   * @return: The number of elements in the key value index
   */
  std::size_t size()
  {
    assert(stack_.size() + 1 != 0);
    assert((stack_.size() + 1) % 2 == 0);
    return (stack_.size() + 1) / 2;
  }

  void Flush()
  {
    stack_.Flush();
  }

  bool is_open() const
  {
    return stack_.is_open();
  }

  bool empty() const
  {
    return stack_.empty();
  }

  void Close()
  {
    stack_.Close();
  }

  using bookmark_type = uint64_t;
  bookmark_type Commit()
  {
    return stack_.Commit();
  }

  bookmark_type Commit(bookmark_type const &b)
  {
    return stack_.Commit(b);
  }

  void Revert(bookmark_type const &b)
  {
    stack_.Revert(b);

    root_ = stack_.header_extra();
  }

  uint64_t const &root_element() const
  {
    return root_;
  }

  class Iterator
  {
  public:
    Iterator(self_type *self, key_value_pair kv, bool node_iterator = false)
      : kv_{kv}
      , kv_node_{kv}
      , node_iterator_{node_iterator}
      , self_{self}
    {
      if (node_iterator)
      {
        self->GetLeftLeaf(kv_);
      }
    }

    Iterator()                    = default;
    Iterator(Iterator const &rhs) = default;
    Iterator(Iterator &&rhs)      = default;
    Iterator &operator=(Iterator const &rhs) = default;
    Iterator &operator=(Iterator &&rhs) = default;

    bool operator==(Iterator const &rhs) const
    {
      return kv_ == rhs.kv_;
    }

    bool operator!=(Iterator const &rhs) const
    {
      return !(kv_ == rhs.kv_);
    }

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

  self_type::Iterator begin()
  {
    if (this->empty())
    {
      return end();
    }

    key_value_pair kv;
    stack_.Get(root_, kv);

    GetLeftLeaf(kv);

    assert(Iterator(this, kv) != end());

    return Iterator(this, kv);
  }

  self_type::Iterator end()
  {
    return Iterator(this, key_value_pair());
  }

  self_type::Iterator Find(byte_array::ConstByteArray const &key_str)
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

    return Iterator(this, kv);
  }

  self_type::Iterator GetSubtree(byte_array::ConstByteArray const &key_str, uint64_t bits)
  {
    if (this->empty())
    {
      return end();
    }

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

    return Iterator(this, kv, true);
  }

private:
  stack_type stack_;

  uint64_t                                     root_ = 0;
  std::unordered_map<uint64_t, key_value_pair> schedule_update_;

  /**
   * Recursively update the parents of a changed node, since this changes the merkle tree.
   *
   * @param: pid Address on the stack of the node's parent
   * @param: cid Address on the stack of the node
   * @param: child Changed node
   *
   */
  void UpdateParents(index_type pid, index_type cid, key_value_pair child)
  {
    key_value_pair parent, left, right;

    while (pid != index_type(-1))
    {
      stack_.Get(pid, parent);

      // Update parent hash
      {
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
      }

      stack_.Set(pid, parent);

      // Move up the tree - parent and its index can be reused as a child
      child = parent;
      cid   = pid;
      pid   = child.parent;
    }
  }

  /**
   * Find the nearest node in the trie to the key supplied
   *
   * @param: key The key to compare
   * @param: kv The key value pair to be set on success
   * @param: split Whether the returned kv splits (is a leaf)
   * @param: pos The bit location of the split if so
   * @param: left_right Whether the key would be the left or right child of the returned kv
   * @param: depth The depth of the kv in the trie
   * @param: max_bits Optional maximum number of bits to compare before terminating
   *
   * @return: the index which this kv can be found in the stack
   */
  index_type FindNearest(key_type const &key, key_value_pair &kv, bool &split, int &pos,
                         int &left_right, uint64_t &depth,
                         uint64_t max_bits = std::numeric_limits<uint64_t>::max())
  {
    depth = 0;
    if (this->empty())
    {
      return index_type(-1);
    }

    std::size_t next = root_;
    std::size_t index;
    do
    {
      ++depth;
      index = next;

      pos = int(key.size());

      stack_.Get(next, kv);

      left_right = key.Compare(kv.key, pos, kv.split >> 6, kv.split & 63);

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

  /**
   * given KV, find nearest parent we are a left branch of, AND has a right.
   * KV will be set to that node
   *
   * Optionally specify a forbidden parent
   *
   * @param: kv The key value to set. If the operation fails, no guarantees are made about its
   *            value. Otherwise, it will be set to the nearest left parent.
   *
   * @param: forbidden_parent Optional value to prevent iterations past a certain node
   *
   * @return: true if success, false otherwise
   */
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

  /**
   * Given kv, traverse down the tree's left hand branches until reaching a leaf.
   *
   * @param: kv The kv to start. Will be set to it's leftmost leaf
   */
  void GetLeftLeaf(key_value_pair &kv) const
  {
    while (!kv.is_leaf())
    {
      stack_.Get(kv.left, kv);
    }
  }

  /**
   * Get the next valid leaf by traversing the trie left to right. Forbidden parent is used to
   * constrain the iteration to never include the kv at that location. This effectively means
   * that the iteration will cover the node that has that parent and all children.
   *
   * @param: kv The key value to set. If the iteration fails, this is an empty kv. Otherwise, return
   *         the next leaf kv
   *
   * @param: forbidden_parent Optional value to prevent iterations past a certain node
   *
   */
  void GetNext(key_value_pair &kv, uint64_t forbidden_parent = uint64_t(-1))
  {
    assert(kv.is_leaf());

    // Get parent so we can check which branch we were on. Assume
    // we were on a leaf. Check we're not root.
    if (kv.parent == uint64_t(-1) || kv.parent == forbidden_parent)
    {
      kv = key_value_pair();
      return;
    }

    // We're in a binary trie, going left to right. We don't know whether we are the left or right
    // node, so we get the parent and see if we are the right node
    key_value_pair parent;
    stack_.Get(kv.parent, parent);

    key_value_pair parent_right;
    stack_.Get(parent.right, parent_right);

    // Easy iteration case where we are able to get the node right of this one
    if (parent_right != kv)
    {
      GetLeftLeaf(parent_right);
      kv = parent_right;
      GetLeftLeaf(kv);
    }
    // Case where we're the right node and the parent is root or forbidden
    else if (parent.parent == uint64_t(-1) || parent.parent == forbidden_parent)
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

  /**
   * Erase the element from the stack. Assume the element is in an undefined state. Erasure is done
   * by swapping the element with the last element of the stack and popping it off the stack.
   *
   * index must refer to a valid location on the stack.
   *
   * @param: index Index of the element to erase
   */
  void Erase(index_type const &index)
  {
    const std::size_t stack_end = stack_.size() - 1;

    assert(index <= stack_end);

    if (index == stack_end)
    {
      stack_.Pop();
      return;
    }

    // Get last element on stack
    key_value_pair last_element;
    stack_.Get(stack_end, last_element);

    // Get parent of last element, update its index to where the last element is going.
    if (last_element.parent != index_type(-1))
    {
      key_value_pair last_element_parent;
      stack_.Get(last_element.parent, last_element_parent);

      if (last_element_parent.right == stack_end)
      {
        last_element_parent.right = index;
      }
      else if (last_element_parent.left == stack_end)
      {
        last_element_parent.left = index;
      }
      else
      {
        throw StorageException(
            "Storage tree referencing broken: parent of node doesn't refer to node via left or "
            "right branches");
      }

      stack_.Set(last_element.parent, last_element_parent);
    }
    else
    {
      // Last element is root, update the root
      root_ = index;
    }

    // Write last element into index location and pop stack
    stack_.Set(index, last_element);
    stack_.Pop();
  }

  void Verify(bool stack_size_correct)
  {
    if (stack_.size() == 0)
    {
      return;
    }

    for (std::size_t i = 0; i < stack_.size(); ++i)
    {
      key_value_pair kv;
      stack_.Get(i, kv);
    }

    // VerifyHash();

    if (root_ >= stack_.size())
    {
      throw StorageException("Root out of bounds of stack");
    }

    // index_type     depth      = 0;

    key_value_pair kv;
    stack_.Get(root_, kv);

    if (kv.parent != uint64_t(-1))
    {
      throw StorageException("Root of tree's parent not terminated correctly");
    }
  }
};

}  // namespace storage
}  // namespace fetch
