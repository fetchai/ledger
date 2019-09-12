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
    Generator generator_g, generator_h;
    SetGenerators(generator_g, generator_h);

    uint32_t cabinet_size = 3;
    uint32_t threshold = 2;
    std::vector<ECDSASigner*> member_ptrs;
    for (uint32_t index = 0; index < cabinet_size; ++index)
    {
        member_ptrs.emplace_back(new ECDSASigner());
    }

    // Set up two honest beacon managers
    CertificatePtr certificate;
    certificate.reset(member_ptrs[0]);
    BeaconManager manager(certificate);
    MuddleAddress my_address = member_ptrs[0]->identity().identifier();

    CertificatePtr certificate1;
    certificate1.reset(member_ptrs[1]);
    BeaconManager manager1(certificate1);


    // Create cabinet and reset the beacon managers
    std::set<Identity> cabinet;
    for (auto &mem : member_ptrs)
    {
        cabinet.insert(mem->identity());
    }
        manager.Reset(cabinet, threshold);
    manager1.Reset(cabinet, threshold);

    // Check reset for one manager
    EXPECT_EQ(manager.cabinet_index(), std::distance(cabinet.begin(), cabinet.find(member_ptrs[0]->identity())));
    for (auto &mem : member_ptrs)
    {
        EXPECT_EQ(manager.cabinet_index(mem->identity().identifier()), std::distance(cabinet.begin(), cabinet.find(mem->identity())));
    }
    EXPECT_TRUE(manager.qual().empty());
    EXPECT_TRUE(manager.group_public_key()==zero.getStr());

    manager.GenerateCoefficients();
    manager1.GenerateCoefficients();

    // Check coefficients and shares generated are non-zero and different
    std::vector<std::string> coefficients = manager.GetCoefficients();
    std::vector<std::string> coefficients1 = manager1.GetCoefficients();
    for (std::size_t elem = 0; elem < coefficients.size(); ++elem)
    {
        EXPECT_TRUE(coefficients[elem] != zero.getStr());
        EXPECT_TRUE(coefficients1[elem] != zero.getStr());
        EXPECT_TRUE(coefficients[elem] != coefficients1[elem]);
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

    // Add shares and coefficients passing verification from someone and check that they are entered in correctly
    MuddleAddress sender1 = member_ptrs[1]->identity().identifier();
    manager.AddShares(sender1, manager1.GetOwnShares(my_address));
    manager.AddCoefficients(sender1, manager1.GetCoefficients());
    EXPECT_EQ(manager.GetReceivedShares(sender1), manager1.GetOwnShares(my_address));

    // Add shares and coefficients failing verification from malicious party
    MuddleAddress malicious = member_ptrs[2]->identity().identifier();
    std::vector<PrivateKey> vec_a, vec_b;
    Init(vec_a, threshold);
    Init(vec_b, threshold);
    for (std::size_t ai = 0; ai < vec_a.size(); ++ai)
    {
        vec_a[ai].setRand();
        vec_b[ai].setRand();
    }
    PrivateKey s_i, sprime_i;
    ComputeShares(s_i, sprime_i, vec_a, vec_b, manager.cabinet_index(my_address));
    std::pair<std::string, std::string> correct_shares = {s_i.getStr(), sprime_i.getStr()};

    // Modify shares
    PrivateKey tmp;
    tmp.setRand();
    bn::Fr::add(tmp, tmp, s_i);
    std::pair<std::string, std::string> wrong_shares = {tmp.getStr(), sprime_i.getStr()};

    std::vector<std::string> sender_coeff2;
    Init(sender_coeff2, threshold);
    for (uint32_t k = 0; k < threshold; k++)
    {
        PublicKey temp_coeff;
        temp_coeff =
                crypto::mcl::ComputeLHS(generator_g, generator_h, vec_a[k], vec_b[k]);
        sender_coeff2[k] = temp_coeff.getStr();
    }

    manager.AddShares(malicious, wrong_shares);
    manager.AddCoefficients(malicious, sender_coeff2);
    EXPECT_EQ(manager.GetReceivedShares(malicious), wrong_shares);

    std::set<MuddleAddress> coeff_received;
    coeff_received.insert(sender1);
    coeff_received.insert(malicious);
    auto complaints = manager.ComputeComplaints(coeff_received);

    std::unordered_set<MuddleAddress> complaints_expected = {malicious};
    EXPECT_EQ(complaints, complaints_expected);

    // Submit false complaints answer
    BeaconManager::ComplaintAnswer wrong_answer;
    wrong_answer.first = my_address;
    wrong_answer.second = wrong_shares;
    EXPECT_FALSE(manager.VerifyComplaintAnswer(malicious, wrong_answer));
    EXPECT_EQ(manager.GetReceivedShares(malicious), wrong_shares);

    // Submit correct correct complaints answer and check values get replaced
    BeaconManager::ComplaintAnswer correct_answer;
    correct_answer.first = my_address;
    correct_answer.second = correct_shares;
    EXPECT_TRUE(manager.VerifyComplaintAnswer(malicious, correct_answer));
    EXPECT_EQ(manager.GetReceivedShares(malicious), correct_shares);

    // Since bad shares have been replaced set qual to be everyone
    std::set<MuddleAddress> qual;
    qual.insert(my_address);
    qual.insert(sender1);
    qual.insert(malicious);
    manager.SetQual(qual);

    // Check computed secret shares
    manager.ComputeSecretShare();
    PrivateKey secret_key_test;
    for (auto & mem : qual)
    {
        PrivateKey share;
        share.setStr(manager.GetReceivedShares(mem).first);
        bn::Fr::add(secret_key_test, secret_key_test, share);
    }
    PrivateKey computed_secret_key;
    PublicKey dummy_public;
    std::vector<PublicKey> dummy_public_shares;
    std::set<MuddleAddress> dummy_qual;
    manager.SetDkgOutput(dummy_public, computed_secret_key, dummy_public_shares, dummy_qual);
    EXPECT_EQ(computed_secret_key, secret_key_test);

    // Add honest qual coefficients
    manager.AddQualCoefficients(sender1, manager1.GetQualCoefficients());

    // Verify qual coefficients before malicious submitted coefficients - expect complaint against them
    BeaconManager::SharesExposedMap qual_complaints_test;
    qual_complaints_test.insert({malicious, correct_shares});
    auto actual_qual_complaints = manager.ComputeQualComplaints({sender1});
    EXPECT_EQ(actual_qual_complaints, qual_complaints_test);

    // Add wrong qual coefficients
    manager.AddQualCoefficients(malicious, sender_coeff2);

    // Verify qual coefficients and check the complaints
    auto actual_qual_complaints1 = manager.ComputeQualComplaints(coeff_received);
    EXPECT_EQ(actual_qual_complaints1, qual_complaints_test);


}