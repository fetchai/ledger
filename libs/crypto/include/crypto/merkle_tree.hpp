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

#include "core/byte_array/byte_array.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace crypto {

class MerkleTree
{
public:
  using HashArray = byte_array::ConstByteArray;
  using Container = std::map<HashArray, HashArray>;

  MerkleTree();
  MerkleTree(MerkleTree const &rhs);
  MerkleTree(MerkleTree &&rhs);
  MerkleTree &operator=(MerkleTree const &rhs);
  MerkleTree &operator=(MerkleTree &&rhs);

  HashArray &operator[](std::size_t const &n);
  HashArray &operator[](HashArray const &n);

  void             CalculateRoot() const;
  HashArray const &root() const;
  Container const &leaf_nodes() const;

private:
  Container         leaf_nodes_;
  mutable HashArray root_;

  template <typename T>
  friend inline void Serialize(T &serializer, MerkleTree const &);

  template <typename T>
  friend inline void Deserialize(T &serializer, MerkleTree &b);
};

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
