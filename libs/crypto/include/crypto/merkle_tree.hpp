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

  MerkleTree()                      = default;
  MerkleTree(MerkleTree const &rhs) = default;
  MerkleTree(MerkleTree &&rhs)      = default;
  MerkleTree &operator=(MerkleTree const &rhs) = default;
  MerkleTree &operator=(MerkleTree &&rhs) = default;

  HashArray &operator[](std::size_t const &n)
  {
    return leaf_nodes_[std::to_string(n)];
  }

  HashArray &operator[](HashArray const &n)
  {
    return leaf_nodes_[n];
  }

  void CalculateRoot() const
  {
    if (leaf_nodes_.size() == 0)
    {
      root_ = Hash<crypto::SHA256>(HashArray{});
      return;
    }

    std::vector<HashArray> hashes;

    for (auto const &kv_iterator : leaf_nodes_)
    {
      auto &value = kv_iterator.second;
      hashes.push_back(value);
    }

    // If necessary bump the 'leaves' up to a power of 2
    while (!platform::IsLog2(uint64_t(hashes.size())))
    {
      hashes.push_back(HashArray{});
    }

    // Now, repeatedly condense the vector by calculating the parents of each of the roots
    while (hashes.size() > 1)
    {
      for (std::size_t i = 0; i < hashes.size(); i += 2)
      {
        HashArray concantenated_hash = hashes[i] + hashes[i + 1];
        concantenated_hash           = Hash<crypto::SHA256>(concantenated_hash);
        hashes[i]                    = concantenated_hash;
      }

      hashes.resize(hashes.size() / 2);
    }

    assert(hashes.size() == 1);
    root_ = hashes[0];
  }

  HashArray const &root() const
  {
    return root_;
  }

  Container const &leaf_nodes() const
  {
    return leaf_nodes_;
  }

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

namespace std {

}  // namespace std
