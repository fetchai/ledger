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

#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <ostream>

using namespace fetch::crypto;
using namespace fetch::byte_array;

using SecretKeyArray = std::vector<bls::PrivateKey>();

template <typename T>
void ToBinary(std::ostream &stream, T const &value)
{
  auto const *   raw = reinterpret_cast<uint8_t const *>(&value);
  ConstByteArray data{raw, sizeof(T)};

  stream << data.ToBase64();
}

std::ostream &operator<<(std::ostream &stream, bls::PrivateKey const &private_key)
{
  ToBinary(stream, private_key);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, bls::Id const &private_key)
{
  ToBinary(stream, private_key);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, bls::PublicKey const &private_key)
{
  ToBinary(stream, private_key);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, bls::Signature const &private_key)
{
  ToBinary(stream, private_key);
  return stream;
}

bls::Id GenerateId()
{
  auto const sk = bls::PrivateKeyByCSPRNG();
  bls::Id    id;
  id.v = sk.v;

  return id;
}

TEST(BlsTests, SimpleRandomBeaconFlow)
{
  bls::Init();

  auto dealer     = bls::PrivateKeyByCSPRNG();
  auto dealer_pub = bls::PublicKeyFromPrivate(dealer);

  auto sk1 = bls::PrivateKeyByCSPRNG();
  auto sk2 = bls::PrivateKeyByCSPRNG();
  auto sk3 = bls::PrivateKeyByCSPRNG();
  auto sk4 = bls::PrivateKeyByCSPRNG();

  std::cout << "SK1 " << sk1 << std::endl;
  std::cout << "SK2 " << sk2 << std::endl;
  std::cout << "SK3 " << sk3 << std::endl;
  std::cout << "SK4 " << sk4 << std::endl;

  bls::Id id1 = GenerateId();
  bls::Id id2 = GenerateId();
  bls::Id id3 = GenerateId();
  bls::Id id4 = GenerateId();

  std::cout << "ID1 " << id1 << std::endl;
  std::cout << "ID2 " << id2 << std::endl;
  std::cout << "ID3 " << id3 << std::endl;
  std::cout << "ID4 " << id4 << std::endl;

  std::vector<bls::PrivateKey> master_key(2);  // threshold
  master_key[0] = dealer;
  master_key[1] = bls::PrivateKeyByCSPRNG();

  auto share1 = bls::PrivateKeyShare(master_key, id1);
  auto share2 = bls::PrivateKeyShare(master_key, id2);
  auto share3 = bls::PrivateKeyShare(master_key, id3);
  auto share4 = bls::PrivateKeyShare(master_key, id4);

  auto share_pub1 = bls::PublicKeyFromPrivate(share1);
  auto share_pub2 = bls::PublicKeyFromPrivate(share2);
  auto share_pub3 = bls::PublicKeyFromPrivate(share3);
  auto share_pub4 = bls::PublicKeyFromPrivate(share4);

  ConstByteArray message = "hello my name is ed";

  auto sig1 = bls::Sign(share1, message);
  EXPECT_TRUE(bls::Verify(sig1, share_pub1, message));

  auto sig2 = bls::Sign(share2, message);
  EXPECT_TRUE(bls::Verify(sig2, share_pub2, message));

  auto sig3 = bls::Sign(share3, message);
  EXPECT_TRUE(bls::Verify(sig3, share_pub3, message));

  auto sig4 = bls::Sign(share4, message);
  EXPECT_TRUE(bls::Verify(sig4, share_pub4, message));

  ConstByteArray reference_sig;
  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig2, sig3}, {id1, id2, id3});
    reference_sig      = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig2, sig4, sig3}, {id2, id4, id3});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig4}, {id1, id4});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }

  message = "hello my name is ed again";

  sig1 = bls::Sign(share1, message);
  EXPECT_TRUE(bls::Verify(sig1, share_pub1, message));

  sig2 = bls::Sign(share2, message);
  EXPECT_TRUE(bls::Verify(sig2, share_pub2, message));

  sig3 = bls::Sign(share3, message);
  EXPECT_TRUE(bls::Verify(sig3, share_pub3, message));

  sig4 = bls::Sign(share4, message);
  EXPECT_TRUE(bls::Verify(sig4, share_pub4, message));

  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig2, sig3}, {id1, id2, id3});
    reference_sig      = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig2, sig4, sig3}, {id2, id4, id3});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig4}, {id1, id4});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }

  message = "hello my name is ed again2";

  sig1 = bls::Sign(share1, message);
  EXPECT_TRUE(bls::Verify(sig1, share_pub1, message));

  sig2 = bls::Sign(share2, message);
  EXPECT_TRUE(bls::Verify(sig2, share_pub2, message));

  sig3 = bls::Sign(share3, message);
  EXPECT_TRUE(bls::Verify(sig3, share_pub3, message));

  sig4 = bls::Sign(share4, message);
  EXPECT_TRUE(bls::Verify(sig4, share_pub4, message));

  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig2, sig3}, {id1, id2, id3});
    reference_sig      = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig2, sig4, sig3}, {id2, id4, id3});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }

  {
    auto recovered_sig = bls::RecoverSignature({sig1, sig4}, {id1, id4});
    auto bin_sig       = bls::ToBinary(recovered_sig);

    EXPECT_TRUE(bls::Verify(recovered_sig, dealer_pub, message));
    EXPECT_EQ(reference_sig, bin_sig);
  }
}
