//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/synchronisation/protected.hpp"
#include "crypto/ecdsa.hpp"
#include "network/generics/milli_timer.hpp"

#include <mutex>
#include <utility>
#include <vector>

namespace fetch {
namespace dkg {
namespace {

class CurveParameters
{
public:
  // Construction / Destruction
  CurveParameters()                        = default;
  CurveParameters(CurveParameters const &) = delete;
  CurveParameters(CurveParameters &&)      = delete;
  ~CurveParameters()                       = default;

  BeaconManager::PrivateKey const &GetZeroFr()
  {
    EnsureInitialised();
    return params_->zeroFr_;
  }

  BeaconManager::Generator const &GetGroupG()
  {
    EnsureInitialised();
    return params_->group_g_;
  }

  BeaconManager::Generator const &GetGroupH()
  {
    EnsureInitialised();
    return params_->group_h_;
  }

  void EnsureInitialised()
  {
    if (!params_)
    {
      params_ = std::make_unique<Params>();
      crypto::mcl::SetGenerators(params_->group_g_, params_->group_h_);
    }
  }

  // Operators
  CurveParameters &operator=(CurveParameters const &) = delete;
  CurveParameters &operator=(CurveParameters &&) = delete;

private:
  struct Params
  {
    BeaconManager::PrivateKey zeroFr_{};

    BeaconManager::Generator group_g_{};
    BeaconManager::Generator group_h_{};
  };

  std::unique_ptr<Params> params_;
};

Protected<CurveParameters> curve_params_{};

}  // namespace

constexpr char const *LOGGING_NAME = "BeaconManager";

BeaconManager::BeaconManager(CertificatePtr certificate)
  : certificate_{std::move(certificate)}
{
  if (certificate_ == nullptr)
  {
    auto ptr = new crypto::ECDSASigner();
    certificate_.reset(ptr);
    ptr->GenerateKeys();
  }

  curve_params_.ApplyVoid([](CurveParameters &params) { params.EnsureInitialised(); });
}

void BeaconManager::SetCertificate(CertificatePtr certificate)
{
  certificate_ = std::move(certificate);
}

void BeaconManager::GenerateCoefficients()
{
  std::vector<PrivateKey> a_i(polynomial_degree_ + 1, GetZeroFr());
  std::vector<PrivateKey> b_i(polynomial_degree_ + 1, GetZeroFr());
  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    a_i[k].setRand();
    b_i[k].setRand();
  }

  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    C_ik[cabinet_index_][k] =
        crypto::mcl::ComputeLHS(g__a_i[k], GetGroupG(), GetGroupH(), a_i[k], b_i[k]);
  }

  for (uint32_t l = 0; l < cabinet_size_; l++)
  {
    crypto::mcl::ComputeShares(s_ij[cabinet_index_][l], sprime_ij[cabinet_index_][l], a_i, b_i, l);
  }
}

std::vector<BeaconManager::Coefficient> BeaconManager::GetCoefficients()
{
  std::vector<Coefficient> coefficients;
  for (uint32_t k = 0; k <= polynomial_degree_; k++)
  {
    coefficients.emplace_back(C_ik[cabinet_index_][k]);
  }
  return coefficients;
}

std::pair<BeaconManager::Share, BeaconManager::Share> BeaconManager::GetOwnShares(
    MuddleAddress const &receiver)
{
  CabinetIndex receiver_index = identity_to_index_[receiver];
  PrivateKey   sij            = s_ij[cabinet_index_][receiver_index];
  PrivateKey   sprimeij       = sprime_ij[cabinet_index_][receiver_index];

  std::pair<Share, Share> shares_j{sij, sprimeij};
  return shares_j;
}

std::pair<BeaconManager::Share, BeaconManager::Share> BeaconManager::GetReceivedShares(
    MuddleAddress const &owner)
{
  std::pair<Share, Share> shares_j{s_ij[identity_to_index_[owner]][cabinet_index_],
                                   sprime_ij[identity_to_index_[owner]][cabinet_index_]};
  return shares_j;
}

void BeaconManager::AddCoefficients(MuddleAddress const &           from,
                                    std::vector<Coefficient> const &coefficients)
{
  if (coefficients.size() == polynomial_degree_ + 1)
  {
    for (uint32_t i = 0; i <= polynomial_degree_; ++i)
    {
      C_ik[identity_to_index_[from]][i] = coefficients[i];
    }
    return;
  }
  FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                 " received coefficients of incorrect size from node ", identity_to_index_[from]);
}

void BeaconManager::AddShares(MuddleAddress const &from, std::pair<Share, Share> const &shares)
{
  CabinetIndex from_index               = identity_to_index_[from];
  s_ij[from_index][cabinet_index_]      = shares.first;
  sprime_ij[from_index][cabinet_index_] = shares.second;
}

/**
 * Checks coefficients broadcasted by cabinet member c_i is consistent with the secret shares
 * received from c_i. If false then add to complaints
 *
 * @return Set of muddle addresses of nodes we complain against
 */
std::set<BeaconManager::MuddleAddress> BeaconManager::ComputeComplaints(
    std::set<MuddleAddress> const &coeff_received)
{
  std::set<MuddleAddress> complaints_local;
  for (auto &cab : coeff_received)
  {
    CabinetIndex i = identity_to_index_[cab];
    if (i != cabinet_index_)
    {
      PublicKey rhs;
      PublicKey lhs;
      lhs = crypto::mcl::ComputeLHS(g__s_ij[i][cabinet_index_], GetGroupG(), GetGroupH(),
                                    s_ij[i][cabinet_index_], sprime_ij[i][cabinet_index_]);
      rhs = crypto::mcl::ComputeRHS(cabinet_index_, C_ik[i]);
      if (lhs != rhs || lhs.isZero())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " received bad coefficients/shares from node ", i);
        complaints_local.insert(cab);
      }
    }
  }
  return complaints_local;
}

bool BeaconManager::VerifyComplaintAnswer(MuddleAddress const &from, ComplaintAnswer const &answer)
{
  CabinetIndex from_index = identity_to_index_[from];
  assert(identity_to_index_.find(answer.first) != identity_to_index_.end());
  CabinetIndex reporter_index = identity_to_index_[answer.first];
  // Verify shares received
  PrivateKey s;
  PrivateKey sprime;
  PublicKey  lhsG;
  PublicKey  rhsG;
  s      = answer.second.first;
  sprime = answer.second.second;
  rhsG   = crypto::mcl::ComputeRHS(reporter_index, C_ik[from_index]);
  lhsG   = crypto::mcl::ComputeLHS(GetGroupG(), GetGroupH(), s, sprime);
  if (lhsG != rhsG || lhsG.isZero())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " verification for node ", from_index,
                   " complaint answer failed");
    return false;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " verification for node ", from_index,
                 " complaint answer succeeded");
  if (reporter_index == cabinet_index_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", cabinet_index_, " reset shares for ", from_index);
    s_ij[from_index][cabinet_index_]      = s;
    sprime_ij[from_index][cabinet_index_] = sprime;
    g__s_ij[from_index][cabinet_index_].clear();
    bn::G2::mul(g__s_ij[from_index][cabinet_index_], GetGroupG(), s_ij[from_index][cabinet_index_]);
  }
  return true;
}

/**
 * If in qual a member computes individual share of the secret key and further computes and
 * broadcasts qual coefficients
 */
void BeaconManager::ComputeSecretShare()
{
  for (auto const &iq : qual_)
  {
    CabinetIndex iq_index = identity_to_index_[iq];
    bn::Fr::add(secret_share_, secret_share_, s_ij[iq_index][cabinet_index_]);
    bn::Fr::add(xprime_i, xprime_i, sprime_ij[iq_index][cabinet_index_]);
  }
}

std::vector<BeaconManager::Coefficient> BeaconManager::GetQualCoefficients()
{
  std::vector<Coefficient> coefficients;
  for (std::size_t k = 0; k <= polynomial_degree_; k++)
  {
    A_ik[cabinet_index_][k] = g__a_i[k];
    coefficients.push_back(A_ik[cabinet_index_][k]);
  }
  return coefficients;
}

void BeaconManager::AddQualCoefficients(MuddleAddress const &           from,
                                        std::vector<Coefficient> const &coefficients)
{
  CabinetIndex from_index = identity_to_index_[from];
  if (coefficients.size() == polynomial_degree_ + 1)
  {
    for (uint32_t i = 0; i <= polynomial_degree_; ++i)
    {
      A_ik[from_index][i] = coefficients[i];
    }
    return;
  }
  FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                 " received qual coefficients of incorrect size from node ",
                 identity_to_index_[from]);
}

/**
 * Checks coefficients sent by qual members and puts their address and the secret shares we received
 * from them into a complaints maps if the coefficients are not valid
 *
 * @return Map of address and pair of secret shares for each qual member we wish to complain against
 */
BeaconManager::SharesExposedMap BeaconManager::ComputeQualComplaints(
    std::set<MuddleAddress> const &coeff_received)
{
  SharesExposedMap qual_complaints;

  for (auto const &miner : qual_)
  {
    CabinetIndex i = identity_to_index_[miner];
    if (i != cabinet_index_)
    {
      if (coeff_received.find(miner) != coeff_received.end())
      {
        PublicKey rhs;
        PublicKey lhs;
        lhs = g__s_ij[i][cabinet_index_];
        rhs = crypto::mcl::ComputeRHS(cabinet_index_, A_ik[i]);
        if (lhs != rhs || rhs.isZero())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                         " received qual coefficients from node ", i, " which failed verification");
          qual_complaints.insert({miner, {s_ij[i][cabinet_index_], sprime_ij[i][cabinet_index_]}});
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                       " did not receive qual coefficients from node ", i);
        qual_complaints.insert({miner, {s_ij[i][cabinet_index_], sprime_ij[i][cabinet_index_]}});
      }
    }
  }
  return qual_complaints;
}

BeaconManager::MuddleAddress BeaconManager::VerifyQualComplaint(MuddleAddress const &  from,
                                                                ComplaintAnswer const &answer)
{
  CabinetIndex from_index   = identity_to_index_[from];
  CabinetIndex victim_index = identity_to_index_[answer.first];

  PublicKey  lhs;
  PublicKey  rhs;
  PrivateKey s;
  PrivateKey sprime;
  s      = answer.second.first;
  sprime = answer.second.second;
  lhs    = crypto::mcl::ComputeLHS(GetGroupG(), GetGroupH(), s, sprime);
  rhs    = crypto::mcl::ComputeRHS(from_index, C_ik[victim_index]);
  if (lhs != rhs || lhs.isZero())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_,
                    " received shares failing initial coefficients verification from node ",
                    from_index, " for node ", victim_index);
    return from;
  }

  bn::G2::mul(lhs, GetGroupG(), s);  // G^s
  rhs = crypto::mcl::ComputeRHS(from_index, A_ik[victim_index]);
  if (lhs != rhs || rhs.isZero())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_,
                    " received shares failing qual coefficients verification from node ",
                    from_index, " for node ", victim_index);
    return answer.first;
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " received incorrect complaint from ",
                 from_index);
  return from;
}

/**
 * Compute group public key and individual public key shares
 */
void BeaconManager::ComputePublicKeys()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_, " compute public keys begin.");
  generics::MilliTimer myTimer("BeaconManager::ComputePublicKeys");

  // For all parties in $QUAL$, set $y_i = A_{i0}
  for (auto const &iq : qual_)
  {
    CabinetIndex it = identity_to_index_[iq];
    y_i[it]         = A_ik[it][0];
  }
  // Compute public key $y = \prod_{i \in QUAL} y_i \bmod p$
  for (auto const &iq : qual_)
  {
    CabinetIndex it = identity_to_index_[iq];
    bn::G2::add(public_key_, public_key_, y_i[it]);
  }
  // Compute public_key_shares_ $v_j = \prod_{i \in QUAL} \prod_{k=0}^t (A_{ik})^{j^k} \bmod
  // p$
  for (auto const &jq : qual_)
  {
    CabinetIndex jt = identity_to_index_[jq];
    for (auto const &iq : qual_)
    {
      CabinetIndex it = identity_to_index_[iq];
      bn::G2::add(public_key_shares_[jt], public_key_shares_[jt], A_ik[it][0]);
      crypto::mcl::UpdateRHS(jt, public_key_shares_[jt], A_ik[it]);
    }
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_, " compute public keys end.");
}

void BeaconManager::AddReconstructionShare(MuddleAddress const &address)
{
  CabinetIndex index = identity_to_index_[address];
  if (reconstruction_shares.find(address) == reconstruction_shares.end())
  {
    reconstruction_shares.insert(
        {address, {{}, std::vector<PrivateKey>(cabinet_size_, GetZeroFr())}});
  }
  reconstruction_shares.at(address).first.insert(cabinet_index_);
  reconstruction_shares.at(address).second[cabinet_index_] = s_ij[index][cabinet_index_];
}

void BeaconManager::AddReconstructionShare(MuddleAddress const &                  from,
                                           std::pair<MuddleAddress, Share> const &share)
{
  CabinetIndex from_index = identity_to_index_[from];
  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_, "received good share from node ",
                  from_index, "for reconstructing node ", identity_to_index_[share.first]);
  if (reconstruction_shares.find(share.first) == reconstruction_shares.end())
  {
    reconstruction_shares.insert(
        {share.first, {{}, std::vector<PrivateKey>(cabinet_size_, GetZeroFr())}});
  }
  else if (!reconstruction_shares.at(share.first).second[from_index].isZero())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_,
                   " received duplicate reconstruction shares from node ", from_index);
    return;
  }
  PrivateKey s;
  s = share.second;
  reconstruction_shares.at(share.first).first.insert(from_index);
  reconstruction_shares.at(share.first).second[from_index] = s;
}

void BeaconManager::VerifyReconstructionShare(MuddleAddress const &from, ExposedShare const &share)
{
  CabinetIndex victim_index = identity_to_index_[share.first];
  PublicKey    lhs;
  PublicKey    rhs;
  PrivateKey   s;
  PrivateKey   sprime;
  s      = share.second.first;
  sprime = share.second.second;
  lhs    = crypto::mcl::ComputeLHS(GetGroupG(), GetGroupH(), s, sprime);
  rhs    = crypto::mcl::ComputeRHS(identity_to_index_[from], C_ik[victim_index]);

  if (lhs == rhs && !lhs.isZero())
  {
    AddReconstructionShare(from, {share.first, share.second.first});
  }
  else
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_, "received bad share from node ",
                    identity_to_index_[from], "for reconstructing node ", victim_index);
  }
}

/**
 * Run polynomial interpolation on the exposed secret shares of other cabinet members to
 * recontruct their random polynomials
 *
 * @return Bool for whether reconstruction from shares was successful
 */
bool BeaconManager::RunReconstruction()
{
  std::vector<std::vector<PrivateKey>> a_ik;
  crypto::mcl::Init(a_ik, static_cast<uint32_t>(cabinet_size_),
                    static_cast<uint32_t>(polynomial_degree_ + 1));
  for (auto const &in : reconstruction_shares)
  {
    CabinetIndex            victim_index = identity_to_index_[in.first];
    std::set<CabinetIndex>  parties{in.second.first};
    std::vector<PrivateKey> shares{in.second.second};
    if (in.first == certificate_->identity().identifier())
    {
      // Do not run reconstruction for myself
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " polynomial being reconstructed.");
      continue;
    }
    if (parties.size() <= polynomial_degree_)
    {
      // Do not have enough good shares to be able to do reconstruction
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", cabinet_index_, " reconstruction for node ",
                     victim_index, " failed with party size ", parties.size());
      return false;
    }
    std::vector<PrivateKey> points;
    std::vector<PrivateKey> shares_f;
    for (const auto &index : parties)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", cabinet_index_, " run reconstruction for node ",
                      victim_index, " with shares from node ", index);
      points.emplace_back(index + 1);  // adjust index in computation
      shares_f.push_back(shares[index]);
    }
    a_ik[victim_index] = crypto::mcl::InterpolatePolynom(points, shares_f);
    for (std::size_t k = 0; k <= polynomial_degree_; k++)
    {
      bn::G2::mul(A_ik[victim_index][k], GetGroupG(), a_ik[victim_index][k]);
    }
  }
  return true;
}

BeaconManager::DkgOutput BeaconManager::GetDkgOutput()
{
  DkgOutput output{public_key_, public_key_shares_, secret_share_, qual_};
  return output;
}

void BeaconManager::SetDkgOutput(DkgOutput const &output)
{
  public_key_        = output.group_public_key;
  secret_share_      = output.private_key_share;
  public_key_shares_ = output.public_key_shares;
  qual_              = output.qual;
}

void BeaconManager::SetQual(std::set<fetch::dkg::BeaconManager::MuddleAddress> qual)
{
  qual_ = std::move(qual);
}

/**
 * @brief resets the class back to a state where a new cabinet is set up.
 * @param cabinet_size is the size of the cabinet.
 * @param threshold is the threshold to be able to generate a signature.
 */
void BeaconManager::NewCabinet(std::set<MuddleAddress> const &cabinet, uint32_t threshold)
{
  assert(threshold > 0);
  auto cabinet_size{static_cast<uint32_t>(cabinet.size())};
  cabinet_size_      = cabinet_size;
  polynomial_degree_ = threshold - 1;

  // Ordering of identities determines the index of each node
  CabinetIndex index = 0;
  for (auto const &cab : cabinet)
  {
    if (cab == certificate_->identity().identifier())
    {
      cabinet_index_ = index;
    }
    identity_to_index_.insert({cab, index});
    ++index;
  }

  Reset();
}

void BeaconManager::Reset()
{
  secret_share_.clear();
  public_key_.clear();
  xprime_i.clear();
  crypto::mcl::Init(y_i, cabinet_size_);
  crypto::mcl::Init(public_key_shares_, cabinet_size_);
  crypto::mcl::Init(s_ij, cabinet_size_, cabinet_size_);
  crypto::mcl::Init(sprime_ij, cabinet_size_, cabinet_size_);
  crypto::mcl::Init(C_ik, cabinet_size_, polynomial_degree_ + 1);
  crypto::mcl::Init(A_ik, cabinet_size_, polynomial_degree_ + 1);
  crypto::mcl::Init(g__s_ij, cabinet_size_, cabinet_size_);
  crypto::mcl::Init(g__a_i, polynomial_degree_ + 1);

  qual_.clear();
  reconstruction_shares.clear();
}

/**
 * @brief adds a signature share.
 * @param from is the identity of the sending node.
 * @param public_key is the public key of the peer.
 * @param signature is the signature part.
 */

BeaconManager::AddResult BeaconManager::AddSignaturePart(Identity const & from,
                                                         Signature const &signature)
{
  auto it = identity_to_index_.find(from.identifier());
  auto iq = qual_.find(from.identifier());

  if (it == identity_to_index_.end() || iq == qual_.end())
  {
    return AddResult::NOT_MEMBER;
  }

  if (already_signed_.find(from.identifier()) != already_signed_.end())
  {
    return AddResult::SIGNATURE_ALREADY_ADDED;
  }

  uint64_t n = it->second;
  if (!crypto::mcl::VerifySign(public_key_shares_[n], current_message_, signature, GetGroupG()))
  {
    return AddResult::INVALID_SIGNATURE;
  }

  signature_buffer_.insert({n, signature});
  already_signed_.insert(from.identifier());
  return AddResult::SUCCESS;
}

/**
 * @brief verifies the group signature.
 */
bool BeaconManager::Verify()
{
  group_signature_ = crypto::mcl::LagrangeInterpolation(signature_buffer_);
  return Verify(group_signature_);
}

/**
 * @brief verifies a group signature.
 */
bool BeaconManager::Verify(Signature const &signature)
{
  // TODO(HUT): use the static helper
  return crypto::mcl::VerifySign(public_key_, current_message_, signature, GetGroupG());
}

bool BeaconManager::Verify(byte_array::ConstByteArray const &group_public_key,
                           MessagePayload const &            message,
                           byte_array::ConstByteArray const &signature)
{
  if (group_public_key.empty() || message.empty() || signature.empty())
  {
    return false;
  }

  PublicKey tmp;
  Signature tmp2;

  // Check strings deserialise into correct MCL types
  bool check_deserialisation{false};
  tmp.setStr(&check_deserialisation, std::string(group_public_key).data());
  if (check_deserialisation)
  {
    tmp2.setStr(&check_deserialisation, std::string(signature).data());
  }

  if (!check_deserialisation)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect serial length of the group public key");
    return false;
  }

  return crypto::mcl::VerifySign(tmp, message, tmp2, GetGroupG());
}

/**
 * @brief returns the signature as a ConstByteArray
 */
BeaconManager::Signature BeaconManager::GroupSignature() const
{
  return group_signature_;
}

/**
 * @brief sets the next message to be signed.
 * @param next_message is the message to be signed.
 */
void BeaconManager::SetMessage(MessagePayload next_message)
{
  current_message_ = std::move(next_message);
  signature_buffer_.clear();
  already_signed_.clear();
  group_signature_.clear();
}

/**
 * @brief signs the current message.
 */
BeaconManager::SignedMessage BeaconManager::Sign()
{
  Signature signature = crypto::mcl::SignShare(current_message_, secret_share_);

  SignedMessage smsg;
  smsg.identity  = certificate_->identity();
  smsg.signature = signature;

  if (AddSignaturePart(certificate_->identity(), signature) == AddResult::INVALID_SIGNATURE)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Computed bad signature share");
  }

  return smsg;
}

bool BeaconManager::InQual(MuddleAddress const &address) const
{
  return qual_.find(address) != qual_.end();
}

std::set<BeaconManager::MuddleAddress> const &BeaconManager::qual() const
{
  return qual_;
}

uint32_t BeaconManager::polynomial_degree() const
{
  return polynomial_degree_;
}

BeaconManager::CabinetIndex BeaconManager::cabinet_index() const
{
  return cabinet_index_;
}

BeaconManager::CabinetIndex BeaconManager::cabinet_index(MuddleAddress const &address) const
{
  assert(identity_to_index_.find(address) != identity_to_index_.end());
  return identity_to_index_.at(address);
}

bool BeaconManager::can_verify()
{
  return signature_buffer_.size() >= polynomial_degree_ + 1;
}

std::string BeaconManager::group_public_key() const
{
  return public_key_.getStr();
}

BeaconManager::Generator const &BeaconManager::GetGroupG()
{
  return *curve_params_.Apply([](CurveParameters &params) { return &params.GetGroupG(); });
}

BeaconManager::Generator const &BeaconManager::GetGroupH()
{
  return *curve_params_.Apply([](CurveParameters &params) { return &params.GetGroupH(); });
}

BeaconManager::PrivateKey const &BeaconManager::GetZeroFr()
{
  return *curve_params_.Apply([](CurveParameters &params) { return &params.GetZeroFr(); });
}

}  // namespace dkg
}  // namespace fetch
