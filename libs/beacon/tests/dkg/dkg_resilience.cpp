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
#include "core/reactor.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/shards/manifest.hpp"
#include "ledger/shards/manifest_cache_interface.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rbc.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

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

struct DummyManifestCache : public ManifestCacheInterface
{
  bool QueryManifest(Address const &, Manifest &) override
  {
    return false;
  }
};

class HonestSetupService : public BeaconSetupService
{
public:
  HonestSetupService(MuddleInterface &endpoint, Identity identity,
                     ManifestCacheInterface &manifest_cache)
    : BeaconSetupService{endpoint, std::move(identity), manifest_cache}
  {}
};

class FaultySetupService : public BeaconSetupService
{
public:
  enum class Failures : uint8_t
  {
    BAD_COEFFICIENT,
    SEND_MULTIPLE_COEFFICIENTS,
    SEND_BAD_SHARE,
    SEND_MULTIPLE_SHARES,
    SEND_MULTIPLE_COMPLAINTS,
    SEND_EMPTY_COMPLAINT_ANSWER,
    SEND_MULTIPLE_COMPLAINT_ANSWERS,
    BAD_QUAL_COEFFICIENTS,
    SEND_MULTIPLE_QUAL_COEFFICIENTS,
    SEND_FALSE_QUAL_COMPLAINT,
    SEND_MULTIPLE_RECONSTRUCTION_SHARES,
    WITHOLD_RECONSTRUCTION_SHARES
  };

  FaultySetupService(MuddleInterface &endpoint, Identity identity,
                     ManifestCacheInterface &     manifest_cache,
                     const std::vector<Failures> &failures = {})
    : BeaconSetupService{endpoint, std::move(identity), manifest_cache}
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
    else
    {
      beacon_->manager.GenerateCoefficients();
      for (auto &cab_i : beacon_->aeon.members)
      {
        if (cab_i == identity_)
        {
          continue;
        }
        std::pair<MessageShare, MessageShare> shares{
            beacon_->manager.GetOwnShares(cab_i.identifier())};
        SendShares(cab_i.identifier(), shares);
        if (Failure(Failures::SEND_MULTIPLE_SHARES))
        {
          SendShares(cab_i.identifier(), shares);
        }
      }
    }
    if (Failure(Failures::BAD_COEFFICIENT))
    {
      SendBadCoefficients();
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                          beacon_->manager.GetCoefficients(), "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_COEFFICIENTS))
      {
        SendBroadcast(
            DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                            beacon_->manager.GetCoefficients(), "signature"}});
      }
    }
  }

  void SendBadCoefficients()
  {
    std::vector<std::string> coefficients;
    bn::G2                   fake;
    fake.clear();
    for (size_t k = 0; k <= beacon_->manager.polynomial_degree(); k++)
    {
      coefficients.push_back(fake.getStr());
    }
    // Send empty coefficients to everyone
    SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                                  coefficients, "signature"}});
  }

  void SendBadShares()
  {
    bool sent_bad{false};
    for (auto &cab_i : beacon_->aeon.members)
    {
      if (cab_i == identity_)
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
        SendShares(cab_i.identifier(), shares);
        sent_bad = true;
      }
      else
      {
        std::pair<MessageShare, MessageShare> shares{
            beacon_->manager.GetOwnShares(cab_i.identifier())};
        SendShares(cab_i.identifier(), shares);
      }
    }
  }

  void BroadcastComplaints() override
  {
    std::unordered_set<MuddleAddress> complaints_local = beacon_->manager.ComputeComplaints();
    for (auto const &cab : complaints_local)
    {
      complaints_manager_.Count(cab);
    }
    SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
    if (Failure(Failures::SEND_MULTIPLE_COMPLAINTS))
    {
      SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
    }
  }

  void BroadcastComplaintsAnswer() override
  {
    std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>> complaints_answer;
    if (!Failure(Failures::SEND_EMPTY_COMPLAINT_ANSWER))
    {
      for (auto const &reporter : complaints_manager_.ComplaintsFrom())
      {
        complaints_answer.insert({reporter, beacon_->manager.GetOwnShares(reporter)});
      }
    }
    SendBroadcast(DKGEnvelope{SharesMessage{
        static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS), complaints_answer, "signature"}});
    if (Failure(Failures::SEND_MULTIPLE_COMPLAINT_ANSWERS))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS),
                                    complaints_answer, "signature"}});
    }
  }

  void BroadcastQualCoefficients() override
  {
    if (Failure(Failures::BAD_QUAL_COEFFICIENTS))
    {
      std::vector<std::string> coefficients;
      bn::G2                   fake;
      fake.clear();
      for (size_t k = 0; k <= beacon_->manager.polynomial_degree(); k++)
      {
        coefficients.push_back(fake.getStr());
      }
      SendBroadcast(DKGEnvelope{CoefficientsMessage{
          static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES), coefficients, "signature"}});
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                          beacon_->manager.GetQualCoefficients(), "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS))
      {
        SendBroadcast(
            DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                            beacon_->manager.GetQualCoefficients(), "signature"}});
      }
    }

    complaints_answer_manager_.Clear();
    qual_coefficients_received_.insert(identity_.identifier());
  }

  void BroadcastQualComplaints() override
  {
    if (Failure(Failures::SEND_FALSE_QUAL_COMPLAINT))
    {
      auto victim = beacon_->aeon.members.begin();
      if (*victim == identity_)
      {
        ++victim;
      }
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
          {{victim->identifier(), beacon_->manager.GetReceivedShares(victim->identifier())}},
          "signature"}});
    }
    else if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(DKGEnvelope{
          SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS), {}, "signature"}});
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                                    beacon_->manager.ComputeQualComplaints(), "signature"}});
    }
  }

  void BroadcastReconstructionShares() override
  {
    SharesExposedMap complaint_shares;
    if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES),
                                    complaint_shares, "signature"}});
    }
    else
    {

      for (auto const &in : qual_complaints_manager_.Complaints())
      {
        beacon_->manager.AddReconstructionShare(in);
        complaint_shares.insert({in, beacon_->manager.GetReceivedShares(in)});
      }
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES),
                                    complaint_shares, "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_RECONSTRUCTION_SHARES))
      {
        SendBroadcast(
            DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES),
                                      complaint_shares, "signature"}});
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
  bn::Fr              secret_share;
  bn::G2              public_key;
  RBC::CabinetMembers qual_set;
  std::vector<bn::G2> public_key_shares{};
  bool                finished{false};

  DkgMember(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
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

  virtual void QueueCabinet(std::set<Identity> cabinet, uint32_t threshold) = 0;
  virtual std::weak_ptr<core::Runnable> GetWeakRunnable()                   = 0;
  virtual bool                          DkgFinished()                       = 0;

  static ProverPtr CreateNewCertificate()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }
};

struct FaultyDkgMember : DkgMember
{
  using SharedAeonExecutionUnit = BeaconSetupService::SharedAeonExecutionUnit;
  DummyManifestCache manifest_cache;
  FaultySetupService dkg;

  FaultyDkgMember(uint16_t port_number, uint16_t index,
                  const std::vector<FaultySetupService::Failures> &failures = {})
    : DkgMember{port_number, index}
    , dkg{*muddle, muddle_certificate->identity(), manifest_cache, failures}
  {
    dkg.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) -> void {
      finished = true;
      beacon->manager.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
    });
  }

  ~FaultyDkgMember() override = default;

  void QueueCabinet(std::set<Identity> cabinet, uint32_t threshold) override
  {
    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    // Determines if we are observing or actively participating
    if (cabinet.find(muddle_certificate->identity()) == cabinet.end())
    {
      beacon->observe_only = true;
    }
    else
    {
      beacon->manager.SetCertificate(muddle_certificate);
      beacon->manager.Reset(cabinet, threshold);
    }

    // Setting the aeon details
    beacon->aeon.round_start               = 0;
    beacon->aeon.round_end                 = 10;
    beacon->aeon.members                   = std::move(cabinet);
    beacon->aeon.start_reference_timepoint = static_cast<uint64_t>(std::time(nullptr));

    // Even "observe only" details need to pass through the setup phase
    // to preserve order.
    dkg.QueueSetup(beacon);
  }

  std::weak_ptr<core::Runnable> GetWeakRunnable() override
  {
    return dkg.GetWeakRunnable();
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
    , dkg{*muddle, muddle_certificate->identity(), manifest_cache}
  {
    dkg.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) -> void {
      finished = true;
      beacon->manager.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
    });
  }

  ~HonestDkgMember() override = default;

  void QueueCabinet(std::set<Identity> cabinet, uint32_t threshold) override
  {
    SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

    // Determines if we are observing or actively participating
    if (cabinet.find(muddle_certificate->identity()) == cabinet.end())
    {
      beacon->observe_only = true;
    }
    else
    {
      beacon->manager.SetCertificate(muddle_certificate);
      beacon->manager.Reset(cabinet, threshold);
    }

    // Setting the aeon details
    beacon->aeon.round_start               = 0;
    beacon->aeon.round_end                 = 10;
    beacon->aeon.members                   = std::move(cabinet);
    beacon->aeon.start_reference_timepoint = static_cast<uint64_t>(std::time(nullptr));

    // Even "observe only" details need to pass through the setup phase
    // to preserve order.
    dkg.QueueSetup(beacon);
  }

  std::weak_ptr<core::Runnable> GetWeakRunnable() override
  {
    return dkg.GetWeakRunnable();
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
  std::set<Identity>                                                  cabinet_identities;
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
    cabinet_identities.insert(committee[ii]->muddle_certificate->identity());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // Reset cabinet for rbc in pre-dkg sync
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    committee[ii]->QueueCabinet(cabinet_identities, threshold);
  }

  // Wait until everyone else has connected
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      committee[ii]->muddle->ConnectTo(committee[jj]->muddle->GetAddress(),
                                       peers_list[committee[jj]->muddle->GetAddress()]);
    }
  }

  uint32_t kk = 0;
  while (kk < cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (committee[mm]->muddle->GetEndpoint().GetDirectlyConnectedPeers().size() !=
          cabinet_size - 1)
      {
        break;
      }
      else
      {
        ++kk;
      }
    }
  }

  // Start at DKG
  {
    for (auto &member : committee)
    {
      member->reactor.Attach(member->GetWeakRunnable());
    }

    for (auto &member : committee)
    {
      member->reactor.Start();
      std::this_thread::sleep_for(std::chrono::milliseconds(setup_delay));
    }

    // Loop until everyone is finished with DKG
    uint32_t pp = 0;
    while (pp < cabinet_size)
    {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      for (auto qq = pp; qq < cabinet_size; ++qq)
      {
        if (!committee[qq]->DkgFinished())
        {
          break;
        }
        else
        {
          ++pp;
        }
      }
    }

    // Check everyone in qual agrees on qual
    uint32_t start_qual = cabinet_size - qual_size;
    for (uint32_t nn = start_qual + 1; nn < cabinet_size; ++nn)
    {
      EXPECT_EQ(committee[start_qual]->qual_set, expected_qual);
    }

    // Check DKG is working correctly for everyone who completes the DKG successfully
    uint32_t start_complete = cabinet_size - expected_completion_size;
    for (uint32_t nn = start_complete + 1; nn < cabinet_size; ++nn)
    {
      EXPECT_EQ(committee[start_complete]->public_key, committee[nn]->public_key);
      EXPECT_EQ(committee[start_complete]->public_key_shares, committee[nn]->public_key_shares);
      EXPECT_NE(committee[start_complete]->public_key_shares[start_complete],
                committee[nn]->public_key_shares[nn]);
      for (uint32_t qq = nn + 1; qq < cabinet_size; ++qq)
      {
        EXPECT_NE(committee[start_complete]->public_key_shares[nn],
                  committee[start_complete]->public_key_shares[qq]);
      }
    }
  }
}

TEST(dkg_setup, small_scale_test)
{
  GenerateTest(4, 3, 4, 4);
}

TEST(dkg_setup, send_bad_share)
{
  // Node 0 sends bad secret shares to Node 1 which complains against it.
  // Node 0 then broadcasts its real shares as defense and then is allowed into
  // qual
  GenerateTest(4, 3, 4, 4, {{FaultySetupService::Failures::SEND_BAD_SHARE}});
}

// TODO(HUT): rework disabled tests with the addition of time-outs
TEST(dkg_setup, DISABLED_bad_coefficients)
{
  // Node 0 broadcasts bad coefficients which fails verification by everyone.
  // Rejected from qual
  GenerateTest(4, 3, 3, 3, {{FaultySetupService::Failures::BAD_COEFFICIENT}});
}

TEST(dkg_setup, DISABLED_send_empty_complaints_answer)
{
  // Node 0 sends computes bad secret shares to Node 1 which complains against it.
  // Node 0 then does not send real shares and instead sends empty complaint answer.
  // Node 0 should be disqualified from qual
  GenerateTest(4, 3, 3, 3,
               {{FaultySetupService::Failures::SEND_BAD_SHARE,
                 FaultySetupService::Failures::SEND_EMPTY_COMPLAINT_ANSWER}});
}

TEST(dkg_setup, send_multiple_messages)
{
  // Node 0 sends multiple coefficients, complaint, complaint answers and qual coefficient messages.
  // Everyone should succeed in DKG
  GenerateTest(4, 3, 4, 4,
               {{FaultySetupService::Failures::SEND_MULTIPLE_SHARES,
                 FaultySetupService::Failures::SEND_MULTIPLE_COEFFICIENTS,
                 FaultySetupService::Failures::SEND_MULTIPLE_COMPLAINTS,
                 FaultySetupService::Failures::SEND_MULTIPLE_COMPLAINT_ANSWERS,
                 FaultySetupService::Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, DISABLED_qual_below_threshold)
{
  GenerateTest(4, 3, 2, 0,
               {{FaultySetupService::Failures::BAD_COEFFICIENT},
                {FaultySetupService::Failures::BAD_COEFFICIENT}});
}

TEST(dkg_setup, DISABLED_bad_qual_coefficients)
{
  // Node 0 computes bad qual coefficients so node 0 is in qual complaints but everyone reconstructs
  // their shares. Everyone else except node 0 succeeds in DKG
  GenerateTest(4, 3, 4, 3, {{FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, DISABLED_send_fake_qual_complaint)
{
  // Node 0 sends fake qual coefficients. Should trigger warning and node 0's shares will be
  // reconstructed but everyone else should succeed in the DKG. Important test as it means
  // reconstruction computes the correct thing.
  GenerateTest(4, 3, 4, 4, {{FaultySetupService::Failures::SEND_FALSE_QUAL_COMPLAINT}});
}

TEST(dkg_setup, too_many_bad_qual_coefficients)
{
  // Three nodes send bad qual coefficients which means that there are
  // not enough parties not in complaints. DKG fails
  GenerateTest(4, 2, 4, 0,
               {{FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, send_multiple_reconstruction_shares)
{
  // Node sends multiple reconstruction shares which triggers warning but
  // DKG succeeds
  GenerateTest(4, 3, 4, 3,
               {{FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultySetupService::Failures::SEND_MULTIPLE_RECONSTRUCTION_SHARES}});
}

TEST(dkg_setup, withold_reconstruction_shares)
{
  // Node 0 sends bad qual coefficients and another in collusion does not broadcast node 0's shares
  // so there are not enough shares to run reconstruction
  GenerateTest(4, 3, 4, 0,
               {{FaultySetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultySetupService::Failures::WITHOLD_RECONSTRUCTION_SHARES}});
}

TEST(dkg_setup, long_delay_between_starts)
{
  // Start nodes with delay (ms) between starting service
  SetGlobalLogLevel(LogLevel::TRACE);
  GenerateTest(4, 3, 4, 4, {}, 1000);
}
