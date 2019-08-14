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

#include "dkg/dkg.hpp"

#include <mutex>

namespace fetch {
namespace dkg {

using MsgCoefficient = std::string;

constexpr char const *LOGGING_NAME = "DKG";
bn::G2                DistributedKeyGeneration::zeroG2_;
bn::Fr                DistributedKeyGeneration::zeroFr_;
bn::G2                DistributedKeyGeneration::group_g_;
bn::G2                DistributedKeyGeneration::group_h_;

DistributedKeyGeneration::DistributedKeyGeneration(
    MuddleAddress address, CabinetMembers const &cabinet, uint32_t const &threshold,
    std::function<void(DKGEnvelope const &)> broadcast_function,
    std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
        rpc_function)
  : cabinet_{cabinet}
  , threshold_{threshold}
  , address_{std::move(address)}
  , broadcast_function_{std::move(broadcast_function)}
  , rpc_function_{std::move(rpc_function)}
{
  static std::once_flag flag;

  std::call_once(flag, []() {
    bn::initPairing();
    zeroG2_.clear();
    zeroFr_.clear();
    group_g_.clear();
    group_h_.clear();

    // Values taken from TMCG main.cpp
    const bn::Fp2 g(
        "1380305877306098957770911920312855400078250832364663138573638818396353623780",
        "14633108267626422569982187812838828838622813723380760182609272619611213638781");
    const bn::Fp2 h(
        "6798148801244076840612542066317482178930767218436703568023723199603978874964",
        "12726557692714943631796519264243881146330337674186001442981874079441363994424");
    bn::mapToG2(group_g_, g);
    bn::mapToG2(group_h_, h);
  });
}

/**
 * Sends DKG message via reliable broadcast channel in dkg_service
 *
 * @param env DKGEnvelope containing message to the broadcasted
 */
void DistributedKeyGeneration::SendBroadcast(DKGEnvelope const &env)
{
  broadcast_function_(env);
}

/**
 * Computes and broadcasts a vector of coefficients computed from the randomly initialised
 * coefficients of two polynomials of order threshold_ + 1
 *
 * @param a_i Vector of coefficients for polynomial f
 * @param b_i Vector of coefficients for another polynomial f'
 */
void DistributedKeyGeneration::SendCoefficients(std::vector<bn::Fr> const &a_i,
                                                std::vector<bn::Fr> const &b_i)
{
  // Let z_i = f(0)
  z_i[cabinet_index_] = a_i[0];

  std::vector<MsgCoefficient> coefficients;
  for (size_t k = 0; k <= threshold_; k++)
  {
    C_ik[cabinet_index_][k] = ComputeLHS(g__a_i[k], group_g_, group_h_, a_i[k], b_i[k]);
    coefficients.push_back(C_ik[cabinet_index_][k].getStr());
  }
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                                coefficients, "signature"}});
}

/**
 * Computes and sends different sets of secret shares (s_ij, sprime_ij) to all cabinet members using
 * the randomly initialised coefficients of f and f', polynomials of order threshold_ + 1
 *
 * @param a_i Vector of coefficients for polynomial f
 * @param b_i Vector of coefficients for another polynomial f'
 */
void DistributedKeyGeneration::SendShares(std::vector<bn::Fr> const &a_i,
                                          std::vector<bn::Fr> const &b_i)
{
  uint32_t j = 0;
  for (auto &cab_i : cabinet_)
  {
    ComputeShares(s_ij[cabinet_index_][j], sprime_ij[cabinet_index_][j], a_i, b_i, j);
    if (j != cabinet_index_)
    {
      std::pair<MsgShare, MsgShare> shares{s_ij[cabinet_index_][j].getStr(),
                                           sprime_ij[cabinet_index_][j].getStr()};
      rpc_function_(cab_i, shares);
    }
    ++j;
  }
}

/**
 * Randomly initialises coefficients of two polynomials, computes the coefficients and secret
 * shares and sends to cabinet members
 */
void DistributedKeyGeneration::BroadcastShares()
{
  std::vector<bn::Fr> a_i(threshold_ + 1, zeroFr_), b_i(threshold_ + 1, zeroFr_);
  for (size_t k = 0; k <= threshold_; k++)
  {
    a_i[k].setRand();
    b_i[k].setRand();
  }
  SendCoefficients(a_i, b_i);
  SendShares(a_i, b_i);
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " broadcasts coefficients ");
  state_.store(State::WAITING_FOR_SHARE);
  ReceivedCoefficientsAndShares();
}

/**
 * Broadcast the set nodes we are complaining against based on the secret shares and coefficients
 * sent to use. Also increments the number of complaints a given cabinet member has received with
 * our complaints
 */
void DistributedKeyGeneration::BroadcastComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local = ComputeComplaints();
  for (auto const &cab : complaints_local)
  {
    complaints_manager_.Count(cab);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " broadcasts complaints size ",
                 complaints_local.size());
  SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
  state_ = State::WAITING_FOR_COMPLAINTS;
  ReceivedComplaint();
}

/**
 * For a complaint by cabinet member c_i against self we broadcast the secret share
 * we sent to c_i to all cabinet members. This serves as a round of defense against
 * complaints where a member reveals the secret share they sent to c_i to everyone to
 * prove that it is consistent with the coefficients they originally broadcasted
 */
void DistributedKeyGeneration::BroadcastComplaintsAnswer()
{
  std::unordered_map<MuddleAddress, std::pair<MsgShare, MsgShare>> complaints_answer;
  for (auto const &reporter : complaints_manager_.ComplaintsFrom())
  {
    uint32_t from_index{CabinetIndex(reporter)};
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " received complaints from ", from_index);
    complaints_answer.insert({reporter,
                              {s_ij[cabinet_index_][from_index].getStr(),
                               sprime_ij[cabinet_index_][from_index].getStr()}});
  }
  SendBroadcast(
      DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS),
                                complaints_answer, "signature"}});
  state_ = State::WAITING_FOR_COMPLAINT_ANSWERS;
  ReceivedComplaintsAnswer();
}

/**
 * Broadcast coefficients after computing own secret share
 */
void DistributedKeyGeneration::BroadcastQualCoefficients()
{
  std::vector<MsgCoefficient> coefficients;
  for (size_t k = 0; k <= threshold_; k++)
  {
    A_ik[cabinet_index_][k] = g__a_i[k];
    coefficients.push_back(A_ik[cabinet_index_][k].getStr());
  }
  SendBroadcast(DKGEnvelope{CoefficientsMessage{
      static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES), coefficients, "signature"}});
  complaints_answer_manager_.Clear();
  state_ = State::WAITING_FOR_QUAL_SHARES;
  {
    std::unique_lock<std::mutex> lock{mutex_};
    A_ik_received_.insert(address_);
  }
  ReceivedQualShares();
}

/**
 * After constructing the qualified set (qual) and receiving new qual coefficients members
 * broadcast the secret shares s_ij, sprime_ij of all members in qual who sent qual coefficients
 * which failed verification
 */
void DistributedKeyGeneration::BroadcastQualComplaints()
{
  SendBroadcast(DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS),
                                          ComputeQualComplaints(), "signature"}});
  state_ = State::WAITING_FOR_QUAL_COMPLAINTS;
  ReceivedQualComplaint();
}

/**
 * For all members that other nodes have complained against in qual we also broadcast
 * the secret shares we received from them to all cabinet members and collect the shares broadcasted
 * by others
 */

void DistributedKeyGeneration::BroadcastReconstructionShares()
{
  std::unordered_map<MuddleAddress, std::pair<MsgShare, MsgShare>> complaint_shares;
  for (auto const &in : qual_complaints_manager_.Complaints())
  {
    assert(qual_.find(in) != qual_.end());
    uint32_t in_index{CabinetIndex(in)};
    reconstruction_shares.insert({in, {{}, std::vector<bn::Fr>(cabinet_.size(), zeroFr_)}});
    reconstruction_shares.at(in).first.insert(cabinet_index_);
    reconstruction_shares.at(in).second[cabinet_index_] = s_ij[in_index][cabinet_index_];
    complaint_shares.insert(
        {in,
         {s_ij[in_index][cabinet_index_].getStr(), sprime_ij[in_index][cabinet_index_].getStr()}});
  }
  SendBroadcast(
      DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES),
                                complaint_shares, "signature"}});
  state_ = State::WAITING_FOR_RECONSTRUCTION_SHARES;
  ReceivedReconstructionShares();
}

/**
 * Checks if all coefficients and shares sent in the initial state have been received in order to
 * transition to the next state
 */
void DistributedKeyGeneration::ReceivedCoefficientsAndShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  FETCH_LOG_TRACE(LOGGING_NAME, "receivedCoefficientsAndShares node ", id_, " state ",
                  state_.load() == State::WAITING_FOR_SHARE, " C_ik ", C_ik_received_.load(),
                  " shares ", shares_received_.load());
  if (!received_all_coef_and_shares_ && (state_.load() == State::WAITING_FOR_SHARE) &&
      (C_ik_received_.load() == cabinet_.size() - 1) &&
      (shares_received_.load()) == cabinet_.size() - 1)
  {
    received_all_coef_and_shares_.store(true);
    lock.unlock();
    BroadcastComplaints();
  }
}

/**
 * Checks if all complaints have been received to trigger transition into complaints answer state
 */
void DistributedKeyGeneration::ReceivedComplaint()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (!received_all_complaints_ && state_ == State::WAITING_FOR_COMPLAINTS &&
      complaints_manager_.IsFinished(cabinet_, cabinet_index_, static_cast<uint32_t>(threshold_)))
  {
    // Complaints at this point consist only of parties which have received over threshold number of
    // complaints
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " complaints size ",
                   complaints_manager_.Complaints().size());
    complaints_answer_manager_.Init(complaints_manager_.Complaints());
    received_all_complaints_.store(true);
    lock.unlock();
    BroadcastComplaintsAnswer();
  }
}

/**
 * Check whether all complaint answers have been received so that members can build the qualified
 * set (qual) which will take part in the public key generation. If self is in qual and qual is
 * at least of size threshold_ + 1 then we compute our secret share of the private key
 */
void DistributedKeyGeneration::ReceivedComplaintsAnswer()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (!received_all_complaints_answer_ && state_ == State::WAITING_FOR_COMPLAINT_ANSWERS &&
      complaints_answer_manager_.IsFinished(cabinet_, cabinet_index_))
  {
    received_all_complaints_answer_.store(true);
    lock.unlock();
    if (BuildQual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, "build qual of size ", qual_.size());
      ComputeSecretShare();
      BroadcastQualCoefficients();
    }
    else
    {
      finished_ = true;
      // TODO(jmw): procedure failed for this node
    }
    complaints_manager_.Clear();
  }
}

/**
 * Checks whether all new shares sent by qual have been received and triggers transition into
 * qual complaints if true
 */
void DistributedKeyGeneration::ReceivedQualShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (!received_all_qual_shares_ && (state_ == State::WAITING_FOR_QUAL_SHARES))
  {
    std::set<MuddleAddress> diff;
    std::set_difference(qual_.begin(), qual_.end(), A_ik_received_.begin(), A_ik_received_.end(),
                        std::inserter(diff, diff.begin()));
    if (diff.empty())
    {
      received_all_qual_shares_.store(true);
      lock.unlock();
      BroadcastQualComplaints();
    }
  }
}

/**
 * Checks wehther all qual complaints have been received. Self can fail at DKG if we are in
 * complaints or if there are too many complaints. If checks pass then transition into exposing the
 * secret shares of members in qual complaints
 */
void DistributedKeyGeneration::ReceivedQualComplaint()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (!received_all_qual_complaints_ && (state_ == State::WAITING_FOR_QUAL_COMPLAINTS) &&
      (qual_complaints_manager_.IsFinished(qual_, address_)))
  {
    CheckQualComplaints();
    received_all_qual_complaints_.store(true);
    size_t size = qual_complaints_manager_.ComplaintsSize();

    if (size > threshold_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " DKG has failed: complaints size ",
                     size);
      finished_ = true;
      lock.unlock();
      return;
    }
    else if (qual_complaints_manager_.ComplaintsFind(address_))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " is in qual complaints");
      lock.unlock();
      ComputePublicKeys();
      return;
    }
    assert(qual_.find(address_) != qual_.end());
    lock.unlock();
    BroadcastReconstructionShares();
  }
}

/**
 * Checks whether all messages containing exposed secret shares have been received (each qual member
 * sends one message with all shares they are exposing) and run polynomial interpolation on exposed
 * shares to reconstruct the polynomial of members in qual complaints. If this passes then all still
 * valid qual members (those not in qual complaints) compute the group public key and individual
 * public key shares
 */
void DistributedKeyGeneration::ReceivedReconstructionShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (!received_all_reconstruction_shares_ && state_ == State::WAITING_FOR_RECONSTRUCTION_SHARES &&
      reconstruction_shares_received_.load() ==
          qual_.size() - qual_complaints_manager_.ComplaintsSize() - 1)
  {
    received_all_reconstruction_shares_.store(true);
    lock.unlock();
    if (!RunReconstruction())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_,
                     " DKG failed due to reconstruction failure");
      finished_ = true;
    }
    else
    {
      ComputePublicKeys();
      qual_complaints_manager_.Clear();
    }
  }
}

/**
 * Handler for DKGMessage that has passed through the reliable broadcast
 *
 * @param from Muddle address of sender
 * @param msg_ptr Pointer of DKGMessage
 */
void DistributedKeyGeneration::OnDkgMessage(MuddleAddress const &       from,
                                            std::shared_ptr<DKGMessage> msg_ptr)
{
  if (!BasicMsgCheck(from, msg_ptr))
  {
    return;
  }
  uint32_t senderIndex{CabinetIndex(from)};
  switch (msg_ptr->type())
  {
  case DKGMessage::MessageType::COEFFICIENT:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", cabinet_index_, " received RBroadcast from node ",
                    senderIndex);
    auto coefficients_ptr = std::dynamic_pointer_cast<CoefficientsMessage>(msg_ptr);
    if (coefficients_ptr != nullptr)
    {
      OnNewCoefficients(*coefficients_ptr, from);
    }
    break;
  }
  case DKGMessage::MessageType::SHARE:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", cabinet_index_, " received REcho from node ",
                    senderIndex);
    auto share_ptr = std::dynamic_pointer_cast<SharesMessage>(msg_ptr);
    if (share_ptr != nullptr)
    {
      OnExposedShares(*share_ptr, from);
    }
    break;
  }
  case DKGMessage::MessageType::COMPLAINT:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", cabinet_index_, " received RReady from node ",
                    senderIndex);
    auto complaint_ptr = std::dynamic_pointer_cast<ComplaintsMessage>(msg_ptr);
    if (complaint_ptr != nullptr)
    {
      OnComplaints(*complaint_ptr, from);
    }
    break;
  }
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Node: ", cabinet_index_, " can not process payload from node ",
                    senderIndex);
  }
}

/**
 * Handler for all broadcasted messages containing secret shares
 *
 * @param shares Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnExposedShares(SharesMessage const &shares,
                                               MuddleAddress const &from_id)
{
  uint64_t phase1{shares.phase()};
  if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " received complaint answer from ",
                   CabinetIndex(from_id));
    OnComplaintsAnswer(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " received QUAL complaint from ",
                   CabinetIndex(from_id));
    OnQualComplaints(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " received reconstruction share from ",
                   CabinetIndex(from_id));
    OnReconstructionShares(shares, from_id);
  }
}

/**
 * Handler for RPC submit shares used for members to send individual pairs of
 * secret shares to other cabinet members
 *
 * @param from Muddle address of sender
 * @param shares Pair of secret shares
 */
void DistributedKeyGeneration::OnNewShares(MuddleAddress                        from,
                                           std::pair<MsgShare, MsgShare> const &shares)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " received shares from node  ",
                 CabinetIndex(from));
  uint32_t from_index{CabinetIndex(from)};
  s_ij[from_index][cabinet_index_].setStr(shares.first);
  sprime_ij[from_index][cabinet_index_].setStr(shares.second);

  ++shares_received_;
  ReceivedCoefficientsAndShares();
}

/**
 * Handler for broadcasted coefficients
 *
 * @param msg_ptr Pointer of CoefficientsMessage
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnNewCoefficients(CoefficientsMessage const &msg,
                                                 MuddleAddress const &      from_id)
{
  uint32_t from_index{CabinetIndex(from_id)};
  if (msg.phase() == static_cast<uint64_t>(State::WAITING_FOR_SHARE))
  {
    for (uint32_t ii = 0; ii <= threshold_; ++ii)
    {
      if (C_ik[from_index][ii] == zeroG2_)
      {
        C_ik[from_index][ii].setStr((msg.coefficients())[ii]);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " received duplicate coefficients from node ", from_index);
        return;
      }
    }
    ++C_ik_received_;
    ReceivedCoefficientsAndShares();
  }
  else if (msg.phase() == static_cast<uint64_t>(State::WAITING_FOR_QUAL_SHARES))
  {
    for (uint32_t ii = 0; ii <= threshold_; ++ii)
    {
      if (A_ik[from_index][ii] == zeroG2_)
      {
        A_ik[from_index][ii].setStr((msg.coefficients())[ii]);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " received duplicate qual coefficients from node ", from_index);
        return;
      }
    }
    {
      std::unique_lock<std::mutex> lock{mutex_};
      A_ik_received_.insert(from_id);
    }
    ReceivedQualShares();
  }
}

/**
 * Handler for complaints message
 *
 * @param msg_ptr Pointer of ComplaintsMessage
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnComplaints(ComplaintsMessage const &msg,
                                            MuddleAddress const &    from_id)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " received complaints from node ",
                 CabinetIndex(from_id));
  complaints_manager_.Add(msg, from_id, CabinetIndex(from_id), address_);
  ReceivedComplaint();
}

/**
 * Handler for complaints answer message containing the pairs of secret shares the sender sent to
 * members that complained against the sender
 *
 * @param msg_ptr Pointer of SharesMessage containing the sender's shares
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnComplaintsAnswer(SharesMessage const &answer,
                                                  MuddleAddress const &from_id)
{
  uint32_t from_index{CabinetIndex(from_id)};
  if (complaints_answer_manager_.Count(from_index))
  {
    CheckComplaintAnswer(answer, from_id, from_index);
    ReceivedComplaintsAnswer();
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                   " received multiple complaint answer from node ", from_index);
  }
}

/**
 * Handler for qual complaints message which contains the secret shares sender received from
 * members in qual complaints
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnQualComplaints(SharesMessage const &shares_msg,
                                                MuddleAddress const &from_id)
{
  qual_complaints_manager_.Received(from_id, shares_msg.shares());
  ReceivedQualComplaint();
}

/**
 * Handler for messages containing secret shares of qual members that other qual members have
 * complained against
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DistributedKeyGeneration::OnReconstructionShares(SharesMessage const &shares_msg,
                                                      MuddleAddress const &from_id)
{
  uint32_t from_index{CabinetIndex(from_id)};
  // Return if the sender is in complaints, or not in QUAL
  // TODO(JMW): Could be problematic if qual has not been built yet
  if (qual_complaints_manager_.ComplaintsFind(from_id) or qual_.find(from_id) == qual_.end())
  {
    return;
  }
  for (auto const &share : shares_msg.shares())
  {
    if (reconstruction_shares.find(share.first) == reconstruction_shares.end())
    {
      reconstruction_shares.insert(
          {share.first, {{}, std::vector<bn::Fr>(cabinet_.size(), zeroFr_)}});
    }
    else if (reconstruction_shares.at(share.first).second[from_index] != zeroFr_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                     " received duplicate reconstruction shares from node ", from_index);
      return;
    }
    uint32_t victim_index{CabinetIndex(share.first)};
    // assert(qual_complaints_manager_.ComplaintsFind(share.first)); // Fails for nodes who receive
    // shares for themselves when they don't know they are being complained against
    bn::G2 lhs, rhs;
    bn::Fr s, sprime;
    lhs.clear();
    rhs.clear();
    s.clear();
    sprime.clear();

    s.setStr(share.second.first);
    sprime.setStr(share.second.second);
    lhs = ComputeLHS(group_g_, group_h_, s, sprime);
    rhs = ComputeRHS(from_index, C_ik[victim_index]);
    // check equation (4)
    if (lhs == rhs)
    {
      std::lock_guard<std::mutex> lock{mutex_};
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, "received good share from node ",
                     from_index, "for reconstructing node ", victim_index);
      reconstruction_shares.at(share.first).first.insert(from_index);  // good share received
      reconstruction_shares.at(share.first).second[from_index] = s;
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, "received bad share from node ",
                     from_index, "for reconstructing node ", victim_index);
    }
  }
  ++reconstruction_shares_received_;
  ReceivedReconstructionShares();
}

/**
 * Checks coefficients broadcasted by cabinet member c_i is consistent with the secret shares
 * received from c_i. If false then add to complaints
 *
 * @return Set of muddle addresses of nodes we complain against
 */
std::unordered_set<DistributedKeyGeneration::MuddleAddress>
DistributedKeyGeneration::ComputeComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local;
  uint32_t                          i = 0;
  for (auto &cab : cabinet_)
  {
    if (i != cabinet_index_)
    {
      // Can only require this if group_g_, group_h_ do not take the default values from clear()
      if (C_ik[i][0] != zeroG2_ && s_ij[i][cabinet_index_] != zeroFr_)
      {
        bn::G2 rhs, lhs;
        lhs = ComputeLHS(g__s_ij[i][cabinet_index_], group_g_, group_h_, s_ij[i][cabinet_index_],
                         sprime_ij[i][cabinet_index_]);
        rhs = ComputeRHS(cabinet_index_, C_ik[i]);
        if (lhs != rhs)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                         " received bad coefficients/shares from ", CabinetIndex(cab));
          complaints_local.insert(cab);
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " received vanishing coefficients/shares from ", i);
        complaints_local.insert(cab);
      }
    }
    ++i;
  }
  return complaints_local;
}

/**
 * For all complaint answers received in defense of a complaint we check the exposed secret share
 * is consistent with the broadcasted coefficients
 *
 * @param answer Pointer of message containing some secret shares of sender
 * @param from_id Muddle address of sender
 * @param from_index Index of sender in cabinet_
 */
void DistributedKeyGeneration::CheckComplaintAnswer(SharesMessage const &answer,
                                                    MuddleAddress const &from_id,
                                                    uint32_t             from_index)
{
  // If not enough answers are sent for number of complaints against a node then add a complaint a
  // against it
  auto diff = complaints_manager_.ComplaintsCount(from_id) - answer.shares().size();
  if (diff > 0 && diff <= cabinet_.size())
  {
    complaints_answer_manager_.Add(from_id);
  }
  for (auto const &share : answer.shares())
  {
    uint32_t reporter_index{CabinetIndex(share.first)};
    // Verify shares received
    bn::Fr s, sprime;
    bn::G2 lhsG, rhsG;
    s.clear();
    sprime.clear();
    lhsG.clear();
    rhsG.clear();
    s.setStr(share.second.first);
    sprime.setStr(share.second.second);
    rhsG = ComputeRHS(reporter_index, C_ik[from_index]);
    lhsG = ComputeLHS(group_g_, group_h_, s, sprime);
    if (lhsG != rhsG)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " verification for node ",
                     CabinetIndex(from_id), " complaint answer failed");
      complaints_answer_manager_.Add(from_id);
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " verification for node ",
                     CabinetIndex(from_id), " complaint answer succeeded");
      if (reporter_index == cabinet_index_)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " reset shares for ", from_index);
        s_ij[from_index][cabinet_index_]      = s;
        sprime_ij[from_index][cabinet_index_] = sprime;
        g__s_ij[from_index][cabinet_index_].clear();
        bn::G2::mul(g__s_ij[from_index][cabinet_index_], group_g_,
                    s_ij[from_index][cabinet_index_]);
      }
    }
  }
}

/**
 * Builds the set of qualified members of the cabinet.  Altogether, complaints consists of
  // 1. Nodes which received over t complaints
  // 2. Complaint answers which were false

 * @return True if self is in qual and qual is at least of size threshold_ + 1, false otherwise
 */
bool DistributedKeyGeneration::BuildQual()
{
  qual_ = complaints_answer_manager_.BuildQual(cabinet_);
  if (qual_.find(address_) == qual_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " build QUAL failed as not in QUAL");
    return false;
  }
  else if (qual_.size() <= threshold_)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " build QUAL failed as size ",
                   qual_.size(), " less than threshold ", threshold_);
    return false;
  }
  return true;
}

/**
 * Checks coefficients sent by qual members and puts their address and the secret shares we received
 * from them into a complaints maps if the coefficients are not valid
 *
 * @return Map of address and pair of secret shares for each qual member we wish to complain against
 */
DistributedKeyGeneration::SharesExposedMap DistributedKeyGeneration::ComputeQualComplaints()
{
  SharesExposedMap qual_complaints;

  uint32_t i  = 0;
  auto     iq = cabinet_.begin();
  for (auto const &miner : qual_)
  {
    while (*iq != miner)
    {
      ++iq;
      ++i;
    }
    if (i != cabinet_index_)
    {
      // Can only require this if G, H do not take the default values from clear()
      if (A_ik[i][0] != zeroG2_)
      {
        bn::G2 rhs, lhs;
        lhs = g__s_ij[i][cabinet_index_];
        rhs = ComputeRHS(cabinet_index_, A_ik[i]);
        if (lhs != rhs)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                         " received qual coefficients from node ", i, " which failed verification");
          qual_complaints.insert(
              {miner, {s_ij[i][cabinet_index_].getStr(), sprime_ij[i][cabinet_index_].getStr()}});
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       "received vanishing qual coefficients from node ", i);
        qual_complaints.insert(
            {miner, {s_ij[i][cabinet_index_].getStr(), sprime_ij[i][cabinet_index_].getStr()}});
      }
    }
  }
  return qual_complaints;
}

/**
 * Checks the complaints set by qual members
 */
void DistributedKeyGeneration::CheckQualComplaints()
{
  for (const auto &complaint : qual_complaints_manager_.ComplaintsReceived())
  {
    MuddleAddress sender = complaint.first;
    // Return if the sender not in QUAL
    if (qual_.find(sender) == qual_.end())
    {
      return;
    }
    uint32_t from_index{CabinetIndex(sender)};
    for (auto const &share : complaint.second)
    {
      // Check person who's shares are being exposed is not in QUAL then don't bother with checks
      if (qual_.find(share.first) != qual_.end())
      {
        uint32_t victim_index = CabinetIndex(share.first);
        // verify complaint, i.e. (4) holds (5) not
        bn::G2 lhs, rhs;
        bn::Fr s, sprime;
        lhs.clear();
        rhs.clear();
        s.clear();
        sprime.clear();
        s.setStr(share.second.first);
        sprime.setStr(share.second.second);
        // check equation (4)
        lhs = ComputeLHS(group_g_, group_h_, s, sprime);
        rhs = ComputeRHS(from_index, C_ik[victim_index]);
        if (lhs != rhs)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                         " received shares failing initial coefficients verification from node ",
                         from_index, " for node ", victim_index);
          qual_complaints_manager_.Complaints(sender);
        }
        else
        {
          // check equation (5)
          bn::G2::mul(lhs, group_g_, s);  // G^s
          rhs = ComputeRHS(from_index, A_ik[victim_index]);
          if (lhs != rhs)
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                           " received shares failing qual coefficients verification from node ",
                           from_index, " for node ", victim_index);
            qual_complaints_manager_.Complaints(share.first);
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                           " received incorrect complaint from ", from_index);
            qual_complaints_manager_.Complaints(sender);
          }
        }
      }
    }
  }
}

/**
 * If in qual a member computes individual share of the secret key and further computes and
 * broadcasts qual coefficients
 */
void DistributedKeyGeneration::ComputeSecretShare()
{
  secret_share_.clear();
  xprime_i = 0;
  for (auto const &iq : qual_)
  {
    uint32_t iq_index = CabinetIndex(iq);
    bn::Fr::add(secret_share_, secret_share_, s_ij[iq_index][cabinet_index_]);
    bn::Fr::add(xprime_i, xprime_i, sprime_ij[iq_index][cabinet_index_]);
  }
}

/**
 * Run polynomial interpolation on the exposed secret shares of other cabinet members to
 * recontruct their random polynomials
 *
 * @return Bool for whether reconstruction from shares was successful
 */
bool DistributedKeyGeneration::RunReconstruction()
{
  std::vector<std::vector<bn::Fr>> a_ik;
  Init(a_ik, static_cast<uint32_t>(cabinet_.size()), static_cast<uint32_t>(threshold_ + 1));
  for (auto const &in : reconstruction_shares)
  {
    std::set<uint32_t>  parties{in.second.first};
    std::vector<bn::Fr> shares{in.second.second};
    if (parties.size() <= threshold_)
    {
      // Do not have enough good shares to be able to do reconstruction
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " reconstruction for ",
                     CabinetIndex(in.first), " failed with party size ", parties.size());
      return false;
    }
    // compute $z_i$ using Lagrange interpolation (without corrupted parties)
    uint32_t victim_index{CabinetIndex(in.first)};
    z_i[victim_index] = ComputeZi(in.second.first, in.second.second);
    std::vector<bn::Fr> points, shares_f;
    for (const auto &index : parties)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " run reconstruction for node ",
                     victim_index, " with shares from node ", index);
      points.push_back(index + 1);  // adjust index in computation
      shares_f.push_back(shares[index]);
    }
    a_ik[victim_index] = InterpolatePolynom(points, shares_f);
    for (size_t k = 0; k <= threshold_; k++)
    {
      bn::G2::mul(A_ik[victim_index][k], group_g_, a_ik[victim_index][k]);
    }
  }
  return true;
}

/**
 * Compute group public key and individual public key shares
 */
void DistributedKeyGeneration::ComputePublicKeys()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " compute public keys");
  // For all parties in $QUAL$, set $y_i = A_{i0} = g^{z_i} \bmod p$.
  for (auto const &iq : qual_)
  {
    uint32_t it{CabinetIndex(iq)};
    y_i[it] = A_ik[it][0];
  }
  // Compute public key $y = \prod_{i \in QUAL} y_i \bmod p$
  public_key_.clear();
  for (auto const &iq : qual_)
  {
    uint32_t it{CabinetIndex(iq)};
    bn::G2::add(public_key_, public_key_, y_i[it]);
  }
  // Compute public_key_shares_ $v_j = \prod_{i \in QUAL} \prod_{k=0}^t (A_{ik})^{j^k} \bmod
  // p$
  for (auto const &jq : qual_)
  {
    uint32_t jt{CabinetIndex(jq)};
    for (auto const &iq : qual_)
    {
      uint32_t it{CabinetIndex(iq)};
      bn::G2::add(public_key_shares_[jt], public_key_shares_[jt], A_ik[it][0]);
      UpdateRHS(jt, public_key_shares_[jt], A_ik[it]);
    }
  }
  finished_ = true;
}

/**
 * Helper function for getting index of cabinet memebers
 *
 * @param other_address Muddle address of member
 * @return Index of member in cabinet_
 */
uint32_t DistributedKeyGeneration::CabinetIndex(MuddleAddress const &other_address) const
{
  return static_cast<uint32_t>(std::distance(cabinet_.begin(), cabinet_.find(other_address)));
}

/**
 * Helper function to check basic details of the message to determine if it should be processed
 *
 * @param from Muddle address of sender
 * @param msg_ptr Shared pointer of message
 * @return Bool of whether the message passes the test or not
 */
bool DistributedKeyGeneration::BasicMsgCheck(MuddleAddress const &              from,
                                             std::shared_ptr<DKGMessage> const &msg_ptr)
{
  if (msg_ptr == nullptr)
  {
    return false;
  }
  else if (cabinet_.find(from) == cabinet_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " received message from unknown sender");
    return false;
  }
  return true;
}

/**
 * Resets cabinet for next round of DKG
 */
void DistributedKeyGeneration::ResetCabinet()
{
  assert(cabinet_.find(address_) != cabinet_.end());  // We should be in the cabinet

  std::lock_guard<std::mutex> lock_{mutex_};
  finished_      = false;
  state_         = State::INITIAL;
  cabinet_index_ = static_cast<uint32_t>(std::distance(cabinet_.begin(), cabinet_.find(address_)));
  auto cabinet_size{static_cast<uint32_t>(cabinet_.size())};
  auto polynomial_size{static_cast<uint32_t>(threshold_ + 1)};
  secret_share_.clear();
  public_key_.clear();
  qual_.clear();
  xprime_i.clear();
  Init(y_i, cabinet_size);
  Init(public_key_shares_, cabinet_size);
  Init(s_ij, cabinet_size, cabinet_size);
  Init(sprime_ij, cabinet_size, cabinet_size);
  Init(z_i, cabinet_size);
  Init(C_ik, cabinet_size, polynomial_size);
  Init(A_ik, cabinet_size, polynomial_size);
  Init(g__s_ij, cabinet_size, cabinet_size);
  Init(g__a_i, polynomial_size);

  complaints_manager_.ResetCabinet(cabinet_size);
  complaints_answer_manager_.ResetCabinet(cabinet_size);
  qual_complaints_manager_.Reset();

  received_all_coef_and_shares_       = false;
  received_all_complaints_            = false;
  received_all_complaints_answer_     = false;
  received_all_qual_shares_           = false;
  received_all_qual_complaints_       = false;
  received_all_reconstruction_shares_ = false;

  shares_received_                = 0;
  C_ik_received_                  = 0;
  reconstruction_shares_received_ = 0;

  A_ik_received_.clear();
  reconstruction_shares.clear();
}

/**
 * Sets the output DKG into the input variables
 *
 * @param public_key Group public key
 * @param secret_share Individual share of secret key
 * @param public_key_shares Vector of the public key shares of cabinet members
 * @param qual Set of muddle address of members in qual
 */
void DistributedKeyGeneration::SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                                            std::vector<bn::G2> &    public_key_shares,
                                            std::set<MuddleAddress> &qual)
{
  Init(public_key_shares, static_cast<uint32_t>(cabinet_.size()));
  public_key        = public_key_;
  secret_share      = secret_share_;
  public_key_shares = public_key_shares_;
  qual              = qual_;
}

/**
 * @return Whether DKG has finished
 */
bool DistributedKeyGeneration::finished() const
{
  return finished_.load();
}

/**
 * @return Generator of group used in DKG needed for threshold signatures
 */
bn::G2 DistributedKeyGeneration::group() const
{
  return group_g_;
}

}  // namespace dkg
}  // namespace fetch
