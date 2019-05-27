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
#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/hash.hpp"
#include "crypto/merkle_tree.hpp"
#include "crypto/sha256.hpp"

#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::crypto;

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

static ConstByteArray CalculateHash(ConstByteArray const &a, ConstByteArray const &b)
{
  crypto::SHA256 sha256{};
  sha256.Update(a);
  sha256.Update(b);
  return sha256.Final();
}

TEST(crypto_merkle_tree, empty_tree)
{
  MerkleTree tree{0};
  tree.CalculateRoot();

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), Hash<crypto::SHA256>(ByteArray{}));
}

TEST(crypto_merkle_tree, manual_test_log2_count)
{
  MerkleTree tree{4};

  // populate the tree
  for (std::size_t i = 0; i < tree.size(); ++i)
  {
    // generate the value
    ByteArray hash_value;
    hash_value.Resize(256 / 8);
    std::memset(hash_value.pointer(), static_cast<int>(i), hash_value.size());

    // store it in the tree
    tree[i] = hash_value;
  }

  // manually generate the merkle hash
  auto const intermediate1 = CalculateHash(tree[0], tree[1]);
  auto const intermediate2 = CalculateHash(tree[2], tree[3]);
  auto const final         = CalculateHash(intermediate1, intermediate2);

  tree.CalculateRoot();
  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), final);
}

TEST(crypto_merkle_tree, manual_test_non_log2_count)
{
  MerkleTree tree{5};

  // populate the tree
  for (std::size_t i = 0; i < tree.size(); ++i)
  {
    // generate the value
    ByteArray hash_value;
    hash_value.Resize(256 / 8);
    std::memset(hash_value.pointer(), static_cast<int>(i), hash_value.size());

    // store it in the tree
    tree[i] = hash_value;
  }

  // manually generate the merkle hash
  auto const intermediate1_1 = CalculateHash(tree[0], tree[1]);
  auto const intermediate1_2 = CalculateHash(tree[2], tree[3]);
  auto const intermediate1_3 = CalculateHash(tree[4], ConstByteArray{});
  auto const intermediate1_4 = CalculateHash(ConstByteArray{}, ConstByteArray{});
  auto const intermediate2_1 = CalculateHash(intermediate1_1, intermediate1_2);
  auto const intermediate2_2 = CalculateHash(intermediate1_3, intermediate1_4);
  auto const final           = CalculateHash(intermediate2_1, intermediate2_2);

  tree.CalculateRoot();

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), final);
}

TEST(crypto_merkle_tree, partially_filled_tree)
{
  MerkleTree tree{100};

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
  MerkleTree tree{256};
  MerkleTree tree2{256};

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
  MerkleTree tree{256};   // Reference
  MerkleTree tree2{256};  // Calculate root then serialize
  MerkleTree tree3{256};  // Don't calculate root until after serialize
  MerkleTree tree2_deser{256};
  MerkleTree tree3_deser{256};

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
  MerkleTree tree2{256};
  MerkleTree tree{256};

  for (std::size_t i = 0; i < 256; ++i)
  {
    tree2[i] = ByteArray{std::to_string(i)};
  }

  tree2.CalculateRoot();

  ByteArray root_before = tree2.root();

  tree = std::move(tree2);

  EXPECT_EQ(tree.root().size(), 256 / 8);
  EXPECT_EQ(tree.root(), root_before);
}
