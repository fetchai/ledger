#include "beacon/beacon_manager.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdsa.hpp"

#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::crypto;
using namespace fetch::crypto::mcl;
using namespace fetch::dkg;
using Certificate      = Prover;
using CertificatePtr   = std::shared_ptr<Certificate>;
using MuddleAddress    = byte_array::ConstByteArray;

TEST(beacon_manager, test1)
{
    bn::initPairing();
    PublicKey zero;
    zero.clear();

    uint32_t cabinet_size = 3;
    uint32_t threshold = 2;
    std::vector<ECDSASigner*> member_ptrs;
    for (uint32_t index = 0; index < cabinet_size; ++index)
    {
        member_ptrs.emplace_back(new ECDSASigner());
    }
    CertificatePtr certificate1;
    certificate1.reset(member_ptrs[0]);

    BeaconManager manager(certificate1);

    std::set<Identity> cabinet;
    for (auto &mem : member_ptrs)
    {
        cabinet.insert(mem->identity());
    }
        manager.Reset(cabinet, threshold);

    // Check reset
    EXPECT_EQ(manager.cabinet_index(), std::distance(cabinet.begin(), cabinet.find(member_ptrs[0]->identity())));
    for (auto &mem : member_ptrs)
    {
        EXPECT_EQ(manager.cabinet_index(mem->identity().identifier()), std::distance(cabinet.begin(), cabinet.find(mem->identity())));
    }
    EXPECT_TRUE(manager.qual().empty());
    EXPECT_TRUE(manager.group_public_key()==zero.getStr());

    manager.GenerateCoefficients();

    // Check coefficients and shares generated are non-zero
    std::vector<std::string> coefficients = manager.GetCoefficients();
    for (auto &elem : coefficients)
    {
        EXPECT_TRUE(elem != zero.getStr());
    }
    for (auto &mem : member_ptrs)
    {
        EXPECT_TRUE(manager.GetOwnShares(mem->identity().identifier()).first != zero.getStr());
        EXPECT_TRUE(manager.GetOwnShares(mem->identity().identifier()).second != zero.getStr());
    }
    // Shares received from others should be zero
    for (auto &mem : member_ptrs)
    {
        if (mem != member_ptrs[0])
        {
            EXPECT_TRUE(manager.GetReceivedShares(mem->identity().identifier()).first == zero.getStr());
            EXPECT_TRUE(manager.GetReceivedShares(mem->identity().identifier()).second == zero.getStr());
        }
    }

    // Add shares from someone and check that they are entered in correctly
    PrivateKey s_i, sprime_i;
    s_i.setRand();
    sprime_i.setRand();
    MuddleAddress sender = member_ptrs[1]->identity().identifier();
    std::pair<std::string, std::string> shares = {s_i.getStr(), sprime_i.getStr()};
    manager.AddShares(sender, shares);
    EXPECT_EQ(manager.GetReceivedShares(sender), shares);

}