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
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
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

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
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

}  // namespace crypto

namespace serializers {

template <typename D>
struct MapSerializer<crypto::MerkleTree, D>
{
public:
  using Type       = crypto::MerkleTree;
  using DriverType = D;

  static constexpr uint8_t LEAF_NODES = 1;
  static constexpr uint8_t ROOT       = 2;

  template <typename Constructor>
  static inline void Serialize(Constructor &map_constructor, Type const &data)
  {
    auto map = map_constructor(2);
    map.Append(LEAF_NODES, data.leaf_nodes_);
    map.Append(ROOT, data.root_);
  }

  template <typename MapDeserializer>
  static inline void Deserialize(MapDeserializer &map, Type &data)
  {
    map.ExpectKeyGetValue(LEAF_NODES, data.leaf_nodes_);
    map.ExpectKeyGetValue(ROOT, data.root_);
  }
};

}  // namespace serializers
}  // namespace fetch
