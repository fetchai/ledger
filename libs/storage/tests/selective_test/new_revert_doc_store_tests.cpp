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
#include "core/random/lfg.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/new_revertible_document_store.hpp"
#include "testing/common_testing_functionality.hpp"
#include "core/random/lcg.hpp"
#include "crypto/merkle_tree.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::crypto;
using namespace fetch::testing;

using fetch::random::LinearCongruentialGenerator;

using ByteArray      = fetch::byte_array::ByteArray;
using ConstByteArray = fetch::byte_array::ConstByteArray;

char NewChar(LinearCongruentialGenerator & rng)
{
  char a = char(rng());
  return a == '\0' ? '0' : a;
}

std::string GetStringForTesting(LinearCongruentialGenerator & rng)
{
  uint64_t    size_desired = (1 << 10) + (rng() & 0xFF);
  std::string ret;
  ret.resize(size_desired);

  for (std::size_t i = 0; i < 1 << 10; ++i)
  {
    ret[i] = NewChar(rng);
  }

  if(!(rng() % 10))
  {
    ret = std::string{};
  }

  return ret;
}

TEST(new_revertible_store_test, basic_example_of_commit_revert1)
{
  NewRevertibleDocumentStore store;
  store.New("a_11.db", "b_11.db", "c_11.db", "d_11.db", true);

  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  // Make some changes to the store
  for (std::size_t i = 0; i < 17; ++i)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceAddress(set_me), set_me);
    ASSERT_EQ(store.size(), i + 1);  // this fails for tests using correlated strings
  }

  // Verify state is correct with no changes
  for (std::size_t i = 0; i < 17; ++i)
  {
    // Test for success
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // Test for fail
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
      EXPECT_EQ(document.failed, true);
      EXPECT_EQ(document.was_created, false);
      EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
    }
  }

  // *** Commit this ***
  hashes.push_back(store.Commit());

  // Verify state is the same
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
  }

  // mash the state
  for (std::size_t i = 0; i < 17; ++i)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceAddress(set_me), std::to_string(i + 5));
  }

  // Verify the change
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(std::string(document.document), std::to_string(i + 5));
  }

  // EXPECT_EQ(store.HashExists(hashes[0]), true);

  // Revert!
  store.RevertToHash(hashes[0]);

  // Verify old state is as it was
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
  }
}

TEST(new_revertible_store_test, more_involved_commit_revert)
{
  NewRevertibleDocumentStore store;
  store.New("a_12.db", "b_12.db", "c_12.db", "d_12.db", true);

  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  // Our keys will be selected to be very close to each other
  auto unique_hashes = GenerateUniqueHashes(1000);

  // Make some changes to the store
  std::size_t i = 0;
  for (auto const &hash : unique_hashes)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceID(hash), set_me);
    ASSERT_EQ(store.size(), i + 1); /* This fails */
    ++i;
  }

  i = 0;
  for (auto const &hash : unique_hashes)
  {
    // Test for success
    {
      auto document = store.Get(storage::ResourceID(hash));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // Test for fail
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
      EXPECT_EQ(document.failed, true);
      EXPECT_EQ(document.was_created, false);
      EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
    }
    ++i;
  }
}

TEST(new_revertible_store_test, basic_example_of_commit_revert_with_load)
{
  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  {
    NewRevertibleDocumentStore store;
    store.New("a_13.db", "b_13.db", "c_13.db", "d_13.db", true);

    // Make some changes to the store
    for (std::size_t i = 0; i < 17; ++i)
    {
      std::string set_me{std::to_string(i)};
      store.Set(storage::ResourceAddress(set_me), set_me);
      ASSERT_EQ(store.size(), i + 1);  // this fails for tests using correlated strings
    }

    // Verify state is correct with no changes
    for (std::size_t i = 0; i < 17; ++i)
    {
      // Test for success
      {
        auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
        EXPECT_EQ(document.failed, false);
        EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
      }

      // Test for fail
      {
        auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
        EXPECT_EQ(document.failed, true);
        EXPECT_EQ(document.was_created, false);
        EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
      }
    }

    // *** Commit this ***
    hashes.push_back(store.Commit());

    // Verify state is the same
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // mash the state
    for (std::size_t i = 0; i < 17; ++i)
    {
      std::string set_me{std::to_string(i)};
      store.Set(storage::ResourceAddress(set_me), std::to_string(i + 5));
    }

    // Verify the change
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i + 5)));
    }

    // EXPECT_EQ(store.HashExists(hashes[0]), true);

    // Revert!
    store.RevertToHash(hashes[0]);

    // Verify old state is as it was
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }
  }
}

TEST(new_revertible_store_test, erase_functionality_works)
{
  NewRevertibleDocumentStore store;
  store.New("a_44.db", "b_44.db", "c_44.db", "d_44.db", true);

  auto unique_hashes = GenerateUniqueHashes(1000);

  std::size_t i = 0;
  for (auto const &hash : unique_hashes)
  {
    std::string set_me{std::to_string(i)};
    auto rid = storage::ResourceID(hash);
    store.Set(rid, set_me);
    ASSERT_EQ(store.size(), 1);

    store.Erase(rid);
    ASSERT_EQ(store.size(), 0);
    ASSERT_EQ(store.Get(rid).failed, true);
    ++i;
  }
}

TEST(new_revertible_store_test, erase_functionality_works_at_scale)
{
  NewRevertibleDocumentStore store;
  store.New("a_44.db", "b_44.db", "c_44.db", "d_44.db", true);

  auto unique_hashes = GenerateUniqueHashes(1000);

  std::size_t i             = 0;
  std::size_t expected_size = 0;
  for (auto const &hash : unique_hashes)
  {
    std::string set_me{std::to_string(i)};
    auto rid = storage::ResourceID(hash);
    store.Set(rid, set_me);

    if(i % 2)
    {
      //std::cerr << "Insert!" << std::endl; // DELETEME_NH
      expected_size++;
      ASSERT_EQ(store.size(), expected_size);
      ASSERT_EQ(store.Get(rid).failed, false);
      ASSERT_EQ(std::string{store.Get(rid).document}, std::to_string(i));
    }
    else
    {
      //std::cerr << "Erase!" << std::endl; // DELETEME_NH
      store.Erase(rid);
      ASSERT_EQ(store.Get(rid).failed, true);
    }
    ++i;
  }
}

TEST(new_revertible_store_test, more_erase)
{
  NewRevertibleDocumentStore store;
  store.New("a_55.db", "b_55.db", "c_55.db", "d_55.db", true);

  // Our keys will be selected to be very close to each other
  auto unique_hashes = GenerateUniqueHashes(1000);
  std::unordered_map<storage::ResourceID, std::string> expected_in_store;

  // Make some changes to the store
  std::size_t i = 0;
  for (auto const &hash : unique_hashes)
  {
    std::string set_me{std::to_string(i)};
    auto rid = storage::ResourceID(hash);
    store.Set(rid, set_me);
    expected_in_store[rid] = set_me;
    ASSERT_EQ(store.size(), i + 1);
    ASSERT_EQ(store.size(), expected_in_store.size());
    ++i;
  }

  // Erase the elements
  for (auto it = expected_in_store.begin();it != expected_in_store.end();)
  {
    store.Erase(it->first);
    it = expected_in_store.erase(it);
    ASSERT_EQ(store.size(), expected_in_store.size());
  }
}

TEST(new_revertible_store_test, commit_and_erase)
{
  NewRevertibleDocumentStore store;
  store.New("a_555.db", "b_555.db", "c_555.db", "d_555.db", true);

  auto unique_hashes = GenerateUniqueHashes(1000);

  std::size_t i             = 0;
  std::size_t expected_size = 0;
  for (auto const &hash : unique_hashes)
  {
    std::string set_me{std::to_string(i)};
    auto rid = storage::ResourceID(hash);
    store.Set(rid, set_me);

    ++expected_size;

    if(i % 2)
    {
      //std::cerr << "Insert!" << std::endl; // DELETEME_NH
      ASSERT_EQ(store.size(), expected_size);
      ASSERT_EQ(store.Get(rid).failed, false);
      ASSERT_EQ(std::string{store.Get(rid).document}, std::to_string(i));
    }
    else
    {
      auto hash_before_erase = store.CurrentHash();
      store.Commit();
      store.Erase(rid);
      auto hash_after_erase = store.CurrentHash();

      ASSERT_NE(hash_before_erase, hash_after_erase);

      store.RevertToHash(hash_before_erase);
      ASSERT_EQ(store.CurrentHash(), hash_before_erase);
    }
    ++i;
  }
}

// note: disabled because the storage does not hash the same way as the merkle tree
TEST(new_revertible_store_test, DISABLED_hashing_correct_basic)
{
  NewRevertibleDocumentStore store;
  store.New("a_0133.db", "b_0133.db", "c_0133.db", "d_0133.db", true);
  LinearCongruentialGenerator rng;

  auto hash_pool        = GenerateUniqueHashes(1000);
  auto rid_pool         = GenerateUniqueIDs(1000);

  using State = std::map<storage::ResourceID, std::string>;
  State current_state;
  crypto::MerkleTree  reference_tree{0};
  std::string         random_string;

  for(auto const &rid : rid_pool)
  {
    random_string = GetStringForTesting(rng);

    store.Set(rid, random_string);
    current_state[rid] = random_string;

    reference_tree = crypto::MerkleTree{current_state.size()};

    std::size_t counter = 0;
    for (auto it = current_state.begin(); it != current_state.end(); ++it)
    {
      reference_tree[counter++] = Hash<crypto::SHA256>(it->second);
    }

    reference_tree.CalculateRoot();

    auto store_hash    = store.CurrentHash().ToBase64();
    auto merkle_hash   = reference_tree.root().ToBase64();
    auto raw_hash      = Hash<crypto::SHA256>(random_string).ToBase64();
    auto raw_hash_hash = Hash<crypto::SHA256>(raw_hash).ToBase64();

    auto store_size         = store.size();
    auto current_state_size = current_state.size();

    ASSERT_EQ(store_size, current_state_size);
    ASSERT_EQ(store_hash, merkle_hash);
  }
}

template <typename C>
typename C::value_type GetRandom(C const &container, LinearCongruentialGenerator &rng)
{
  auto beg = container.cbegin();
  auto container_size = container.size();

  if(container_size == 0)
  {
    return {};
  }

  std::size_t select = rng() % container_size;

  for (std::size_t i = 0; i < select; ++i)
  {
    beg++;
  }

  return *beg;
}

TEST(new_revertible_store_test, stress_test)
{
  NewRevertibleDocumentStore store;
  store.New("a_66.db", "b_66.db", "c_66.db", "d_66.db", true);
  LinearCongruentialGenerator rng;

  // TODO(HUT): replace this with a fake RDS
  using CommitID = ByteArray;
  using State = std::unordered_map<storage::ResourceID, std::string>;

  std::unordered_map<CommitID, State> previous_states;
  State current_state;
  auto hash_pool        = GenerateUniqueHashes(1000, 0);
  auto rid_pool         = GenerateUniqueIDs(1000, 1);
  auto unused_hash_pool = GenerateUniqueHashes(1000, 2);
  auto unused_rid_pool  = GenerateUniqueIDs(1000, 3);

  std::unordered_set<storage::ResourceID> combine = rid_pool;

  for(auto const &i : unused_rid_pool)
  {
    combine.insert(i);
  }

  auto ab = combine.size();
  FETCH_UNUSED(ab);

  enum class Action
  {
    GET,
    GET_OR_CREATE,
    SET,
    ERASE,
    COMMIT,
    REVERT,
    CHECK_FOR_HASH,
    BAD_CHECK_FOR_HASH,
    BAD_REVERT,
    BAD_ERASE
  };

  std::vector<Action> prev_actions;
  FETCH_UNUSED(prev_actions);

  for (std::size_t i = 0; i < 10000; ++i)
  {
    uint64_t rnd_action = rng() % 100;
    Action action{Action::GET};

    if(rnd_action > 90)
    {
      std::cerr << ">GET" << std::endl; // DELETEME_NH
      action = Action::GET;
    }
    else if(rnd_action > 80)
    {
      std::cerr << ">GET/create" << std::endl; // DELETEME_NH
      action = Action::GET_OR_CREATE;
    }
    else if(rnd_action > 70)
    {
      std::cerr << ">set" << std::endl; // DELETEME_NH
      action = Action::SET;
    }
    else if(rnd_action > 60)
    {
      std::cerr << ">erase" << std::endl; // DELETEME_NH
      action = Action::ERASE;
    }
    else if(rnd_action > 50)
    {
      std::cerr << ">commit" << std::endl; // DELETEME_NH
      action = Action::COMMIT;
    }
    else if(rnd_action > 40)
    {
      std::cerr << ">revert" << std::endl; // DELETEME_NH
      action = Action::REVERT;
    }
    else if(rnd_action > 30)
    {
      std::cerr << ">check 4 hash" << std::endl; // DELETEME_NH
      action = Action::CHECK_FOR_HASH;
    }
    else if(rnd_action > 20)
    {
      std::cerr << ">bad check 4 hash" << std::endl; // DELETEME_NH
      action = Action::BAD_CHECK_FOR_HASH;
    }
    else if(rnd_action > 10)
    {
      std::cerr << ">bad revert" << std::endl; // DELETEME_NH
      action = Action::BAD_REVERT;
    }
    else
    {
      std::cerr << ">bad erase" << std::endl; // DELETEME_NH
      action = Action::BAD_ERASE;
    }

    prev_actions.push_back(action);

    storage::ResourceID random_rid          = GetRandom(rid_pool, rng);
    ByteArray           random_unused_hash  = GetRandom(unused_hash_pool, rng);
    storage::ResourceID random_unused_rid   = GetRandom(unused_rid_pool, rng);
    ByteArray           random_prev_commmit = GetRandom(previous_states, rng).first;
    std::string         random_string       = GetStringForTesting(rng);
    ByteArray           current_hash;

    switch(action)
    {
      case Action::GET:
        if(current_state.find(random_rid) != current_state.end())
        {
          ASSERT_EQ(store.Get(random_rid).failed, false);
          ASSERT_EQ(store.Get(random_rid).was_created, false);
          ASSERT_EQ(current_state[random_rid], std::string{store.Get(random_rid).document});
        }
        else
        {
          ASSERT_EQ(store.Get(random_rid).failed, true);
          ASSERT_EQ(store.Get(random_rid).was_created, false);
        }
      break;

      case Action::GET_OR_CREATE:
        /*
        random_rid = GetRandom(rid_pool, rng);

        if(current_state.find(random_rid) != current_state.end())
        {
          ASSERT_EQ(store.Get(random_rid).failed, false);
          ASSERT_EQ(store.Get(random_rid).was_created, false);
          ASSERT_EQ(current_state[random_rid], std::string{store.Get(random_rid).document});
        }
        else
        {
          ASSERT_EQ(store.Get(random_rid).failed, true);
          ASSERT_EQ(store.Get(random_rid).was_created, false);
        }
        */
      break;

      case Action::SET:
        current_state[random_rid] = random_string;
        store.Set(random_rid, random_string);

        std::cerr << "After setting, hash is " << store.CurrentHash().ToBase64() << std::endl; // DELETEME_NH
      break;

      case Action::ERASE:

        if(current_state.find(random_rid) != current_state.end())
        {
          current_state.erase(random_rid);
        }

        store.Erase(random_rid);
        ASSERT_EQ(store.Get(random_rid).failed, true);
      break;

      case Action::COMMIT:

      current_hash = store.CurrentHash();

      std::cerr << "Commit: " << current_hash.ToBase64() << std::endl; // DELETEME_NH
      std::cerr << "size: " << store.size() << std::endl; // DELETEME_NH

      previous_states[current_hash] = current_state;

      ASSERT_EQ(store.Commit(), current_hash);
      ASSERT_EQ(store.HashExists(current_hash), true);
      break;

      case Action::REVERT:
      // TODO(HUT): this could be better - stack oriented.
      if(!previous_states.empty() && store.HashExists(random_prev_commmit))
      {
        auto aa = previous_states.size(); FETCH_UNUSED(aa);
        assert(previous_states.size() != 0);
        current_state = std::move(previous_states[random_prev_commmit]);
        previous_states.erase(random_prev_commmit);

        aa = previous_states.size(); FETCH_UNUSED(aa);

        auto hash_before0 = store.CurrentHash();
        auto hash_before1 = hash_before0.ToBase64();
        auto rand_p_c = random_prev_commmit.ToBase64();

        ASSERT_EQ(store.HashExists(random_prev_commmit), true);
        ASSERT_EQ(store.RevertToHash(random_prev_commmit), true);

        auto hash_after0 = store.CurrentHash();
        auto hash_after1 = hash_before1.ToBase64();

        FETCH_UNUSED(hash_before0);
        FETCH_UNUSED(hash_before1);
        FETCH_UNUSED(hash_after0);
        FETCH_UNUSED(hash_after1);

        ASSERT_EQ(store.CurrentHash(), random_prev_commmit);
        /* ASSERT_EQ(store.HashExists(random_prev_commmit), false); */ // TODO(HUT): this fails.
        std::cerr << "revert to: " << random_prev_commmit.ToBase64() << std::endl; // DELETEME_NH
      }
      break;

      case Action::CHECK_FOR_HASH:
      if(!previous_states.empty())
      {
        /* ASSERT_EQ(store.HashExists(random_prev_commmit), true); */
      }
      break;

      case Action::BAD_CHECK_FOR_HASH:
        ASSERT_EQ(store.HashExists(random_unused_hash), false);
      break;

      case Action::BAD_REVERT:
        ASSERT_EQ(store.RevertToHash(random_unused_hash), false);
      break;

      case Action::BAD_ERASE:

        store.Erase(random_unused_rid);
      break;
    }

    std::cerr << "Hash after operation: " << store.CurrentHash().ToBase64() << std::endl; // DELETEME_NH
    std::cerr << "" << std::endl; // DELETEME_NH

    // Check states are identical
    for (auto it = current_state.begin(); it != current_state.end(); ++it)
    {
      if(store.Get(it->first).failed != false)
      {
        std::cerr << "" << std::endl; // DELETEME_NH
      }

      ASSERT_EQ(store.Get(it->first).failed, false);
      ASSERT_EQ(it->second, std::string{store.Get(it->first).document});
    }

    if(current_state.size() != store.size())
    {
      std::cerr << "argh" << std::endl; // DELETEME_NH
    }

    ASSERT_EQ(current_state.size(), store.size());
  }
}
