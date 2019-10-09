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

#include "beacon/beacon_manager.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdsa.hpp"

#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::crypto;
using namespace fetch::crypto::mcl;
using namespace fetch::dkg;
using Certificate    = Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using MuddleAddress  = byte_array::ConstByteArray;
using DkgOutput      = beacon::DkgOutput;

TEST(beacon_manager, dkg_and_threshold_signing)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  PublicKey zero;
  zero.clear();
  Generator generator_g, generator_h;
  SetGenerators(generator_g, generator_h);

  uint32_t                             cabinet_size = 3;
  uint32_t                             threshold    = 2;
  std::vector<std::shared_ptr<Prover>> member_ptrs;
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    std::shared_ptr<ECDSASigner> certificate = std::make_shared<ECDSASigner>();
    certificate->GenerateKeys();
    member_ptrs.emplace_back(certificate);
  }

  // Set up two honest beacon managers
  std::vector<std::shared_ptr<BeaconManager>> beacon_managers;
  std::vector<MuddleAddress>                  addresses;
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    beacon_managers.emplace_back(new BeaconManager(member_ptrs[index]));
    addresses.push_back(member_ptrs[index]->identity().identifier());
  }

  // Create cabinet and reset the beacon managers
  std::set<MuddleAddress> cabinet;
  for (auto &mem : member_ptrs)
  {
    cabinet.insert(mem->identity().identifier());
  }

  // Reset all managers
  for (auto &manager : beacon_managers)
  {
    manager->NewCabinet(cabinet, threshold);
  }

  // Check reset for one manager
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    std::shared_ptr<BeaconManager> manager = beacon_managers[index];
    EXPECT_EQ(
        manager->cabinet_index(),
        std::distance(cabinet.begin(), cabinet.find(member_ptrs[index]->identity().identifier())));
    for (auto &mem : member_ptrs)
    {
      EXPECT_EQ(manager->cabinet_index(mem->identity().identifier()),
                std::distance(cabinet.begin(), cabinet.find(mem->identity().identifier())));
    }
    EXPECT_TRUE(manager->qual().empty());
    EXPECT_TRUE(manager->group_public_key() == zero.getStr());
  }

  for (auto &manager : beacon_managers)
  {
    manager->GenerateCoefficients();
  }

  // Checks on coefficients and shares
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    std::shared_ptr<BeaconManager> manager      = beacon_managers[index];
    std::vector<std::string>       coefficients = manager->GetCoefficients();
    for (uint32_t elem = 0; elem < threshold; ++elem)
    {
      // Coefficients generated should be non-zero
      EXPECT_NE(coefficients[elem], zero.getStr());
      for (uint32_t index1 = 0; index1 < cabinet_size; ++index1)
      {
        if (index1 != index)
        {
          std::shared_ptr<BeaconManager> another_manager = beacon_managers[index1];
          std::vector<std::string>       coefficients1   = another_manager->GetCoefficients();
          // Coefficients should be different
          EXPECT_NE(coefficients, coefficients1);
          // Shares generated ourself are non-zero
          EXPECT_NE(manager->GetOwnShares(member_ptrs[index1]->identity().identifier()).first,
                    zero.getStr());
          EXPECT_NE(manager->GetOwnShares(member_ptrs[index1]->identity().identifier()).second,
                    zero.getStr());
          // Shares received from others should be zero
          EXPECT_EQ(manager->GetReceivedShares(member_ptrs[index1]->identity().identifier()).first,
                    zero.getStr());
          EXPECT_EQ(manager->GetReceivedShares(member_ptrs[index1]->identity().identifier()).second,
                    zero.getStr());
        }
      }
    }
  }

  // Name identifiers
  MuddleAddress my_address = addresses[0];
  MuddleAddress honest     = addresses[1];
  MuddleAddress malicious  = addresses[2];

  // Add shares and coefficients passing verification from someone and check that they are entered
  // in correctly
  beacon_managers[0]->AddShares(honest, beacon_managers[1]->GetOwnShares(my_address));
  beacon_managers[0]->AddCoefficients(honest, beacon_managers[1]->GetCoefficients());
  EXPECT_EQ(beacon_managers[0]->GetReceivedShares(honest),
            beacon_managers[1]->GetOwnShares(my_address));

  // Add shares and coefficients failing verification from malicious party
  PrivateKey  s_i;
  PrivateKey  sprime_i;
  std::string malicious_share1 = beacon_managers[2]->GetOwnShares(my_address).first;
  s_i.setStr(malicious_share1);
  // Modify one shares
  PrivateKey tmp;
  tmp.setRand();
  bn::Fr::add(tmp, tmp, s_i);
  std::pair<std::string, std::string> wrong_shares = {
      tmp.getStr(), beacon_managers[2]->GetOwnShares(my_address).second};

  beacon_managers[0]->AddShares(malicious, wrong_shares);
  beacon_managers[0]->AddCoefficients(malicious, beacon_managers[2]->GetCoefficients());
  EXPECT_EQ(beacon_managers[0]->GetReceivedShares(malicious), wrong_shares);

  std::set<MuddleAddress> coeff_received;
  coeff_received.insert(honest);
  coeff_received.insert(malicious);
  auto complaints = beacon_managers[0]->ComputeComplaints(coeff_received);

  std::unordered_set<MuddleAddress> complaints_expected = {malicious};
  EXPECT_EQ(complaints, complaints_expected);

  // Submit false complaints answer
  BeaconManager::ComplaintAnswer wrong_answer;
  wrong_answer.first  = my_address;
  wrong_answer.second = wrong_shares;
  EXPECT_FALSE(beacon_managers[0]->VerifyComplaintAnswer(malicious, wrong_answer));
  EXPECT_EQ(beacon_managers[0]->GetReceivedShares(malicious), wrong_shares);

  // Submit correct correct complaints answer and check values get replaced
  BeaconManager::ComplaintAnswer correct_answer;
  correct_answer.first  = my_address;
  correct_answer.second = beacon_managers[2]->GetOwnShares(my_address);
  EXPECT_TRUE(beacon_managers[0]->VerifyComplaintAnswer(malicious, correct_answer));
  EXPECT_EQ(beacon_managers[0]->GetReceivedShares(malicious),
            beacon_managers[2]->GetOwnShares(my_address));

  // Distribute correct shares and coefficients amongst everyone else
  for (uint32_t index = 1; index < cabinet_size; ++index)
  {
    for (uint32_t index1 = 0; index1 < cabinet_size; ++index1)
    {
      if (index1 != index)
      {
        beacon_managers[index]->AddShares(addresses[index1],
                                          beacon_managers[index1]->GetOwnShares(addresses[index]));
        beacon_managers[index]->AddCoefficients(addresses[index1],
                                                beacon_managers[index1]->GetCoefficients());
      }
    }
  }

  // Since bad shares have been replaced set qual to be everyone
  std::set<MuddleAddress> qual;
  for (auto &address : addresses)
  {
    qual.insert(address);
  }
  for (auto &manager : beacon_managers)
  {
    manager->SetQual(qual);
  }

  // Check computed secret shares
  for (auto &manager : beacon_managers)
  {
    manager->ComputeSecretShare();
    PrivateKey secret_key_test;
    secret_key_test.clear();
    for (auto &mem : qual)
    {
      PrivateKey share;
      share.setStr(manager->GetReceivedShares(mem).first);
      bn::Fr::add(secret_key_test, secret_key_test, share);
    }
    PrivateKey              computed_secret_key;
    PublicKey               dummy_public;
    std::vector<PublicKey>  dummy_public_shares;
    std::set<MuddleAddress> dummy_qual;
    DkgOutput               output{manager->GetDkgOutput()};
    EXPECT_EQ(output.private_key_share, secret_key_test);
  }

  // Add honest qual coefficients
  beacon_managers[0]->AddQualCoefficients(honest, beacon_managers[1]->GetQualCoefficients());

  // Verify qual coefficients before malicious submitted coefficients - expect complaint against
  // them
  BeaconManager::SharesExposedMap qual_complaints_test;
  qual_complaints_test.insert({malicious, beacon_managers[0]->GetReceivedShares(malicious)});
  auto actual_qual_complaints = beacon_managers[0]->ComputeQualComplaints({honest});
  EXPECT_EQ(actual_qual_complaints, qual_complaints_test);

  // Add wrong qual coefficients
  beacon_managers[0]->AddQualCoefficients(malicious, beacon_managers[1]->GetQualCoefficients());

  // Verify qual coefficients and check the complaints
  auto actual_qual_complaints1 = beacon_managers[0]->ComputeQualComplaints(coeff_received);
  EXPECT_EQ(actual_qual_complaints1, qual_complaints_test);

  // Share qual coefficients amongst other nodes
  for (uint32_t index = 1; index < cabinet_size; ++index)
  {
    for (uint32_t index1 = 0; index1 < cabinet_size; ++index1)
    {
      if (index1 != index)
      {
        beacon_managers[index]->AddQualCoefficients(addresses[index1],
                                                    beacon_managers[index1]->GetQualCoefficients());
      }
    }
  }

  // Invalid qual complaint
  BeaconManager::ComplaintAnswer incorrect_complaint = {
      honest, beacon_managers[1]->GetOwnShares(malicious)};
  EXPECT_EQ(malicious, beacon_managers[0]->VerifyQualComplaint(malicious, incorrect_complaint));
  // Qual complaint which fails first
  BeaconManager::ComplaintAnswer fail_check1 = {malicious, wrong_shares};
  EXPECT_EQ(honest, beacon_managers[0]->VerifyQualComplaint(honest, fail_check1));
  // Qual complaint which fails second check
  BeaconManager::ComplaintAnswer fail_check2 = {malicious,
                                                beacon_managers[1]->GetReceivedShares(malicious)};
  EXPECT_EQ(malicious, beacon_managers[0]->VerifyQualComplaint(honest, fail_check2));

  // Verify invalid reconstruction share
  BeaconManager::ComplaintAnswer incorrect_reconstruction_share = {honest, wrong_shares};
  beacon_managers[0]->VerifyReconstructionShare(malicious, incorrect_reconstruction_share);
  // Verify valid reconstruction share
  BeaconManager::ComplaintAnswer correct_reconstruction_share = {
      malicious, beacon_managers[1]->GetReceivedShares(malicious)};
  beacon_managers[0]->VerifyReconstructionShare(honest, correct_reconstruction_share);
  // Duplicate good reconstruction share
  beacon_managers[0]->VerifyReconstructionShare(honest, correct_reconstruction_share);

  // Run reconstruction with not enough shares
  EXPECT_FALSE(beacon_managers[0]->RunReconstruction());
  beacon_managers[0]->AddReconstructionShare(malicious);
  // Run reconstruction with enough shares
  EXPECT_TRUE(beacon_managers[0]->RunReconstruction());
  // Skip reconstruction for oneself
  BeaconManager::ComplaintAnswer my_reconstruction_share1 = {
      my_address, beacon_managers[0]->GetOwnShares(honest)};
  BeaconManager::ComplaintAnswer my_reconstruction_share2 = {
      my_address, beacon_managers[0]->GetOwnShares(malicious)};
  beacon_managers[0]->VerifyReconstructionShare(honest, my_reconstruction_share1);
  beacon_managers[0]->VerifyReconstructionShare(malicious, my_reconstruction_share2);
  EXPECT_TRUE(beacon_managers[0]->RunReconstruction());

  for (auto &manager : beacon_managers)
  {
    manager->ComputePublicKeys();
  }

  std::vector<DkgOutput> outputs;
  for (auto &manager : beacon_managers)
  {
    DkgOutput output{manager->GetDkgOutput()};
    outputs.push_back(output);
  }

  // Check outputs agree
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    for (uint32_t index1 = index + 1; index1 < cabinet_size; ++index1)
    {
      EXPECT_EQ(outputs[index].group_public_key.getStr(),
                outputs[index1].group_public_key.getStr());
      EXPECT_EQ(outputs[index].qual, outputs[index1].qual);
      for (uint32_t shares_index = 0; shares_index < cabinet_size; ++shares_index)
      {
        EXPECT_EQ(outputs[index].public_key_shares[shares_index],
                  outputs[index1].public_key_shares[shares_index]);
      }
    }
  }

  // Check threshold signing
  std::string                               message = "Hello";
  std::vector<BeaconManager::SignedMessage> signed_msgs;
  for (uint32_t index = 0; index < cabinet_size; ++index)
  {
    std::shared_ptr<BeaconManager> manager = beacon_managers[index];
    manager->SetMessage(message);
    BeaconManager::SignedMessage signed_msg = manager->Sign();
    signed_msgs.push_back(signed_msg);
  }

  // Add signature from unknown sender
  std::shared_ptr<ECDSASigner> unknown_sender = std::make_shared<ECDSASigner>();
  EXPECT_EQ(
      beacon_managers[0]->AddSignaturePart(unknown_sender->identity(), signed_msgs[1].signature),
      BeaconManager::AddResult::NOT_MEMBER);
  // Add invalid signature
  EXPECT_EQ(
      beacon_managers[0]->AddSignaturePart(member_ptrs[1]->identity(), signed_msgs[2].signature),
      BeaconManager::AddResult::INVALID_SIGNATURE);
  // Add valid signature
  EXPECT_EQ(
      beacon_managers[0]->AddSignaturePart(member_ptrs[1]->identity(), signed_msgs[1].signature),
      BeaconManager::AddResult::SUCCESS);
  // Duplicate valid signature
  EXPECT_EQ(
      beacon_managers[0]->AddSignaturePart(member_ptrs[1]->identity(), signed_msgs[1].signature),
      BeaconManager::AddResult::SIGNATURE_ALREADY_ADDED);

  // Compute signature with exactly enough shares
  EXPECT_TRUE(beacon_managers[0]->can_verify());
  EXPECT_TRUE(beacon_managers[0]->Verify());

  // Compute signature with more than require shares
  EXPECT_EQ(
      beacon_managers[0]->AddSignaturePart(member_ptrs[2]->identity(), signed_msgs[2].signature),
      BeaconManager::AddResult::SUCCESS);
  EXPECT_TRUE(beacon_managers[0]->Verify());

  // Check signatures of others with different combinations of signature shares
  EXPECT_EQ(
      beacon_managers[1]->AddSignaturePart(member_ptrs[2]->identity(), signed_msgs[2].signature),
      BeaconManager::AddResult::SUCCESS);
  EXPECT_TRUE(beacon_managers[1]->can_verify());
  EXPECT_TRUE(beacon_managers[1]->Verify());

  EXPECT_EQ(
      beacon_managers[2]->AddSignaturePart(member_ptrs[0]->identity(), signed_msgs[0].signature),
      BeaconManager::AddResult::SUCCESS);
  EXPECT_TRUE(beacon_managers[2]->can_verify());
  EXPECT_TRUE(beacon_managers[2]->Verify());
}
