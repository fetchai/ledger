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

#include "core/reactor.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/dkg_setup_service.hpp"
#include "dkg/pre_dkg_sync.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rbc.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace fetch;
using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;
using namespace fetch::dkg;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;
using ConstByteArray = fetch::byte_array::ConstByteArray;

class FaultyDkgSetupService : public DkgSetupService
{
public:
  enum class Failures : uint8_t
  {
    BAD_COEFFICIENT,
    SEND_MULTIPLE_COEFFICIENTS,
    SEND_BAD_SHARE,
    SEND_MULTIPLE_COMPLAINTS,
    SEND_EMPTY_COMPLAINT_ANSWER,
    SEND_MULTIPLE_COMPLAINT_ANSWERS,
    BAD_QUAL_COEFFICIENTS,
    SEND_MULTIPLE_QUAL_COEFFICIENTS,
    SEND_FALSE_QUAL_COMPLAINT,
    SEND_MULTIPLE_RECONSTRUCTION_SHARES,
    WITHOLD_RECONSTRUCTION_SHARES
  };
  FaultyDkgSetupService(
      MuddleAddress address, std::function<void(DKGEnvelope const &)> broadcast_callback,
      std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
                                   rpc_callback,
      const std::vector<Failures> &failures = {})
    : DkgSetupService{std::move(address), std::move(broadcast_callback), std::move(rpc_callback)}
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

  void BroadcastShares() override
  {
    if (Failure(Failures::SEND_BAD_SHARE))
    {
      manager_.GenerateCoefficients();
      SendBadShares();
    }
    else
    {
      manager_.GenerateCoefficients();
      for (auto &cab_i : cabinet_)
      {
        if (cab_i != address_)
        {
          std::pair<MessageShare, MessageShare> shares{manager_.GetOwnShares(cab_i)};
          rpc_function_(cab_i, shares);
        }
      }
    }
    if (Failure(Failures::BAD_COEFFICIENT))
    {
      SendBadCoefficients();
    }
    else
    {
      SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                                    manager_.GetCoefficients(), "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_COEFFICIENTS))
      {
        SendBroadcast(
            DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                            manager_.GetCoefficients(), "signature"}});
      }
    }
  }

  void SendBadCoefficients()
  {
    std::vector<std::string> coefficients;
    bn::G2                   fake;
    fake.clear();
    for (size_t k = 0; k <= manager_.polynomial_degree(); k++)
    {
      coefficients.push_back(fake.getStr());
    }
    // Send empty coefficients to everyone
    SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                                  coefficients, "signature"}});
  }

  void SendBadShares()
  {
    bool sent_bad{false};
    for (auto &cab_i : cabinet_)
    {
      if (cab_i != address_)
      {
        // Send a one node trivial shares
        if (!sent_bad)
        {
          bn::Fr trivial_share;
          trivial_share.clear();
          std::pair<MessageShare, MessageShare> shares{trivial_share.getStr(),
                                                       trivial_share.getStr()};
          rpc_function_(cab_i, shares);
          sent_bad = true;
        }
        else
        {
          std::pair<MessageShare, MessageShare> shares{manager_.GetOwnShares(cab_i)};
          rpc_function_(cab_i, shares);
        }
      }
    }
  }

  void BroadcastComplaints() override
  {
    std::unordered_set<MuddleAddress> complaints_local = manager_.ComputeComplaints();
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
        complaints_answer.insert({reporter, manager_.GetOwnShares(reporter)});
      }
    }
    SendBroadcast(
        DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS),
                                  complaints_answer, "signature"}});
    if (Failure(Failures::SEND_MULTIPLE_COMPLAINT_ANSWERS))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS),
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
      for (size_t k = 0; k <= manager_.polynomial_degree(); k++)
      {
        coefficients.push_back(fake.getStr());
      }
      SendBroadcast(DKGEnvelope{CoefficientsMessage{
          static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES), coefficients, "signature"}});
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES),
                                          manager_.GetQualCoefficients(), "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS))
      {
        SendBroadcast(
            DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES),
                                            manager_.GetQualCoefficients(), "signature"}});
      }
    }

    complaints_answer_manager_.Clear();
    {
      std::unique_lock<std::mutex> lock{mutex_};
      A_ik_received_.insert(address_);
      lock.unlock();
    }
  }

  void BroadcastQualComplaints() override
  {
    if (Failure(Failures::SEND_FALSE_QUAL_COMPLAINT))
    {
      auto victim = cabinet_.begin();
      if (*victim == address_)
      {
        ++victim;
      }
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS),
                                    {{*victim, manager_.GetReceivedShares(*victim)}},
                                    "signature"}});
    }
    else if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS), {}, "signature"}});
    }
    else
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS),
                                    manager_.ComputeQualComplaints(qual_), "signature"}});
    }
  }

  void BroadcastReconstructionShares() override
  {
    SharesExposedMap complaint_shares;
    if (Failure(Failures::WITHOLD_RECONSTRUCTION_SHARES))
    {
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES),
                                    complaint_shares, "signature"}});
    }
    else
    {

      for (auto const &in : qual_complaints_manager_.Complaints())
      {
        assert(qual_.find(in) != qual_.end());
        manager_.AddReconstructionShare(in);
        complaint_shares.insert({in, manager_.GetReceivedShares(in)});
      }
      SendBroadcast(
          DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES),
                                    complaint_shares, "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_RECONSTRUCTION_SHARES))
      {
        SendBroadcast(DKGEnvelope{
            SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES),
                          complaint_shares, "signature"}});
      }
    }
  }
};

struct DkgMember
{
  const static uint16_t CHANNEL_SHARES = 3;

  uint16_t                              muddle_port;
  NetworkManager                        network_manager;
  fetch::core::Reactor                  reactor;
  ProverPtr                             muddle_certificate;
  Muddle                                muddle;
  std::shared_ptr<muddle::Subscription> shares_subscription;
  RBC                                   rbc;
  PreDkgSync                            pre_sync;

  // Set when DKG is finished
  bn::Fr              secret_share;
  bn::G2              public_key;
  RBC::CabinetMembers qual_set;
  std::vector<bn::G2> public_key_shares;

  DkgMember(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager}
    , shares_subscription(muddle.AsEndpoint().Subscribe(SERVICE_DKG, CHANNEL_SHARES))
    , rbc{muddle.AsEndpoint(), muddle_certificate->identity().identifier(),
          [this](ConstByteArray const &address, ConstByteArray const &payload) -> void {
            DKGEnvelope   env;
            DKGSerializer serializer{payload};
            serializer >> env;
            OnDkgMessage(address, env.Message());
          }}
    , pre_sync{muddle, 4}
  {
    // Set subscription for receiving shares
    shares_subscription->SetMessageHandler([this](ConstByteArray const &from, uint16_t, uint16_t,
                                                  uint16_t, muddle::Packet::Payload const &payload,
                                                  ConstByteArray) {
      fetch::serializers::MsgPackSerializer serialiser(payload);

      std::pair<std::string, std::string> shares;
      serialiser >> shares;

      // Dispatch the event
      OnNewShares(from, shares);
    });

    network_manager.Start();
    muddle.Start({muddle_port});
  }

  virtual ~DkgMember()
  {
    reactor.Stop();
    muddle.Stop();
    muddle.Shutdown();
    network_manager.Stop();
  }

  void SubmitShare(ConstByteArray const &                     destination,
                   std::pair<std::string, std::string> const &shares)
  {
    fetch::serializers::SizeCounter counter;
    counter << shares;

    fetch::serializers::MsgPackSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    muddle.AsEndpoint().Send(destination, SERVICE_DKG, CHANNEL_SHARES, serializer.data());
  }
  virtual void SetOutput()                                                                      = 0;
  virtual void OnDkgMessage(ConstByteArray const &from, std::shared_ptr<DKGMessage> const &env) = 0;
  virtual void OnNewShares(ConstByteArray const &                     from,
                           std::pair<std::string, std::string> const &shares)                   = 0;
  virtual void DkgResetCabinet(std::set<ConstByteArray> const &cabinet, uint32_t threshold)     = 0;
  virtual std::weak_ptr<core::Runnable> GetWeakRunnable()                                       = 0;
  virtual bool                          DkgFinished()                                           = 0;

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
  FaultyDkgSetupService dkg;

  FaultyDkgMember(uint16_t port_number, uint16_t index,
                  const std::vector<FaultyDkgSetupService::Failures> &failures = {})
    : DkgMember{port_number, index}
    , dkg{muddle_certificate->identity().identifier(),
          [this](DKGEnvelope const &envelope) -> void {
            DKGSerializer serialiser;
            serialiser << envelope;
            rbc.Broadcast(serialiser.data());
          },
          [this](ConstByteArray const &                     destination,
                 std::pair<std::string, std::string> const &shares) -> void {
            SubmitShare(destination, shares);
          },
          failures}
  {}

  ~FaultyDkgMember() override = default;

  void SetOutput() override
  {
    dkg.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
  }
  void OnDkgMessage(ConstByteArray const &from, std::shared_ptr<DKGMessage> const &env) override
  {
    dkg.OnDkgMessage(from, env);
  }
  void OnNewShares(ConstByteArray const &                     from,
                   std::pair<std::string, std::string> const &shares) override
  {
    dkg.OnNewShares(from, shares);
  }
  void DkgResetCabinet(std::set<ConstByteArray> const &cabinet, uint32_t threshold) override
  {
    dkg.ResetCabinet(cabinet, threshold);
  }
  std::weak_ptr<core::Runnable> GetWeakRunnable() override
  {
    return dkg.GetWeakRunnable();
  }
  bool DkgFinished() override
  {
    return dkg.finished();
  }
};

struct HonestDkgMember : DkgMember
{
  DkgSetupService dkg;

  HonestDkgMember(uint16_t port_number, uint16_t index)
    : DkgMember{port_number, index}
    , dkg{muddle_certificate->identity().identifier(),
          [this](DKGEnvelope const &envelope) -> void {
            DKGSerializer serialiser;
            serialiser << envelope;
            rbc.Broadcast(serialiser.data());
          },
          [this](ConstByteArray const &                     destination,
                 std::pair<std::string, std::string> const &shares) -> void {
            SubmitShare(destination, shares);
          }}
  {}

  ~HonestDkgMember() override = default;

  void SetOutput() override
  {
    dkg.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
  }
  void OnDkgMessage(ConstByteArray const &from, std::shared_ptr<DKGMessage> const &env) override
  {
    dkg.OnDkgMessage(from, env);
  }
  void OnNewShares(ConstByteArray const &                     from,
                   std::pair<std::string, std::string> const &shares) override
  {
    dkg.OnNewShares(from, shares);
  }
  void DkgResetCabinet(std::set<ConstByteArray> const &cabinet, uint32_t threshold) override
  {
    dkg.ResetCabinet(cabinet, threshold);
  }
  std::weak_ptr<core::Runnable> GetWeakRunnable() override
  {
    return dkg.GetWeakRunnable();
  }
  bool DkgFinished() override
  {
    return dkg.finished();
  }
};

void GenerateTest(uint32_t cabinet_size, uint32_t threshold, uint32_t qual_size,
                  uint32_t expected_completion_size,
                  const std::vector<std::vector<FaultyDkgSetupService::Failures>> &failures = {})
{
  RBC::CabinetMembers                                                 cabinet;
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
      expected_qual.insert(committee[ii]->muddle.identity().identifier());
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
    committee[ii]->pre_sync.ResetCabinet(peers_list);
    committee[ii]->rbc.ResetCabinet(cabinet);
    committee[ii]->DkgResetCabinet(cabinet, threshold);
  }

  // Wait until everyone else has connected
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    committee[ii]->pre_sync.Connect();
  }

  uint32_t kk = 0;
  while (kk < cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (!committee[mm]->pre_sync.ready())
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

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Set DKG outputs
    for (auto &member : committee)
    {
      member->SetOutput();
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

// TODO(jmw): Send multiple shares

TEST(dkg_setup, small_scale_test)
{
  GenerateTest(4, 3, 4, 4);
}

TEST(dkg_setup, send_bad_share)
{
  // Node 0 sends bad secret shares to Node 1 which complains against it.
  // Node 0 then broadcasts its real shares as defense and then is allowed into
  // qual
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_BAD_SHARE}});
}

TEST(dkg_setup, bad_coefficients)
{
  // Node 0 broadcasts bad coefficients which fails verification by everyone.
  // Rejected from qual
  GenerateTest(4, 3, 3, 3, {{FaultyDkgSetupService::Failures::BAD_COEFFICIENT}});
}

TEST(dkg_setup, send_empty_complaints_answer)
{
  // Node 0 sends computes bad secret shares to Node 1 which complains against it.
  // Node 0 then does not send real shares and instead sends empty complaint answer.
  // Node 0 should be disqualified from qual
  GenerateTest(4, 3, 3, 3,
               {{FaultyDkgSetupService::Failures::SEND_BAD_SHARE,
                 FaultyDkgSetupService::Failures::SEND_EMPTY_COMPLAINT_ANSWER}});
}

TEST(dkg_setup, send_multiple_complaints)
{
  // Node 0 sends multiple complaint messages in the first round of complaints
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_MULTIPLE_COMPLAINTS}});
}

TEST(dkg_setup, send_multiple_coefficients)
{
  // Node 0 sends multiple coefficients. Should trigger warning but everyone
  // should succeed in DKG
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_MULTIPLE_COEFFICIENTS}});
}

TEST(dkg_setup, send_multiple_complaint_answers)
{
  // Node 0 sends multiple complaint answers. Should trigger warning but everyone
  // should succeed in DKG
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_MULTIPLE_COMPLAINT_ANSWERS}});
}

TEST(dkg_setup, qual_below_threshold)
{
  GenerateTest(4, 3, 2, 0,
               {{FaultyDkgSetupService::Failures::BAD_COEFFICIENT},
                {FaultyDkgSetupService::Failures::BAD_COEFFICIENT}});
}

TEST(dkg_setup, bad_qual_coefficients)
{
  // Node 0 computes bad qual coefficients so node 0 is in qual complaints but everyone reconstructs
  // their shares. Everyone else except node 0 succeeds in DKG
  GenerateTest(4, 3, 4, 3, {{FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, send_multiple_qual_coefficients)
{
  // Node 0 sends multiple qual coefficients so node 0.
  // Should trigger warning but everyone should succeed in DKG
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, send_fake_qual_complaint)
{
  // Node 0 sends fake qual coefficients. Should trigger warning and node 0's shares will be
  // reconstructed but everyone else should succeed in the DKG. Important test as it means
  // reconstruction computes the correct thing.
  GenerateTest(4, 3, 4, 4, {{FaultyDkgSetupService::Failures::SEND_FALSE_QUAL_COMPLAINT}});
}

TEST(dkg_setup, too_many_bad_qual_coefficients)
{
  // Three nodes send bad qual coefficients which means that there are
  // not enough parties not in complaints. DKG fails
  GenerateTest(4, 2, 4, 0,
               {{FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg_setup, send_multiple_reconstruction_shares)
{
  // Node sends multiple reconstruction shares which triggers warning but
  // DKG succeeds
  GenerateTest(4, 3, 4, 3,
               {{FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkgSetupService::Failures::SEND_MULTIPLE_RECONSTRUCTION_SHARES}});
}

TEST(dkg_setup, withold_reconstruction_shares)
{
  // Node 0 sends bad qual coefficients and another in collusion does not broadcast node 0's shares
  // so there are not enough shares to run reconstruction
  GenerateTest(4, 3, 4, 0,
               {{FaultyDkgSetupService::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkgSetupService::Failures::WITHOLD_RECONSTRUCTION_SHARES}});
}
