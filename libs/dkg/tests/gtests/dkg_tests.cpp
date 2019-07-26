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

#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/dkg.hpp"
#include "dkg/rbc.hpp"
#include "network/muddle/muddle.hpp"
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

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

uint16_t CHANNEL_SHARES = 3;

class FaultyDkg : public DistributedKeyGeneration
{
public:
  enum class Failures : uint8_t
  {
    BAD_COEFFICIENT,
    SEND_MULTIPLE_COEFFICIENTS,
    SEND_BAD_SHARE,
    COMPUTE_BAD_SHARE,
    SEND_MULTIPLE_COMPLAINTS,
    SEND_EMPTY_COMPLAINT_ANSWER,
    SEND_MULTIPLE_COMPLAINT_ANSWERS,
    BAD_QUAL_COEFFICIENTS,
    SEND_MULTIPLE_QUAL_COEFFICIENTS,
    SEND_FALSE_QUAL_COMPLAINT,
    SEND_MULTIPLE_RECONSTRUCTION_SHARES,
    WITHOLD_RECONSTRUCTION_SHARES
  };
  FaultyDkg(MuddleAddress address, CabinetMembers const &cabinet, uint32_t const &threshold,
            std::function<void(DKGEnvelope const &)> broadcast_callback,
            std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
                                         rpc_callback,
            const std::vector<Failures> &failures = {})
    : DistributedKeyGeneration{std::move(address), cabinet, threshold,
                               std::move(broadcast_callback), std::move(rpc_callback)}
  {
    for (auto f : failures)
    {
      failures_flags_.set(static_cast<uint32_t>(f));
    }
  }

  void BroadcastShares()
  {
    std::vector<bn::Fr> a_i(threshold_ + 1, zeroFr_), b_i(threshold_ + 1, zeroFr_);
    for (size_t k = 0; k <= threshold_; k++)
    {
      a_i[k].setRand();
      b_i[k].setRand();
    }
    if (Failure(Failures::BAD_COEFFICIENT))
    {
      SendBadCoefficients(a_i, b_i);
    }
    else
    {
      SendCoefficients(a_i, b_i);
      if (Failure(Failures::SEND_MULTIPLE_COEFFICIENTS))
      {
        SendBroadcast(DKGEnvelope{
            CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE), {}, "signature"}});
      }
    }
    if (Failure(Failures::SEND_BAD_SHARE))
    {
      SendBadShares(a_i, b_i);
    }
    else if (Failure(Failures::COMPUTE_BAD_SHARE))
    {
      ComputeBadShares(a_i, b_i);
    }
    else
    {
      SendShares(a_i, b_i);
    }
    state_.store(State::WAITING_FOR_SHARE);
    ReceivedCoefficientsAndShares();
  }

private:
  std::bitset<static_cast<uint8_t>(Failures::WITHOLD_RECONSTRUCTION_SHARES) + 1> failures_flags_;

  bool Failure(Failures f) const
  {
    return failures_flags_[static_cast<uint8_t>(f)];
  }

  void SendBadCoefficients(std::vector<bn::Fr> const &a_i, std::vector<bn::Fr> const &b_i)
  {
    // Let z_i = f(0)
    z_i[cabinet_index_] = a_i[0];

    std::vector<std::string> coefficients;
    for (size_t k = 0; k <= threshold_; k++)
    {
      coefficients.push_back(C_ik[cabinet_index_][k].getStr());
      C_ik[cabinet_index_][k] = ComputeLHS(g__a_i[k], group_g_, group_h_, a_i[k], b_i[k]);
    }
    // Send empty coefficients to everyone
    SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                                  coefficients, "signature"}});
  }

  void SendBadShares(std::vector<bn::Fr> const &a_i, std::vector<bn::Fr> const &b_i)
  {
    uint32_t j = 0;
    bool     sent_bad{false};
    for (auto &cab_i : cabinet_)
    {
      ComputeShares(s_ij[cabinet_index_][j], sprime_ij[cabinet_index_][j], a_i, b_i, j);
      if (j != cabinet_index_)
      {
        // Send a one node trivial shares
        if (!sent_bad)
        {
          bn::Fr trivial_share;
          trivial_share.clear();
          std::pair<MsgShare, MsgShare> shares{trivial_share.getStr(), trivial_share.getStr()};
          rpc_function_(cab_i, shares);
          sent_bad = true;
        }
        else
        {
          std::pair<MsgShare, MsgShare> shares{s_ij[cabinet_index_][j].getStr(),
                                               sprime_ij[cabinet_index_][j].getStr()};
          rpc_function_(cab_i, shares);
        }
      }
      ++j;
    }
  }

  void ComputeBadShares(std::vector<bn::Fr> const &a_i, std::vector<bn::Fr> const &b_i)
  {
    uint32_t j = 0;
    bool     bad_share{false};
    for (auto &cab_i : cabinet_)
    {
      if (j != cabinet_index_)
      {
        // Compute one pair of trivial shares
        if (!bad_share)
        {
          std::pair<MsgShare, MsgShare> shares{s_ij[cabinet_index_][j].getStr(),
                                               sprime_ij[cabinet_index_][j].getStr()};
          rpc_function_(cab_i, shares);
          bad_share = true;
        }
        else
        {
          ComputeShares(s_ij[cabinet_index_][j], sprime_ij[cabinet_index_][j], a_i, b_i, j);
          std::pair<MsgShare, MsgShare> shares{s_ij[cabinet_index_][j].getStr(),
                                               sprime_ij[cabinet_index_][j].getStr()};
          rpc_function_(cab_i, shares);
        }
      }
      ++j;
    }
  }

  void BroadcastComplaints() override
  {
    std::unordered_set<MuddleAddress> complaints_local = ComputeComplaints();
    for (auto const &cab : complaints_local)
    {
      complaints_manager_.Count(cab);
    }
    SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
    if (Failure(Failures::SEND_MULTIPLE_COMPLAINTS))
    {
      SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
    }
    state_ = State::WAITING_FOR_COMPLAINTS;
    ReceivedComplaint();
  }

  void BroadcastComplaintsAnswer() override
  {
    std::unordered_map<MuddleAddress, std::pair<MsgShare, MsgShare>> complaints_answer;
    if (!Failure(Failures::SEND_EMPTY_COMPLAINT_ANSWER))
    {
      for (auto const &reporter : complaints_manager_.ComplaintsFrom())
      {
        uint32_t from_index{CabinetIndex(reporter)};
        complaints_answer.insert({reporter,
                                  {s_ij[cabinet_index_][from_index].getStr(),
                                   sprime_ij[cabinet_index_][from_index].getStr()}});
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
    state_ = State::WAITING_FOR_COMPLAINT_ANSWERS;
    ReceivedComplaintsAnswer();
  }

  void BroadcastQualCoefficients() override
  {
    std::vector<std::string> coefficients;
    if (Failure(Failures::BAD_QUAL_COEFFICIENTS))
    {
      for (size_t k = 0; k <= threshold_; k++)
      {
        A_ik[cabinet_index_][k] = zeroG2_;
        coefficients.push_back(A_ik[cabinet_index_][k].getStr());
      }
      SendBroadcast(DKGEnvelope{CoefficientsMessage{
          static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES), coefficients, "signature"}});
    }
    else
    {
      for (size_t k = 0; k <= threshold_; k++)
      {
        A_ik[cabinet_index_][k] = g__a_i[k];
        coefficients.push_back(A_ik[cabinet_index_][k].getStr());
      }
      SendBroadcast(DKGEnvelope{CoefficientsMessage{
          static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES), coefficients, "signature"}});
      if (Failure(Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS))
      {
        SendBroadcast(DKGEnvelope{CoefficientsMessage{
            static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES), coefficients, "signature"}});
      }
    }

    complaints_answer_manager_.Clear();
    state_ = State::WAITING_FOR_QUAL_SHARES;
    ReceivedQualShares();
  }

  void BroadcastQualComplaints() override
  {
    if (Failure(Failures::SEND_FALSE_QUAL_COMPLAINT))
    {
      auto     victim = cabinet_.begin();
      uint32_t i      = 0;
      if (*victim == address_)
      {
        ++victim;
        ++i;
      }
      SendBroadcast(DKGEnvelope{SharesMessage{
          static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS),
          {{*victim, {s_ij[i][cabinet_index_].getStr(), sprime_ij[i][cabinet_index_].getStr()}}},
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
                                    ComputeQualComplaints(), "signature"}});
    }
    state_ = State::WAITING_FOR_QUAL_COMPLAINTS;
    ReceivedQualComplaint();
  }

  void BroadcastReconstructionShares() override
  {
    std::unordered_map<MuddleAddress, std::pair<MsgShare, MsgShare>> complaint_shares;
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
        uint32_t in_index{CabinetIndex(in)};
        reconstruction_shares.insert({in, {{}, std::vector<bn::Fr>(cabinet_.size(), zeroFr_)}});
        reconstruction_shares.at(in).first.insert(cabinet_index_);
        reconstruction_shares.at(in).second[cabinet_index_] = s_ij[in_index][cabinet_index_];
        complaint_shares.insert({in,
                                 {s_ij[in_index][cabinet_index_].getStr(),
                                  sprime_ij[in_index][cabinet_index_].getStr()}});
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
    state_ = State::WAITING_FOR_RECONSTRUCTION_SHARES;
    ReceivedReconstructionShares();
  }
};

struct CabinetMember
{
  uint16_t                              muddle_port;
  NetworkManager                        network_manager;
  ProverPtr                             muddle_certificate;
  Muddle                                muddle;
  std::shared_ptr<muddle::Subscription> shares_subscription;
  RBC                                   rbc;
  FaultyDkg                             dkg;

  // Set when DKG is finished
  bn::Fr              secret_share;
  bn::G2              public_key;
  RBC::CabinetMembers qual_set;
  std::vector<bn::G2> public_key_shares;

  CabinetMember(uint16_t port_number, uint16_t index, RBC::CabinetMembers const &current_cabinet,
                uint32_t const &threshold, const std::vector<FaultyDkg::Failures> &failures = {})
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 3}
    , muddle_certificate{CreateNewCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager, true,
             true}
    , shares_subscription(muddle.AsEndpoint().Subscribe(SERVICE_DKG, CHANNEL_SHARES))
    , rbc{muddle.AsEndpoint(), muddle_certificate->identity().identifier(), current_cabinet,
          [this](ConstByteArray const &address, ConstByteArray const &payload) -> void {
            DKGEnvelope   env;
            DKGSerializer serializer{payload};
            serializer >> env;
            dkg.OnDkgMessage(address, env.Message());
          }}
    , dkg{muddle_certificate->identity().identifier(),
          current_cabinet,
          threshold,
          [this](DKGEnvelope const &envelope) -> void {
            DKGSerializer serialiser;
            envelope.Serialize(serialiser);
            rbc.SendRBroadcast(serialiser.data());
          },
          [this](ConstByteArray const &                     destination,
                 std::pair<std::string, std::string> const &shares) -> void {
            SubmitShare(destination, shares);
          },
          failures}
  {
    // Set subscription for receiving shares
    shares_subscription->SetMessageHandler([this](ConstByteArray const &from, uint16_t, uint16_t,
                                                  uint16_t, muddle::Packet::Payload const &payload,
                                                  ConstByteArray) {
      fetch::serializers::ByteArrayBuffer serialiser(payload);

      std::pair<std::string, std::string> shares;
      serialiser >> shares;

      // Dispatch the event
      dkg.OnNewShares(from, shares);
    });

    network_manager.Start();
    muddle.Start({muddle_port});
  }

  void SubmitShare(ConstByteArray const &                     destination,
                   std::pair<std::string, std::string> const &shares)
  {
    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << shares;

    fetch::serializers::ByteArrayBuffer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    muddle.AsEndpoint().Send(destination, SERVICE_DKG, CHANNEL_SHARES, serializer.data());
  }
  void SetOutput()
  {
    dkg.SetDkgOutput(public_key, secret_share, public_key_shares, qual_set);
  }
};

void GenerateTest(uint32_t cabinet_size, uint32_t threshold, uint32_t qual_size,
                  uint32_t                                             expected_completion_size,
                  const std::vector<std::vector<FaultyDkg::Failures>> &failures       = {},
                  uint32_t                                             num_dkg_rounds = 1)
{
  RBC::CabinetMembers cabinet;

  std::vector<std::unique_ptr<CabinetMember>> committee;
  std::set<RBC::MuddleAddress>                expected_qual;
  std::set<uint32_t>                          qual_index;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    if (!failures.empty() && ii < failures.size())
    {
      committee.emplace_back(new CabinetMember{port_number, ii, cabinet, threshold, failures[ii]});
    }
    else
    {
      committee.emplace_back(new CabinetMember{port_number, ii, cabinet, threshold});
    }
    if (ii >= (cabinet_size - qual_size))
    {
      expected_qual.insert(committee[ii]->muddle.identity().identifier());
      qual_index.insert(ii);
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      committee[ii]->muddle.AddPeer(
          fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(committee[jj]->muddle_port)});
    }
  }

  // Make sure everyone is connected to everyone else
  uint32_t kk = 0;
  while (kk != cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (committee[mm]->muddle.AsEndpoint().GetDirectlyConnectedPeers().size() !=
          (cabinet_size - 1))
      {
        break;
      }
      else
      {
        ++kk;
      }
    }
  }

  for (auto &member : committee)
  {
    cabinet.insert(member->muddle_certificate->identity().identifier());
  }

  assert(cabinet.size() == cabinet_size);

  // Reset cabinet
  for (uint32_t round_i = 0; round_i < num_dkg_rounds; ++round_i)
  {
    for (auto &member : committee)
    {
      member->dkg.ResetCabinet();
      member->rbc.ResetCabinet();
    }

    // Start at DKG
    {
      for (auto &member : committee)
      {
        member->dkg.BroadcastShares();
      }

      // Loop until everyone is finished with DKG
      uint32_t pp = 0;
      while (pp < cabinet_size)
      {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        for (const auto &member : committee)
        {
          if (!member->dkg.finished())
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

  for (auto &member : committee)
  {
    member->muddle.Stop();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(dkg, small_scale_test)
{
  GenerateTest(4, 1, 4, 4);
}

TEST(dkg, send_bad_share)
{
  // Node 0 sends bad secret shares to Node 1 which complains against it.
  // Node 0 then broadcasts its real shares as defense and then is allowed into
  // qual
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_BAD_SHARE}});
}

TEST(dkg, bad_coefficients)
{
  // Node 0 broadcasts bad coefficients which fails verification by everyone.
  // Rejected from qual
  GenerateTest(4, 1, 3, 3, {{FaultyDkg::Failures::BAD_COEFFICIENT}});
}

TEST(dkg, compute_bad_shares)
{
  // Node 0 sends computes bad secret shares to Node 1 which complains against it.
  // Node 0 then broadcasts the shares sent to Node 1 as defense but as they have been
  // computed wrong they are not in qual
  GenerateTest(4, 1, 3, 3, {{FaultyDkg::Failures::COMPUTE_BAD_SHARE}});
}

TEST(dkg, send_empty_complaints_answer)
{
  // Node 0 sends computes bad secret shares to Node 1 which complains against it.
  // Node 0 then does not send real shares and instead sends empty complaint answer.
  // Node 0 should be disqualified from qual
  GenerateTest(
      4, 1, 3, 3,
      {{FaultyDkg::Failures::SEND_BAD_SHARE, FaultyDkg::Failures::SEND_EMPTY_COMPLAINT_ANSWER}});
}

TEST(dkg, send_multiple_complaints)
{
  // Node 0 sends multiple complaint messages in the first round of complaints
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_MULTIPLE_COMPLAINTS}});
}

TEST(dkg, send_multiple_coefficients)
{
  // Node 0 sends multiple coefficients. Should trigger warning but everyone
  // should succeed in DKG
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_MULTIPLE_COEFFICIENTS}});
}

TEST(dkg, send_multiple_complaint_answers)
{
  // Node 0 sends multiple complaint answers. Should trigger warning but everyone
  // should succeed in DKG
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_MULTIPLE_COMPLAINT_ANSWERS}});
}

TEST(dkg, qual_below_threshold)
{
  // Qual size is below threshold as two many nodes were disqualified so DKG fails for
  // everyone
  GenerateTest(4, 2, 2, 0,
               {{FaultyDkg::Failures::BAD_COEFFICIENT}, {FaultyDkg::Failures::BAD_COEFFICIENT}});
}

TEST(dkg, bad_qual_coefficients)
{
  // Node 0 computes bad qual coefficients so node 0 is in qual complaints but everyone reconstructs
  // their shares. Everyone else except node 0 succeeds in DKG
  GenerateTest(4, 1, 4, 3, {{FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg, send_multiple_qual_coefficients)
{
  // Node 0 sends multiple qual coefficients so node 0.
  // Should trigger warning but everyone should succeed in DKG
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_MULTIPLE_QUAL_COEFFICIENTS}});
}

TEST(dkg, send_fake_qual_complaint)
{
  // Node 0 sends multiple qual coefficients. Should trigger warning and node 0's shares will be
  // reconstructed but everyone else should succeed in the DKG. Important test as it means
  // reconstruction computes the correct thing.
  GenerateTest(4, 1, 4, 4, {{FaultyDkg::Failures::SEND_FALSE_QUAL_COMPLAINT}});
}

TEST(dkg, too_many_bad_qual_coefficients)
{
  // Two nodes send bad qual coefficients which means that there are
  // not enough parties not (threshold + 1) parities not in complaints.
  // DKG fails
  GenerateTest(4, 2, 4, 0,
               {{FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS}});
}

TEST(dkg, send_multiple_reconstruction_shares)
{
  // Node sends multiple reconstruction shares which triggers warning but
  // DKG succeeds
  GenerateTest(4, 1, 4, 3,
               {{FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkg::Failures::SEND_MULTIPLE_RECONSTRUCTION_SHARES}});
}

TEST(dkg, withold_reconstruction_shares)
{
  // Node 0 sends bad qual coefficients and another in collusion does not broadcast node 0's shares
  // so there are not enough shares to run reconstruction
  GenerateTest(4, 2, 4, 0,
               {{FaultyDkg::Failures::BAD_QUAL_COEFFICIENTS},
                {FaultyDkg::Failures::WITHOLD_RECONSTRUCTION_SHARES}});
}

TEST(dkg, successive_dkgs)
{
  GenerateTest(4, 1, 4, 4, {}, 4);
}
