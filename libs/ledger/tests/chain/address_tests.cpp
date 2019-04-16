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

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "crypto/identity.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/v2/address.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <algorithm>

using fetch::byte_array::ConstByteArray;
using fetch::crypto::Identity;
using fetch::crypto::ECDSASigner;
using fetch::crypto::SHA256;
using fetch::crypto::Hash;
using fetch::ledger::v2::Address;


class AddressTests : public ::testing::Test
{
protected:
  using HashFunction = fetch::crypto::SHA256;

  ConstByteArray CreateAddress(Identity const &identity)
  {
    return Hash<HashFunction>(identity.identifier());
  }

  ConstByteArray CreateChecksum(ConstByteArray const &address)
  {
    return Hash<HashFunction>(address).SubArray(0, 4);
  }

  Address::RawAddress CreateRawAddress(Identity const &identity)
  {
    Address::RawAddress raw_address{};

    // create the initial address
    auto const address = CreateAddress(identity);
    if (address.size() != raw_address.size())
    {
      throw std::logic_error("Incompatible array sizes");
    }

    // copy the contents of the array
    std::memcpy(raw_address.data(), address.pointer(), address.size());

    return raw_address;
  }
};

TEST_F(AddressTests, CheckEmptyConstruction)
{
  Address address{};

  EXPECT_TRUE(address.address().empty());
  EXPECT_TRUE(address.display().empty());
}

TEST_F(AddressTests, CheckIdentityContruction)
{
  // create the private key
  ECDSASigner signer;

  // create the address from the identity
  Address address{signer.identity()};

  // generate our expectations
  auto const expected_address = CreateAddress(signer.identity());
  auto const expected_display = expected_address + CreateChecksum(expected_address);

  // check them
  EXPECT_EQ(address.address(), expected_address);
  EXPECT_EQ(address.display(), expected_display);
}

TEST_F(AddressTests, CheckRawAddressContruction)
{
  // create the private key
  ECDSASigner signer;

  Address::RawAddress raw_address = CreateRawAddress(signer.identity());

  // create the address from the identity
  Address address{raw_address};

  // generate our expectations
  auto const expected_address = CreateAddress(signer.identity());
  auto const expected_display = expected_address + CreateChecksum(expected_address);

  // check them
  EXPECT_EQ(address.address(), expected_address);
  EXPECT_EQ(address.display(), expected_display);
}
