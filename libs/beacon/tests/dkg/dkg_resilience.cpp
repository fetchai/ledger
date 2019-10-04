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

#include "beacon/beacon_setup_service.hpp"
#include "beacon/create_new_certificate.hpp"
#include "beacon/dkg_output.hpp"
#include "core/reactor.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"
#include "muddle/create_muddle_fake.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rbc.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace fetch;
using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;
using namespace fetch::dkg;
using namespace fetch::beacon;
using namespace fetch::ledger;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;
using ConstByteArray = fetch::byte_array::ConstByteArray;
using MuddleAddress  = ConstByteArray;

struct DummyManifestCache : public ManifestCacheInterface
{
  bool QueryManifest(Address const & /*address*/, Manifest & /*manifest*/) override
  {
    return false;
  }
};

class HonestSetupService : public BeaconSetupService
{
public:
  HonestSetupService(MuddleInterface &endpoint, const ProverPtr &prover,
                     ManifestCacheInterface &manifest_cache)
    : BeaconSetupService{endpoint, prover->identity(), manifest_cache, prover}
  {}
};

class FaultySetupService : public BeaconSetupService
{
public:
  enum class Failures : uint8_t
  {
    BAD_COEFFICIENT,
    SEND_MULTIPLE_MESSAGES,
    MESSAGES_WITH_UNKNOWN_ADDRESSES,
    MESSAGES_WITH_INVALID_CRYPTO,
    QUAL_MESSAGES_WITH_INVALID_CRYPTO,
    SEND_BAD_SHARE,
    SEND_EMPTY_COMPLAINT_ANSWER,
    BAD_QUAL_COEFFICIENTS,
    SEND_FALSE_QUAL_COMPLAINT,
    WITHOLD_RECONSTRUCTION_SHARES
  };

  FaultySetupService(MuddleInterface &endpoint, const ProverPtr &prover,
                     ManifestCacheInterface &     manifest_cache,
                     const std::vector<Failures> &failures = {})
    : BeaconSetupService{endpoint, prover->identity(), manifest_cache, prover}
  {
    for (auto f : failures)
    {
      failures_flags_.set(static_cast<uint32_t>(f));
    }
  }

private:
  std::bitset<static_cast<uint8_t>(Failures::WITHOLD_RECONSTRUCTION_SHARES) + 1> failures_flags_;

  bool Failure(Failures f) const
  {
    return failures_flags_[static_cast<uint8_t>(f)];
  }

  void SendShares(MuddleAddress const &                        destination,
                  std::pair<MessageShare, MessageShare> const &shares)
  {
    fetch::serializers::SizeCounter counter;
    counter << shares;

    fetch::serializers::MsgPackSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    endpoint_.Send(destination, SERVICE_DKG, CHANNEL_SECRET_KEY, serializer.data());
  }

  void BroadcastShares() override
  {
    if (Failure(Failures::SEND_BAD_SHARE))
    {
      beacon_->manager.GenerateCoefficients();
      SendBadShares();
    }
    else if (Failure(Failures::MESSAGES_WITH_INVALID_CRYPTO))
    {
      beacon_->manager.GenerateCoefficients();
      for (auto &cab_i : beacon_->aeon.members)
      {
        if (cab_i == identity_.identifier())
        {
          continue;
        }
        SendShares(cab_i, {"fake share", "fake share"});
      }
    }
    else
    {
      beacon_->manager.GenerateCoefficients();
      for (auto &cab_i : beacon_->aeon.members)
      {
        if (cab_i == identity_.identifier())
        {
          continue;
        }
        std::pair<MessageShare, MessageShare> shares{beacon_->manager.GetOwnShares(cab_i)};
        SendShares(cab_i, shares);
        if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
        {
          SendShares(cab_i, shares);
        }
      }
    }
    if (Failure(Failures::BAD_COEFFICIENT))
    {
      SendBadCoefficients();
    }
    else if (Failure(Failures::MESSAGES_WITH_INVALID_CRYPTO))
    {
      SendBroadcast(DKGEnvelope{
          CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES), {"fake coefficient"}}});
    }
    else
    {
      SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                                    beacon_->manager.GetCoefficients()}});
      if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
      {
        SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                                      beacon_->manager.GetCoefficients()}});
      }
    }
  }

  void SendBadCoefficients()
  {
    std::vector<std::string> coefficients;
    bn::G2                   fake;
    fake.clear();
    for (std::size_t k = 0; k <= beacon_->manager.polynomial_degree(); k++)
    {
      coefficients.push_back(fake.getStr());
    }
    // Send empty coefficients to everyone
    SendBroadcast(DKGEnvelope{
        CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES), coefficients}});
  }

  void SendBadShares()
  {
    bool sent_bad{false};
    for (auto &cab_i : beacon_->aeon.members)
    {
      if (cab_i == identity_.identifier())
      {
        continue;
      }
      // Send a one node trivial shares
      if (!sent_bad)
      {
        bn::Fr trivial_share;
        trivial_share.clear();
        std::pair<MessageShare, MessageShare> shares{trivial_share.getStr(),
                                                     trivial_share.getStr()};
        SendShares(cab_i, shares);
        sent_bad = true;
      }
      else
      {
        std::pair<MessageShare, MessageShare> shares{beacon_->manager.GetOwnShares(cab_i)};
        SendShares(cab_i, shares);
      }
    }
  }

  void BroadcastComplaints() override
  {
    std::unordered_set<MuddleAddress> complaints_local =
        beacon_->manager.ComputeComplaints(coefficients_received_);
    for (auto const &cab : complaints_local)
    {
      complaints_manager_.AddComplaintAgainst(cab);
    }
    if (Failure(Failures::MESSAGES_WITH_UNKNOWN_ADDRESSES))
    {
      complaints_local.insert("Unknown sender");
      SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local}});
    }
    else
    {
      SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local}});
    }
    if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
    {
      SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local}});
    }
  }

  void BroadcastComplaintAnswers() override
  {
    std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>> complaint_answers;
    if (Failure(Failures::MESSAGES_WITH_UNKNOWN_ADDRESSES))
    {
      complaint_answers.insert({"unknown reporter", {"fake share", "fake share2"}});
    }
    else if (Failure(Failures::MESSAGES_WITH_INVALID_CRYPTO))
    {
      for (auto const &reporter : complaints_manager_.ComplaintsAgainstSelf())
      {
        complaint_answers.insert({reporter, {"fake share", "fake share"}});
      }
    }
    else if (!Failure(Failures::SEND_EMPTY_COMPLAINT_ANSWER))
    {
      for (auto const &reporter : complaints_manager_.ComplaintsAgainstSelf())
      {
        complaint_answers.insert({reporter, beacon_->manager.GetOwnShares(reporter)});
      }
    }
    SendBroadcast(DKGEnvelope{SharesMessage{
        static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS), complaint_answers}});
    if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
    {
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS), complaint_answers}});
    }
  }

  void BroadcastQualCoefficients() override
  {
    if (Failure(Failures::BAD_QUAL_COEFFICIENTS))
    {
      std::vector<std::string> coefficients;
      bn::G2                   fake;
      fake.clear();
      for (std::size_t k = 0; k <= beacon_->manager.polynomial_degree(); k++)
      {
        coefficients.push_back(fake.getStr());
      }
      SendBroadcast(DKGEnvelope{
          CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES), coefficients}});
    }
    else if (Failure(Failures::QUAL_MESSAGES_WITH_INVALID_CRYPTO))
    {
      beacon_->manager.GetQualCoefficients();
      SendBroadcast(DKGEnvelope{CoefficientsMessage{
          static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES), {"fake coefficients"}}});
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                          beacon_->manager.GetQualCoefficients()}});
      if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
      {
        SendBroadcast(
            DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                            beacon_->manager.GetQualCoefficients()}});
      }
    }

    qual_coefficients_received_.insert(identity_.identifier());
  }

  void BroadcastQualComplaints() override
  {
    if (Failure(Failures::SEND_FALSE_QUAL_COMPLAINT))
    {
      auto victim = beacon_->aeon.members.begin();
      if (*victim == identity_.identifier())
      {
        ++victim;
      }
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                                    {{*victim, beacon_->manager.GetReceivedShares(*victim)}}}});
    }
    else if (Failure(Failures::MESSAGES_WITH_UNKNOWN_ADDRESSES))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                                    {{"unknown sender", {"fake share", "fake share"}}}}});
    }
    else if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS), {}}});
    }
    else if (Failure(Failures::QUAL_MESSAGES_WITH_INVALID_CRYPTO))
    {
      auto victim = beacon_->aeon.members.begin();
      if (*victim == identity_.identifier())
      {
        ++victim;
      }
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                                    {{*victim, {"fake share", "fake share"}}}}});
    }
    else
    {
      SendBroadcast(DKGEnvelope{
          SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                        beacon_->manager.ComputeQualComplaints(qual_coefficients_received_)}});
      if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
      {
        SendBroadcast(DKGEnvelope{
            SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                          beacon_->manager.ComputeQualComplaints(qual_coefficients_received_)}});
      }
    }
  }

  void BroadcastReconstructionShares() override
  {
    SharesExposedMap complaint_shares;
    if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
    }
    else if (Failure(Failures::MESSAGES_WITH_UNKNOWN_ADDRESSES))
    {
      complaint_shares.insert({"unknown address", {"fake share", "fake share1"}});
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
    }
    else if (Failure(Failures::QUAL_MESSAGES_WITH_INVALID_CRYPTO))
    {
      for (auto const &in : qual_complaints_manager_.Complaints())
      {
        beacon_->manager.AddReconstructionShare(in);
        complaint_shares.insert({in, {"fake share", "fake share"}});
      }
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
    }
    else
    {
      for (auto const &in : qual_complaints_manager_.Complaints())
      {
        beacon_->manager.AddReconstructionShare(in);
        complaint_shares.insert({in, beacon_->manager.GetReceivedShares(in)});
      }
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
      if (Failure(Failures::SEND_MULTIPLE_MESSAGES))
      {
        SendBroadcast(DKGEnvelope{SharesMessage{
            static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
      }
    }
  }
};

struct DkgMember
{
  const static uint16_t CHANNEL_SHARES = 3;

  uint16_t             muddle_port;
  NetworkManager       network_manager;
  fetch::core::Reactor reactor;
  ProverPtr            muddle_certificate;
  MuddlePtr            muddle;

  // Set when DKG is finished
  DkgOutput output;
  bool      finished{false};

  DkgMember(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1")}
  {
    network_manager.Start();
    muddle->Start({muddle_port});
  }

  virtual ~DkgMember()
  {
    reactor.Stop();
    muddle->Stop();
    network_manager.Stop();
  }

  virtual void QueueCabinet(std::set<MuddleAddress> cabinet, uint32_t threshold) = 0;
  virtual std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables()          = 0;
  virtual bool                                       DkgFinished()               = 0;
};

struct FaultyDkgMember : DkgMember
{
  using SharedAeonExecutionUnit = BeaconSetupService::SharedAeonExecutionUnit;
  DummyManifestCache manifest_cache;
  FaultySetupService dkg;

  FaultyDkgMember(uint16_t port_number, uint16_t index,
                  const std::vector<FaultySetupService::Failures> &failures = {})
    : DkgMember{port_number, index}
    , dkg{*muddle, muddle_certificate, manifest_cache, failures}
  {
    dkg.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) -> void {
      finished = true;
      output   = beacon->manager.GetDkgOutput();
    });
  }

  ~FaultyDkgMember() override
  {
    reactor.Stop();
  }

  void QueueCabinet(std::set<MuddleAddress> cabinet, uint32_t threshold) override
  {
    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    beacon->manager.SetCertificate(muddle_certificate);
    beacon->manager.NewCabinet(cabinet, threshold);

    // Setting the aeon details
    beacon->aeon.round_start = 0;
    beacon->aeon.round_end   = 10;
    beacon->aeon.members     = std::move(cabinet);
    // Plus 5 so tests pass on first DKG attempt
    beacon->aeon.start_reference_timepoint = static_cast<uint64_t>(std::time(nullptr)) + 5;

    dkg.QueueSetup(beacon);
  }

  std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables() override
  {
    return dkg.GetWeakRunnables();
  }
  bool DkgFinished() override
  {
    return finished;
  }
};

struct HonestDkgMember : DkgMember
{
  using SharedAeonExecutionUnit = BeaconSetupService::SharedAeonExecutionUnit;
  DummyManifestCache manifest_cache;
  HonestSetupService dkg;

  HonestDkgMember(uint16_t port_number, uint16_t index)
    : DkgMember{port_number, index}
    , dkg{*muddle, muddle_certificate, manifest_cache}
  {
    dkg.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) -> void {
      finished = true;
      output   = beacon->manager.GetDkgOutput();
    });
  }

  ~HonestDkgMember() override
  {
    reactor.Stop();
  }

  void QueueCabinet(std::set<MuddleAddress> cabinet, uint32_t threshold) override
  {
    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    beacon->manager.SetCertificate(muddle_certificate);
    beacon->manager.NewCabinet(cabinet, threshold);

    // Setting the aeon details
    beacon->aeon.round_start = 0;
    beacon->aeon.round_end   = 10;
    beacon->aeon.members     = std::move(cabinet);
    // Plus 5 so tests pass on first DKG attempt
    beacon->aeon.start_reference_timepoint = static_cast<uint64_t>(std::time(nullptr)) + 5;

    dkg.QueueSetup(beacon);
  }

  std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables() override
  {
    return dkg.GetWeakRunnables();
  }
  bool DkgFinished() override
  {
    return finished;
  }
};

void GenerateTest(uint32_t cabinet_size, uint32_t threshold, uint32_t qual_size,
                  uint32_t expected_completion_size,
                  const std::vector<std::vector<FaultySetupService::Failures>> &failures    = {},
                  uint16_t                                                      setup_delay = 0)
{
  std::set<MuddleAddress>                                             cabinet;
  std::vector<std::unique_ptr<DkgMember>>                             committee;
  std::set<RBC::MuddleAddress>                                        expected_qual;
  std::unordered_map<byte_array::ConstByteArray, fetch::network::Uri> peers_list;
  std::set<uint32_t>                                                  qual_index;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    if (ii < failures.size() && !failures[ii].empty())
    {
      committee.emplace_back(new FaultyDkgMember{port_number, ii, failures[ii]});
    }
    else
    {
      committee.emplace_back(new HonestDkgMember{port_number, ii});
    }
    if (ii >= (cabinet_size - qual_size))
    {
      expected_qual.insert(committee[ii]->muddle->GetAddress());
      qual_index.insert(ii);
    }
    peers_list.insert({committee[ii]->muddle_certificate->identity().identifier(),
                       fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(port_number)}});
    cabinet.insert(committee[ii]->muddle_certificate->identity().identifier());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // Reset cabinet for rbc in pre-dkg sync
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    committee[ii]->QueueCabinet(cabinet, threshold);
  }

  // Start off some connections until everyone else has connected
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      committee[ii]->muddle->ConnectTo(committee[jj]->muddle->GetAddress(),
                                       peers_list[committee[jj]->muddle->GetAddress()]);
    }
  }

  // Start at DKG
  {
    for (auto &member : committee)
    {
      member->reactor.Attach(member->GetWeakRunnables());
    }

    for (auto &member : committee)
    {
      member->reactor.Start();
      std::this_thread::sleep_for(std::chrono::milliseconds(setup_delay));
    }

    // Loop until everyone we expect to finish completes the DKG
    uint32_t pp = cabinet_size - expected_completion_size;
    while (pp < cabinet_size)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      for (auto qq = pp; qq < cabinet_size; ++qq)
      {
        if (!committee[qq]->DkgFinished())
        {
          break;
        }

        ++pp;
      }
    }

    // Check everyone in qual agrees on qual
    uint32_t start_qual = cabinet_size - expected_completion_size;
    for (uint32_t nn = start_qual + 1; nn < cabinet_size; ++nn)
    {
      EXPECT_EQ(committee[start_qual]->output.qual, expected_qual);
    }

    // Check DKG is working correctly for everyone who completes the DKG successfully
    uint32_t start_complete = cabinet_size - expected_completion_size;
    for (uint32_t nn = start_complete + 1; nn < cabinet_size; ++nn)
    {
      EXPECT_EQ(committee[start_complete]->output.group_public_key,
                committee[nn]->output.group_public_key);
      EXPECT_EQ(committee[start_complete]->output.public_key_shares,
                committee[nn]->output.public_key_shares);
      EXPECT_NE(committee[start_complete]->output.public_key_shares[start_complete],
                committee[nn]->output.public_key_shares[nn]);
      for (uint32_t qq = nn + 1; qq < cabinet_size; ++qq)
      {
        EXPECT_NE(committee[start_complete]->output.public_key_shares[nn],
                  committee[start_complete]->output.public_key_shares[qq]);
      }
    }
  }
}

TEST(dkg_setup, DISABLED_bad_messages)
{
  // Node 0 sends pre-qual messages with invalid crypto - is excluded from qual.
  // Another node sends certain messages with unknown member in it. Ignored and not excluded.
  // Finally, a third node enters qual but then sends qual messages with incorrect crypto -
  // fails the dkg as it receives threshold number of complaints
  GenerateTest(7, 4, 6, 5,
               {{FaultySetupService::Failures::MESSAGES_WITH_INVALID_CRYPTO},
                {FaultySetupService::Failures::QUAL_MESSAGES_WITH_INVALID_CRYPTO},
                {FaultySetupService::Failures::MESSAGES_WITH_UNKNOWN_ADDRESSES}});
}

// disabled due to timeouts TODO(HUT): fix
TEST(dkg_setup, DISABLED_send_empty_complaint_answer)
{
  // Node 0 sends computes bad secret shares to Node 1 which complains against it.
  // Node 0 then does not send real shares and instead sends empty complaint answer.
  // Node 0 should be disqualified from qual. A different node sends bad secret shares
  // but then reveals correct shares in complaint answer so is allowed into qual.
  GenerateTest(4, 3, 3, 3,
               {{FaultySetupService::Failures::SEND_BAD_SHARE,
                 FaultySetupService::Failures::SEND_EMPTY_COMPLAINT_ANSWER},
                {FaultySetupService::Failures::SEND_BAD_SHARE}});
}

TEST(dkg_setup, DISABLED_send_multiple_messages)
{
  // Node 0 broadcasts bad coefficients which fails verification by everyone and is
  // rejected from qual. Another node sends multiple of each DKG message but should succeed in DKG.
  // A third node sends fake qual coefficients. Should trigger warning and this node's shares will
  // be reconstructed but should succeed in the DKG. Thirs behavious important to test as it means
  // reconstruction computes the correct thing.
  GenerateTest(5, 3, 4, 4,
               {{FaultySetupService::Failures::BAD_COEFFICIENT},
                {FaultySetupService::Failures::SEND_MULTIPLE_MESSAGES},
                {FaultySetupService::Failures::SEND_FALSE_QUAL_COMPLAINT}});
}
