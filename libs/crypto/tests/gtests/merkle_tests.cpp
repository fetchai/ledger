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

#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/hash.hpp"
#include "crypto/merkle_tree.hpp"
#include "crypto/sha256.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace fetch;
using namespace fetch::crypto;

using ByteArray = byte_array::ByteArray;

TEST(crypto_merkle_tree, empty_tree)
{
  MerkleTree tree;

  tree.CalculateRoot();

  /* std::cout << "Output is: '" << byte_array::ToHex(tree.root())  << "'"<< std::endl; */

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), Hash<crypto::SHA256>(ByteArray{}));
}

TEST(crypto_merkle_tree, partially_filled_tree)
{
  MerkleTree tree;

  for (std::size_t i = 0; i < 100; ++i)
  {
    tree[i] = ByteArray{std::to_string(i)};
  }

  tree.CalculateRoot();

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_NE(tree.root(), Hash<crypto::SHA256>(ByteArray{}));  // This is unlikely
}

TEST(crypto_merkle_tree, complete_tree_and_deterministic)
{
  MerkleTree tree;
  MerkleTree tree2;

  for (std::size_t i = 0; i < 256; ++i)
  {
    tree[i]  = ByteArray{std::to_string(i)};
    tree2[i] = ByteArray{std::to_string(i)};
  }

  tree.CalculateRoot();
  tree2.CalculateRoot();

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_NE(tree.root(), Hash<crypto::SHA256>(ByteArray{}));
  EXPECT_EQ(tree.root(), tree2.root());
}

TEST(crypto_merkle_tree, serializes_deserializes)
{
  MerkleTree tree;   // Reference
  MerkleTree tree2;  // Calculate root then serialize
  MerkleTree tree3;  // Don't calculate root until after serialize
  MerkleTree tree2_deser;
  MerkleTree tree3_deser;

  for (std::size_t i = 0; i < 256; ++i)
  {
    tree[i]  = ByteArray{std::to_string(i)};
    tree2[i] = ByteArray{std::to_string(i)};
    tree3[i] = ByteArray{std::to_string(i)};
  }

  tree.CalculateRoot();
  tree2.CalculateRoot();

  {
    fetch::serializers::ByteArrayBuffer arr;
    arr << tree2;
    arr.seek(0);
    arr >> tree2_deser;
  }

  {
    fetch::serializers::ByteArrayBuffer arr;
    arr << tree3;
    arr.seek(0);
    arr >> tree3_deser;
  }

  tree3_deser.CalculateRoot();

  EXPECT_EQ(tree2_deser.root(), tree.root());
  EXPECT_EQ(tree3_deser.root(), tree.root());
  EXPECT_EQ(tree2_deser.root(), tree3_deser.root());
}

TEST(crypto_merkle_tree, same_result_after_move)
{
  MerkleTree tree2;
  MerkleTree tree;

  for (std::size_t i = 0; i < 256; ++i)
  {
    tree2[i] = ByteArray{std::to_string(i)};
  }

  tree2.CalculateRoot();

  ByteArray root_before = tree2.root();

  tree = tree2;

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), root_before);
}
