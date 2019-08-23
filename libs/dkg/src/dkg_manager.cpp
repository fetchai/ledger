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

#include "dkg/dkg_manager.hpp"

namespace fetch {
namespace dkg {

bn::G2 DkgManager::zeroG2_;
bn::Fr DkgManager::zeroFr_;
bn::G2 DkgManager::group_g_;
bn::G2 DkgManager::group_h_;

constexpr char const *LOGGING_NAME = "DKG";

DkgManager::DkgManager(MuddleAddress address)
  : address_{std::move(address)}
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

void DkgManager::GenerateCoefficients()
{
  std::vector<bn::Fr> a_i(polynomial_degree_ + 1, zeroFr_), b_i(polynomial_degree_ + 1, zeroFr_);
  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    a_i[k].setRand();
    b_i[k].setRand();
  }

  // Let z_i = f(0)
  z_i[cabinet_index_] = a_i[0];

  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    C_ik[cabinet_index_][k] = ComputeLHS(g__a_i[k], group_g_, group_h_, a_i[k], b_i[k]);
  }

  for (uint32_t l = 0; l < cabinet_size_; l++)
  {
    ComputeShares(s_ij[cabinet_index_][l], sprime_ij[cabinet_index_][l], a_i, b_i, l);
  }
}

std::vector<DkgManager::Coefficient> DkgManager::GetCoefficients()
{
  std::vector<Coefficient> coefficients;
  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    coefficients.push_back(C_ik[cabinet_index_][k].getStr());
  }
  return coefficients;
}

std::pair<DkgManager::Share, DkgManager::Share> DkgManager::GetOwnShares(
    MuddleAddress const &receiver)
{
  uint32_t receiver_index = identity_to_index_[receiver];
  bn::Fr   sij            = s_ij[cabinet_index_][receiver_index];
  bn::Fr   sprimeij       = sprime_ij[cabinet_index_][receiver_index];

  std::pair<Share, Share> shares_j{sij.getStr(), sprimeij.getStr()};
  return shares_j;
}

std::pair<DkgManager::Share, DkgManager::Share> DkgManager::GetReceivedShares(
    MuddleAddress const &owner)
{
  std::pair<Share, Share> shares_j{s_ij[identity_to_index_[owner]][cabinet_index_].getStr(),
                                   sprime_ij[identity_to_index_[owner]][cabinet_index_].getStr()};
  return shares_j;
}

void DkgManager::AddCoefficients(MuddleAddress const &           from,
                                 std::vector<Coefficient> const &coefficients)
{
  for (uint32_t ii = 0; ii <= polynomial_degree_; ++ii)
  {
      C_ik[identity_to_index_[from]][ii].setStr(coefficients[ii]);
  }
}

void DkgManager::AddShares(MuddleAddress const &from, std::pair<Share, Share> const &shares)
{
  uint32_t from_index = identity_to_index_[from];
    s_ij[from_index][cabinet_index_].setStr(shares.first);
    sprime_ij[from_index][cabinet_index_].setStr(shares.second);
}

/**
 * Checks coefficients broadcasted by cabinet member c_i is consistent with the secret shares
 * received from c_i. If false then add to complaints
 *
 * @return Set of muddle addresses of nodes we complain against
 */
std::unordered_set<DkgManager::MuddleAddress> DkgManager::ComputeComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local;
  for (auto &cab : identity_to_index_)
  {
    uint32_t i = cab.second;
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
                         " received bad coefficients/shares from node ", i);
          complaints_local.insert(cab.first);
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " received vanishing coefficients/shares from ", i);
        complaints_local.insert(cab.first);
      }
    }
    ++i;
  }
  return complaints_local;
}

bool DkgManager::VerifyComplaintAnswer(MuddleAddress const &from, ComplaintAnswer const &answer)
{
  uint32_t reporter_index = identity_to_index_[answer.first];
  uint32_t from_index     = identity_to_index_[from];
  // Verify shares received
  bn::Fr s, sprime;
  bn::G2 lhsG, rhsG;
  s.clear();
  sprime.clear();
  lhsG.clear();
  rhsG.clear();
  s.setStr(answer.second.first);
  sprime.setStr(answer.second.second);
  rhsG = ComputeRHS(reporter_index, C_ik[from_index]);
  lhsG = ComputeLHS(group_g_, group_h_, s, sprime);
  if (lhsG != rhsG)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " verification for node ", from_index,
                   " complaint answer failed");
    return false;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " verification for node ", from_index,
                   " complaint answer succeeded");
    if (reporter_index == cabinet_index_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " reset shares for ", from_index);
      s_ij[from_index][cabinet_index_]      = s;
      sprime_ij[from_index][cabinet_index_] = sprime;
      g__s_ij[from_index][cabinet_index_].clear();
      bn::G2::mul(g__s_ij[from_index][cabinet_index_], group_g_, s_ij[from_index][cabinet_index_]);
    }
    return true;
  }
}

/**
 * If in qual a member computes individual share of the secret key and further computes and
 * broadcasts qual coefficients
 */
void DkgManager::ComputeSecretShare(std::set<MuddleAddress> const &qual)
{
  secret_share_.clear();
  xprime_i = 0;
  for (auto const &iq : qual)
  {
    uint32_t iq_index = identity_to_index_[iq];
    bn::Fr::add(secret_share_, secret_share_, s_ij[iq_index][cabinet_index_]);
    bn::Fr::add(xprime_i, xprime_i, sprime_ij[iq_index][cabinet_index_]);
  }
}

std::vector<DkgManager::Coefficient> DkgManager::GetQualCoefficients()
{
  std::vector<Coefficient> coefficients;
  for (size_t k = 0; k <= polynomial_degree_; k++)
  {
    A_ik[cabinet_index_][k] = g__a_i[k];
    coefficients.push_back(A_ik[cabinet_index_][k].getStr());
  }
  return coefficients;
}

void DkgManager::AddQualCoefficients(const MuddleAddress &           from,
                                     std::vector<Coefficient> const &coefficients)
{
  uint32_t from_index = identity_to_index_[from];
  for (uint32_t ii = 0; ii <= polynomial_degree_; ++ii)
  {
      A_ik[from_index][ii].setStr(coefficients[ii]);
  }
}

/**
 * Checks coefficients sent by qual members and puts their address and the secret shares we received
 * from them into a complaints maps if the coefficients are not valid
 *
 * @return Map of address and pair of secret shares for each qual member we wish to complain against
 */
DkgManager::SharesExposedMap DkgManager::ComputeQualComplaints(std::set<MuddleAddress> const &qual)
{
  SharesExposedMap qual_complaints;

  for (auto const &miner : qual)
  {
    uint32_t i = identity_to_index_[miner];
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

DkgManager::MuddleAddress DkgManager::VerifyQualComplaint(MuddleAddress const &  from,
                                                          ComplaintAnswer const &answer)
{
  uint32_t from_index   = identity_to_index_[from];
  uint32_t victim_index = identity_to_index_[answer.first];
  // verify complaint, i.e. (4) holds (5) not
  bn::G2 lhs, rhs;
  bn::Fr s, sprime;
  lhs.clear();
  rhs.clear();
  s.clear();
  sprime.clear();
  s.setStr(answer.second.first);
  sprime.setStr(answer.second.second);
  // check equation (4)
  lhs = ComputeLHS(group_g_, group_h_, s, sprime);
  rhs = ComputeRHS(from_index, C_ik[victim_index]);
  if (lhs != rhs)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                   " received shares failing initial coefficients verification from node ",
                   from_index, " for node ", victim_index);
    return from;
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
      return answer.first;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " received incorrect complaint from ",
                     from_index);
      return from;
    }
  }
}

/**
 * Compute group public key and individual public key shares
 */

void DkgManager::ComputePublicKeys(std::set<MuddleAddress> const &qual)
{

  FETCH_LOG_INFO(LOGGING_NAME, "Node: ", cabinet_index_, " compute public keys");
  // For all parties in $QUAL$, set $y_i = A_{i0} = g^{z_i} \bmod p$.
  for (auto const &iq : qual)
  {
    uint32_t it = identity_to_index_[iq];
    y_i[it]     = A_ik[it][0];
  }
  // Compute public key $y = \prod_{i \in QUAL} y_i \bmod p$
  public_key_.clear();
  for (auto const &iq : qual)
  {
    uint32_t it = identity_to_index_[iq];
    bn::G2::add(public_key_, public_key_, y_i[it]);
  }
  // Compute public_key_shares_ $v_j = \prod_{i \in QUAL} \prod_{k=0}^t (A_{ik})^{j^k} \bmod
  // p$
  for (auto const &jq : qual)
  {
    uint32_t jt = identity_to_index_[jq];
    for (auto const &iq : qual)
    {
      uint32_t it = identity_to_index_[iq];
      bn::G2::add(public_key_shares_[jt], public_key_shares_[jt], A_ik[it][0]);
      UpdateRHS(jt, public_key_shares_[jt], A_ik[it]);
    }
  }
}

void DkgManager::AddReconstructionShare(MuddleAddress const &address)
{
  uint32_t index = identity_to_index_[address];
  reconstruction_shares.insert({address, {{}, std::vector<bn::Fr>(cabinet_size_, zeroFr_)}});
  reconstruction_shares.at(address).first.insert(cabinet_index_);
  reconstruction_shares.at(address).second[cabinet_index_] = s_ij[index][cabinet_index_];
}

void DkgManager::AddReconstructionShare(MuddleAddress const &                  from,
                                        std::pair<MuddleAddress, Share> const &share)
{
  uint32_t from_index = identity_to_index_[from];
  uint32_t index      = identity_to_index_[share.first];
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, "received good share from node ",
                 from_index, "for reconstructing node ", index);
  if (reconstruction_shares.find(share.first) == reconstruction_shares.end())
  {
    reconstruction_shares.insert({share.first, {{}, std::vector<bn::Fr>(cabinet_size_, zeroFr_)}});
  }
  else if (reconstruction_shares.at(share.first).second[from_index] != zeroFr_)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                   " received duplicate reconstruction shares from node ", from_index);
    return;
  }
  bn::Fr s;
  s.clear();
  s.setStr(share.second);
  reconstruction_shares.at(share.first).first.insert(from_index);
  reconstruction_shares.at(share.first).second[from_index] = s;
}

void DkgManager::VerifyReconstructionShare(MuddleAddress const &from, ExposedShare const &share)
{
  uint32_t victim_index = identity_to_index_[share.first];
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
  rhs = ComputeRHS(identity_to_index_[from], C_ik[victim_index]);
  // check equation (4)
  if (lhs == rhs)
  {
    AddReconstructionShare(from, {share.first, share.second.first});
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, "received bad share from node ",
                   identity_to_index_[from], "for reconstructing node ", victim_index);
  }
}

/**
 * Run polynomial interpolation on the exposed secret shares of other cabinet members to
 * recontruct their random polynomials
 *
 * @return Bool for whether reconstruction from shares was successful
 */
bool DkgManager::RunReconstruction()
{
  std::vector<std::vector<bn::Fr>> a_ik;
  Init(a_ik, static_cast<uint32_t>(cabinet_size_), static_cast<uint32_t>(polynomial_degree_ + 1));
  for (auto const &in : reconstruction_shares)
  {
    uint32_t            victim_index = identity_to_index_[in.first];
    std::set<uint32_t>  parties{in.second.first};
    std::vector<bn::Fr> shares{in.second.second};
    if (parties.size() <= polynomial_degree_)
    {
      // Do not have enough good shares to be able to do reconstruction
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " reconstruction for ", victim_index,
                     " failed with party size ", parties.size());
      return false;
    }
    else if (in.first == address_)
    {
      // Do not run reconstruction for myself
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", cabinet_index_, " polynomial being reconstructed.");
      continue;
    }
    // compute $z_i$ using Lagrange interpolation (without corrupted parties)
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
    for (size_t k = 0; k <= polynomial_degree_; k++)
    {
      bn::G2::mul(A_ik[victim_index][k], group_g_, a_ik[victim_index][k]);
    }
  }
  return true;
}

void DkgManager::Reset(std::set<MuddleAddress> const &cabinet, uint32_t threshold)
{
  assert(threshold > 0);
  auto cabinet_size{static_cast<uint32_t>(cabinet.size())};
  cabinet_size_      = cabinet_size;
  polynomial_degree_ = threshold - 1;

  // identity_to_index_
  uint32_t index = 0;
  for (auto const &cab : cabinet)
  {
    if (cab == address_)
    {
      cabinet_index_ = index;
    }
    identity_to_index_.insert({cab, index});
    ++index;
  }

  secret_share_.clear();
  public_key_.clear();
  xprime_i.clear();
  Init(y_i, cabinet_size_);
  Init(public_key_shares_, cabinet_size_);
  Init(s_ij, cabinet_size_, cabinet_size_);
  Init(sprime_ij, cabinet_size_, cabinet_size_);
  Init(z_i, cabinet_size_);
  Init(C_ik, cabinet_size_, threshold);
  Init(A_ik, cabinet_size_, threshold);
  Init(g__s_ij, cabinet_size_, cabinet_size_);
  Init(g__a_i, threshold);

  reconstruction_shares.clear();
}

void DkgManager::SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                              std::vector<bn::G2> &public_key_shares)
{
  Init(public_key_shares, static_cast<uint32_t>(cabinet_size_));
  public_key        = public_key_;
  secret_share      = secret_share_;
  public_key_shares = public_key_shares_;
}

}  // namespace dkg
}  // namespace fetch
