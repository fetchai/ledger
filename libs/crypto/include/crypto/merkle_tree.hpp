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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serialisers/group_definitions.hpp"
#include "core/serialisers/main_serialiser.hpp"
#include "core/serialisers/map_serialiser_boilerplate.hpp"

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

  template <typename D>
  struct MapSerialiser;

private:
  Container      leaf_nodes_;
  mutable Digest root_;

  template <uint8_t KEY, class MemberVariable, MemberVariable MEMBER_VARIABLE>
  friend struct SerialisedStructField;
};

template <typename D>
struct MerkleTree::MapSerialiser
  : serialisers::MapSerialiserBoilerplate<MerkleTree, D,
                                          serialiseD_STRUCT_FIELD(1, MerkleTree::leaf_nodes_),
                                          serialiseD_STRUCT_FIELD(2, MerkleTree::root_)>
{
};

}  // namespace crypto

namespace serialisers {

template <typename D>
struct MapSerialiser<crypto::MerkleTree, D> : crypto::MerkleTree::MapSerialiser<D>
{
};

}  // namespace serialisers
}  // namespace fetch
