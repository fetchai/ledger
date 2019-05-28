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

#include "crypto/sha256.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/key.hpp"
#include "storage/new_versioned_random_access_stack.hpp"
#include "storage/random_access_stack.hpp"
#include "storage/storage_exception.hpp"
#include "storage/versioned_random_access_stack.hpp"

#include <cstring>
#include <deque>
#include <queue>
#include <set>

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
template <std::size_t S = 256, std::size_t N = 32>
struct KeyValuePair
{
  using HashFunction = crypto::SHA256;
  static_assert(N == HashFunction::size_in_bytes(), "Hash size must match the hash function");

  using KeyType   = Key<S>;
  using IndexType = uint64_t;

  static constexpr IndexType TREE_ROOT_VALUE{~IndexType{0}};

  KeyType key;
  uint8_t hash[N]{};

  // The location in bits of the distance down the key this node splits on
  uint16_t split = 0;

  // Ref to parent, left and right branches
  IndexType parent = TREE_ROOT_VALUE;

  union
  {
    IndexType value = 0;
    IndexType left;
  };
  IndexType right = 0;

  bool operator==(KeyValuePair const &kv) const
  {
    return Hash() == kv.Hash();
  }

  bool operator!=(KeyValuePair const &kv) const
  {
    return Hash() != kv.Hash();
  }

  bool is_leaf() const
  {
    return split == S;
  }

  bool UpdateLeaf(uint64_t const &val, byte_array::ConstByteArray const &data)
  {
    memcpy(hash, data.pointer(), N);
    value = val;

    return true;
  }

  bool UpdateNode(KeyValuePair const &left, KeyValuePair const &right)
  {
    HashFunction hasher;
    hasher.Reset();

    hasher.Update(right.hash, N);
    hasher.Update(left.hash, N);
    hasher.Final(hash, N);

    return true;
  }

  byte_array::ByteArray Hash() const
  {
    return {hash, N};
  }
};

/**
 * Allows users to store, retrieve and create key value pairs. Byte arrays are used for the key and
 * must be the correct size. This is written to file.
 *
 * The kvi is versioned, so it includes the functionality to revert to a previous state
 */
template <typename KV = KeyValuePair<>, typename D = VersionedRandomAccessStack<KV>>
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
  using stack_type     = D;
  using key_value_pair = KeyValuePair<>;
  using index_type     = key_value_pair::IndexType;
  using key_type       = typename key_value_pair::KeyType;

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

  void Delete(byte_array::ConstByteArray const & /*key*/)
  {
    throw StorageException("Not implemented");
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
  void Set(byte_array::ConstByteArray const &key_str, uint64_t const &val,
           byte_array::ConstByteArray const &data)
  {
    DebugVerify();

    key_type       key(key_str);
    bool           split;
    int            pos;
    key_value_pair kv;
    int            left_right;

    index_type depth;
    index_type index = FindNearest(key, kv, split, pos, left_right, depth);

    bool update_parent = false;

    // Case where the 'nearest' is the root of the tree
    if (index == key_value_pair::TREE_ROOT_VALUE)
    {
      kv.key        = key;
      kv.parent     = key_value_pair::TREE_ROOT_VALUE;
      kv.split      = uint16_t{key.size_in_bits()};
      update_parent = kv.UpdateLeaf(val, data);

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
        left.split = uint16_t(key.size_in_bits());

        left.parent  = stack_.size() + 1;
        right.parent = stack_.size() + 1;

        update_parent = left.UpdateLeaf(val, data);

        lid = stack_.Push(left);
        stack_.Set(rid, right);
        break;
      case 1:
        cid = lid = index;
        left      = kv;

        pid = left.parent;

        right.key   = key;
        right.split = uint16_t(key.size_in_bits());

        right.parent = stack_.size() + 1;
        left.parent  = stack_.size() + 1;

        update_parent = right.UpdateLeaf(val, data);

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
      update_parent = kv.UpdateLeaf(val, data);
      stack_.Set(uint64_t(index), kv);
    }

    // Depending on whether the underlying stack is caching or not, we write to it or defer writing
    // to it by scheduling updates until the next flush
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

    DebugVerify();
    DebugVerifyMerkle();
  }

  byte_array::ByteArray Hash()
  {
    stack_.Flush();
    key_value_pair kv;
    if (stack_.size() > 0)
    {
      stack_.Get(root_, kv);
    }

    DebugVerifyMerkle();

    return kv.Hash();
  }

  stack_type &underlying_stack()
  {
    return stack_;
  }

  std::size_t size() const
  {
    return (stack_.size() + 1) / 2;
  }

  void Flush(bool lazy = true)
  {
    stack_.Flush(lazy);
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

  // TODO(HUT): this will be removed when updating the versioned stack
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

  //*/

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

  self_type::Iterator GetSubtree(byte_array::ConstByteArray const &key_str, uint64_t max_bits)
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

    FindNearest(key, kv, split, pos, left_right, depth, max_bits);

    // TODO(issue 874): Compare(...) call bellow seems to be unnecessary/redundant,
    // thus it was commented out.
    // pos = 0;
    // kv.key.Compare(key_str, pos, key.size_in_bits());

    if (uint64_t(pos) < max_bits)
    {
      return end();
    }

    return Iterator(this, kv, true);
  }

  /**
   * Erase the element from the tree. This will involve reversing an insertion, that is, deleting
   * the leaf and its parent. The leaf's sibling node can then be joined to that deleted parent's
   * parent.
   *
   * Note: the hashes of the tree must be recalculated in this instance, since deletion is a costly
   * operation anyway we do not schedule hash rewrites.
   *
   * The way this deletion is achieved efficiently is that the node to be deleted switches its
   * location in the stack to the end. It can then be easily popped off
   */
  void Erase(byte_array::ConstByteArray const &key_str)
  {
    static int times_erased = 0;
    times_erased++;

    DebugVerify();

    Flush(false);

    DebugVerify();

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
      // Note: this must be an operation that is recorded in the case of a revertible underlying
      // store.
      stack_.Pop();
      root_ = 0;
      stack_.SetExtraHeader(root_);

      DebugVerify();
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

    // DebugVerify();

    //// Erase our node and its parent, important to do this at the end since it might shuffle
    /// indexes
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

    DebugVerify();
    DebugVerifyMerkle();

    // It's now important to update the merkle tree from the deleted node's sibling upwards
    // UpdateParents(kv.parent, index, kv)
  }

  void UpdateVariables()
  {
    root_ = stack_.header_extra();
  }

private:
  stack_type                  stack_;
  std::vector<key_value_pair> debug_stack_;

  uint64_t                                     root_ = 0;
  std::unordered_map<uint64_t, key_value_pair> schedule_update_;

  /**
   * Update the parents of a changed node, since this changes the merkle tree
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
                         int &left_right, uint64_t &depth, uint64_t max_bits = key_type::BITS)
  {
    depth = 0;
    if (this->empty())
    {
      return key_value_pair::TREE_ROOT_VALUE;
    }

    index_type next = root_;
    index_type index;
    do
    {
      ++depth;
      index = next;

      pos = int(key.size_in_bits());

      stack_.Get(next, kv);

      left_right = key.Compare(kv.key, pos, kv.split);

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

    // Write last element into index location
    stack_.Set(index, last_element);

    // We have now changed the last element location in the stack. Update children's
    // parents
    if (!last_element.is_leaf())
    {
      key_value_pair kv;

      stack_.Get(last_element.left, kv);
      kv.parent = index;
      stack_.Set(last_element.left, kv);

      stack_.Get(last_element.right, kv);
      kv.parent = index;
      stack_.Set(last_element.right, kv);
    }

    // Finally pop stack - last element is invalid
    stack_.Pop();
  }

  void LoadDebugStack()
  {
    debug_stack_.clear();
    for (std::size_t i = 0; i < stack_.size(); ++i)
    {
      key_value_pair kv;
      stack_.Get(i, kv);
      debug_stack_.push_back(kv);
    }
  }

  void DebugVerify()
  {
    LoadDebugStack();

    if (stack_.size() == 0)
    {
      return;
    }

    if (root_ >= stack_.size())
    {
      throw StorageException("Root out of bounds of stack");
    }

    // To verfy, traverse the whole tree, keeping track of all of the nodes passed
    // to make sure it's the same size as the underlying stack
    std::vector<uint64_t> nodes_stack{root_};
    std::vector<uint64_t> verified_so_far{0};
    std::set<uint64_t>    leaf_values;
    key_value_pair        kv;
    uint64_t              nodes_found  = 0;
    uint64_t              leaves_found = 0;

    // Check root is terminated correctly
    stack_.Get(root_, kv);
    if (kv.parent != uint64_t(-1))
    {
      throw StorageException("Root of tree's parent not terminated correctly");
    }

    // To verify, do a depth first search of the tree
    while (nodes_stack.size() > 0)
    {
      // Get node of stack
      auto &stack_end = nodes_stack.back();
      stack_.Get(stack_end, kv);

      // Verify this node is correct
      if (kv.is_leaf())
      {
        // Note: the key value index shouldn't really know that 0 is invalid, but it is.
        if (kv.value == 0 || kv.value == uint64_t(-1))
        {
          throw StorageException("leaf key in key value index is malformed");
        }

        if (leaf_values.find(kv.value) != leaf_values.end())
        {
          throw StorageException("Duplicate values found in key value index! " +
                                 std::to_string(kv.value));
        }
      }
      else
      {
        if (kv.left == kv.right || kv.left == uint64_t(-1) || kv.right == uint64_t(-1))
        {
          throw StorageException("key in key value index is malformed");
        }
      }

      // Verified keeps track of whether the left/right side of the tree has been verified already
      switch (verified_so_far.back())
      {
      // Node hasn't been seen before
      case 0:
        nodes_found++;

        if (!kv.is_leaf())
        {
          nodes_stack.push_back(kv.left);
          verified_so_far[verified_so_far.size() - 1]++;
          verified_so_far.push_back(0);
        }
        else
        {
          leaves_found++;
          nodes_stack.pop_back();
          verified_so_far.pop_back();
        }
        break;

      // Previously node went left
      case 1:
        nodes_stack.push_back(kv.right);
        verified_so_far[verified_so_far.size() - 1]++;
        verified_so_far.push_back(0);
        break;

      // Node went right last time - go upwards
      case 2:
        nodes_stack.pop_back();
        verified_so_far.pop_back();
        break;

      default:
        throw StorageException("Found unexpected value in verified stack");
      }
    }

    // verify size meets expectations
    if (nodes_found != stack_.size())
    {
      throw StorageException("Stack size mismatch");
    }

    if (leaves_found != size())
    {
      throw StorageException("Calculated leaves found mismatch");
    }

    auto res = stack_.size();

    if (!(res & 0x1))
    {
      throw StorageException(
          "In the key value index trie, there should always be an odd number of nodes");
    }
  }

  void DebugVerifyMerkle()
  {
    if (stack_.size() == 0)
    {
      return;
    }

    if (root_ >= stack_.size())
    {
      throw StorageException("Root out of bounds of stack");
    }

    std::vector<uint64_t> nodes_stack{root_};
    std::vector<uint64_t> verified_so_far{0};

    key_value_pair kv;
    key_value_pair kv_left;
    key_value_pair kv_right;
    key_value_pair kv_dummy;

    // To verify, do a depth first search of the tree
    while (nodes_stack.size() > 0)
    {
      // Get node of stack
      auto &stack_end = nodes_stack.back();
      stack_.Get(stack_end, kv);

      // Verify this node is correct (hash is hash of children)
      if (!(kv.is_leaf()))
      {
        // Clear dummy
        kv_dummy = key_value_pair{};

        stack_.Get(kv.left, kv_left);
        stack_.Get(kv.right, kv_right);

        kv_dummy.UpdateNode(kv_left, kv_right);

        if (kv_dummy.Hash() != kv.Hash())
        {
          throw StorageException("Merkle tree is malformed!");
        }
      }

      // Verified keeps track of whether the left/right side of the tree has been verified already
      switch (verified_so_far.back())
      {
      // Node hasn't been seen before
      case 0:

        if (!kv.is_leaf())
        {
          nodes_stack.push_back(kv.left);
          verified_so_far[verified_so_far.size() - 1]++;
          verified_so_far.push_back(0);
        }
        else
        {
          nodes_stack.pop_back();
          verified_so_far.pop_back();
        }
        break;

      // Previously node went left
      case 1:
        nodes_stack.push_back(kv.right);
        verified_so_far[verified_so_far.size() - 1]++;
        verified_so_far.push_back(0);
        break;

      // Node went right last time - go upwards
      case 2:
        nodes_stack.pop_back();
        verified_so_far.pop_back();
        break;

      default:
        throw StorageException("Found unexpected value in verified stack");
      }
    }
  }
};

}  // namespace storage
}  // namespace fetch
