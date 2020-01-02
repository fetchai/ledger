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

#include "crypto/hash.hpp"
#include "crypto/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include "vectorise/platform.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

using HashArray = MerkleTree::Digest;
using Container = MerkleTree::Container;

MerkleTree::MerkleTree(std::size_t count)
  : leaf_nodes_{count}
{}

HashArray &MerkleTree::operator[](std::size_t n)
{
  return leaf_nodes_.at(n);
}

void MerkleTree::CalculateRoot() const
{
  if (leaf_nodes_.empty())
  {
    root_ = Hash<crypto::SHA256>(Digest{});
    return;
  }
  if (leaf_nodes_.size() == 1)
  {
    // special case if there is only one node in the tree it is its own merkle root
    root_ = leaf_nodes_[0];
    return;
  }

  // make a copy of the leaf nodes which are then condensed
  std::vector<Digest> hashes = leaf_nodes_;

  // If necessary bump the 'leaves' up to a power of 2
  while (!platform::IsLog2(uint64_t(hashes.size())))
  {
    hashes.emplace_back();
  }

  // Now, repeatedly condense the vector by calculating the parents of each of the roots
  while (hashes.size() > 1)
  {
    for (std::size_t i = 0, j = 0; i < hashes.size(); i += 2, ++j)
    {
      Digest concantenated_hash = hashes[i] + hashes[i + 1];
      concantenated_hash        = Hash<crypto::SHA256>(concantenated_hash);
      hashes[j]                 = concantenated_hash;
    }

    hashes.resize(hashes.size() / 2);
  }

  assert(hashes.size() == 1);
  root_ = hashes[0];
}

MerkleTree::Digest const &MerkleTree::root() const
{
  return root_;
}

MerkleTree::Container const &MerkleTree::leaf_nodes() const
{
  return leaf_nodes_;
}

std::size_t MerkleTree::size() const
{
  return leaf_nodes_.size();
}

MerkleTree::Iterator MerkleTree::begin()
{
  return leaf_nodes_.begin();
}

MerkleTree::ConstIterator MerkleTree::begin() const
{
  return leaf_nodes_.begin();
}

MerkleTree::ConstIterator MerkleTree::cbegin()
{
  return leaf_nodes_.cbegin();
}

MerkleTree::Iterator MerkleTree::end()
{
  return leaf_nodes_.end();
}

MerkleTree::ConstIterator MerkleTree::end() const
{
  return leaf_nodes_.end();
}

MerkleTree::ConstIterator MerkleTree::cend() const
{
  return leaf_nodes_.cend();
}

}  // namespace crypto
}  // namespace fetch
