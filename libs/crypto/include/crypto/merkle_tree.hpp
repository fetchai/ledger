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

#include "core/byte_array/const_byte_array.hpp"

#include <vector>

namespace fetch {
namespace crypto {

class MerkleTree
{
public:
  using Digest        = byte_array::ConstByteArray;
  using Container     = std::vector<Digest>;
  using Iterator      = Container::iterator;
  using ConstIterator = Container::const_iterator;

  explicit MerkleTree(std::size_t count);
  MerkleTree(MerkleTree const &rhs) = delete;
  MerkleTree(MerkleTree &&rhs)      = default;
  ~MerkleTree()                     = default;
  MerkleTree &operator=(MerkleTree const &rhs) = default;
  MerkleTree &operator=(MerkleTree &&rhs) = default;

  /// @name Leaf Node Access
  /// @{
  Iterator      begin();
  ConstIterator begin() const;
  ConstIterator cbegin();
  Iterator      end();
  ConstIterator end() const;
  ConstIterator cend() const;
  /// @}

  Digest const &   root() const;
  Container const &leaf_nodes() const;
  std::size_t      size() const;

  void CalculateRoot() const;

  Digest &operator[](std::size_t n);

private:
  Container      leaf_nodes_;
  mutable Digest root_;

  template <typename T>
  friend void Serialize(T &serializer, MerkleTree const &);

  template <typename T>
  friend void Deserialize(T &serializer, MerkleTree &b);
};

inline MerkleTree::Digest const &MerkleTree::root() const
{
  return root_;
}

inline MerkleTree::Container const &MerkleTree::leaf_nodes() const
{
  return leaf_nodes_;
}

inline std::size_t MerkleTree::size() const
{
  return leaf_nodes_.size();
}

inline MerkleTree::Iterator MerkleTree::begin()
{
  return leaf_nodes_.begin();
}

inline MerkleTree::ConstIterator MerkleTree::begin() const
{
  return leaf_nodes_.begin();
}

inline MerkleTree::ConstIterator MerkleTree::cbegin()
{
  return leaf_nodes_.cbegin();
}

inline MerkleTree::Iterator MerkleTree::end()
{
  return leaf_nodes_.end();
}

inline MerkleTree::ConstIterator MerkleTree::end() const
{
  return leaf_nodes_.end();
}

inline MerkleTree::ConstIterator MerkleTree::cend() const
{
  return leaf_nodes_.cend();
}

template <typename T>
void Serialize(T &serializer, MerkleTree const &tree)
{
  serializer << tree.leaf_nodes_ << tree.root_;
}

template <typename T>
void Deserialize(T &serializer, MerkleTree &tree)
{
  serializer >> tree.leaf_nodes_ >> tree.root_;
}

}  // namespace crypto
}  // namespace fetch
