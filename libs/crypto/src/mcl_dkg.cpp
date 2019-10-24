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

#include "crypto/mcl_dkg.hpp"

#include "mcl/bn256.hpp"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

namespace bn = mcl::bn256;

namespace fetch {
namespace crypto {
namespace mcl {

std::atomic<bool> details::MCLInitialiser::was_initialised{false};

void SetGenerator(Generator &generator_g, std::string string_to_hash)
{
  generator_g.clear();

  if (string_to_hash.empty())
  {
    string_to_hash = "Fetch.ai Elliptic Curve Generator G";
  }
  bn::hashAndMapToG2(generator_g, string_to_hash);
}

void SetGenerators(Generator &generator_g, Generator &generator_h, std::string string_to_hash,
                   std::string string_to_hash2)
{
  generator_g.clear();
  generator_h.clear();

  if (string_to_hash.empty())
  {
    string_to_hash = "Fetch.ai Elliptic Curve Generator G";
  }
  if (string_to_hash2.empty())
  {
    string_to_hash2 = "Fetch.ai Elliptic Curve Generator h";
  }
  bn::hashAndMapToG2(generator_g, string_to_hash);
  bn::hashAndMapToG2(generator_h, string_to_hash2);
}

/**
 * LHS and RHS functions are used for checking consistency between publicly broadcasted coefficients
 * and secret shares distributed privately
 */
bn::G2 ComputeLHS(bn::G2 &tmpG, bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1,
                  bn::Fr const &share2)
{
  bn::G2 tmp2G, lhsG;
  tmp2G.clear();
  lhsG.clear();
  bn::G2::mul(tmpG, G, share1);
  bn::G2::mul(tmp2G, H, share2);
  bn::G2::add(lhsG, tmpG, tmp2G);

  return lhsG;
}

bn::G2 ComputeLHS(bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1, bn::Fr const &share2)
{
  bn::G2 tmpG;
  tmpG.clear();
  return ComputeLHS(tmpG, G, H, share1, share2);
}

void UpdateRHS(uint32_t rank, bn::G2 &rhsG, std::vector<bn::G2> const &input)
{
  bn::Fr tmpF{1};
  bn::G2 tmpG;
  tmpG.clear();
  assert(!input.empty());
  for (uint32_t k = 1; k < input.size(); k++)
  {
    bn::Fr::pow(tmpF, rank + 1, k);  // adjust rank in computation
    bn::G2::mul(tmpG, input[k], tmpF);
    bn::G2::add(rhsG, rhsG, tmpG);
  }
}

bn::G2 ComputeRHS(uint32_t rank, std::vector<bn::G2> const &input)
{
  bn::Fr tmpF;
  bn::G2 tmpG, rhsG;
  tmpG.clear();
  rhsG.clear();
  assert(!input.empty());
  // initialise rhsG
  rhsG = input[0];
  UpdateRHS(rank, rhsG, input);
  return rhsG;
}

/**
 * Given two polynomials (f and f') with coefficients a_i and b_i, we compute the evaluation of
 * these polynomials at different points
 *
 * @param s_i The value of f(index)
 * @param sprime_i The value of f'(index)
 * @param a_i The vector of coefficients for f
 * @param b_i The vector of coefficients for f'
 * @param index The point at which you evaluate the polynomial
 */
void ComputeShares(bn::Fr &s_i, bn::Fr &sprime_i, std::vector<bn::Fr> const &a_i,
                   std::vector<bn::Fr> const &b_i, uint32_t index)
{
  bn::Fr pow, tmpF;
  assert(a_i.size() == b_i.size());
  assert(!a_i.empty());
  s_i      = a_i[0];
  sprime_i = b_i[0];
  for (uint32_t k = 1; k < a_i.size(); k++)
  {
    bn::Fr::pow(pow, index + 1, k);  // adjust index in computation
    bn::Fr::mul(tmpF, pow, b_i[k]);  // j^k * b_i[k]
    bn::Fr::add(sprime_i, sprime_i, tmpF);
    bn::Fr::mul(tmpF, pow, a_i[k]);  // j^k * a_i[k]
    bn::Fr::add(s_i, s_i, tmpF);
  }
}

/**
 * Computes the coefficients of a polynomial
 *
 * @param a Points at which polynomial has been evaluated
 * @param b Value of the polynomial at points a
 * @return The vector of coefficients of the polynomial
 */
std::vector<bn::Fr> InterpolatePolynom(std::vector<bn::Fr> const &a, std::vector<bn::Fr> const &b)
{
  std::size_t m = a.size();
  if ((b.size() != m) || (m == 0))
  {
    throw std::invalid_argument("mcl_interpolate_polynom: bad m");
  }
  std::vector<bn::Fr> prod{a}, res(m, 0);
  bn::Fr              t1, t2;
  for (std::size_t k = 0; k < m; k++)
  {
    t1 = 1;
    for (auto i = static_cast<long>(k - 1); i >= 0; i--)
    {
      bn::Fr::mul(t1, t1, a[k]);
      bn::Fr::add(t1, t1, prod[static_cast<std::size_t>(i)]);
    }

    t2 = 0;
    for (auto i = static_cast<long>(k - 1); i >= 0; i--)
    {
      bn::Fr::mul(t2, t2, a[k]);
      bn::Fr::add(t2, t2, res[static_cast<std::size_t>(i)]);
    }
    bn::Fr::inv(t1, t1);

    bn::Fr::sub(t2, b[k], t2);
    bn::Fr::mul(t1, t1, t2);

    for (std::size_t i = 0; i < k; i++)
    {
      bn::Fr::mul(t2, prod[i], t1);
      bn::Fr::add(res[i], res[i], t2);
    }
    res[k] = t1;
    if (k < (m - 1))
    {
      if (k == 0)
      {
        bn::Fr::neg(prod[0], prod[0]);
      }
      else
      {
        bn::Fr::neg(t1, a[k]);
        bn::Fr::add(prod[k], t1, prod[k - 1]);
        for (auto i = static_cast<long>(k - 1); i >= 1; i--)
        {
          bn::Fr::mul(t2, prod[static_cast<std::size_t>(i)], t1);
          bn::Fr::add(prod[static_cast<std::size_t>(i)], t2, prod[static_cast<std::size_t>(i - 1)]);
        }
        bn::Fr::mul(prod[0], prod[0], t1);
      }
    }
  }
  return res;
}

/**
 * Computes signature share of a message
 *
 * @param message Message to be signed
 * @param x_i Secret key share
 * @return Signature share
 */
Signature SignShare(MessagePayload const &message, PrivateKey const &x_i)
{
  bn::Fp Hm;
  bn::G1 PH;
  bn::G1 sign;
  sign.clear();
  Hm.setHashOf(message.pointer(), message.size());
  bn::mapToG1(PH, Hm);
  bn::G1::mul(sign, PH, x_i);  // sign = s H(m)
  return sign;
}

/**
 * Verifies a signature
 *
 * @param y The public key (can be the group public key, or public key share)
 * @param message Message that was signed
 * @param sign Signature to be verified
 * @param G Generator used in DKG
 * @return
 */
bool VerifySign(PublicKey const &y, MessagePayload const &message, Signature const &sign,
                Generator const &G)
{
  bn::Fp12 e1, e2;
  bn::Fp   Hm;
  bn::G1   PH;
  Hm.setHashOf(message.pointer(), message.size());
  bn::mapToG1(PH, Hm);

  bn::pairing(e1, sign, G);
  bn::pairing(e2, PH, y);

  return e1 == e2;
}

/**
 * Computes the group signature using the indices and signature shares of threshold_ + 1
 * parties
 *
 * @param shares Unordered map of indices and their corresponding signature shares
 * @return Group signature
 */
Signature LagrangeInterpolation(std::unordered_map<CabinetIndex, Signature> const &shares)
{
  assert(!shares.empty());
  if (shares.size() == 1)
  {
    return shares.begin()->second;
  }
  bn::G1 res;
  res.clear();

  bn::Fr a = 1;
  for (auto &p : shares)
  {
    a *= bn::Fr(p.first + 1);
  }

  for (auto &p1 : shares)
  {
    auto b = static_cast<bn::Fr>(p1.first + 1);
    for (auto &p2 : shares)
    {
      if (p2.first != p1.first)
      {
        b *= static_cast<bn::Fr>(p2.first) - static_cast<bn::Fr>(p1.first);
      }
    }
    bn::G1 t;
    bn::G1::mul(t, p1.second, a / b);
    res += t;
  }
  return res;
}

/**
 * Generates the group public key, public key shares and private key share for a number of
 * parties and a given signature threshold. Nodes must be allocated the outputs according
 * to their index in the committee.
 *
 * @param committee_size Number of parties for which private key shares are generated
 * @param threshold Number of parties required to generate a group signature
 * @return Vector of DkgOutputs containing the data to be given to each party
 */
std::vector<DkgKeyInformation> TrustedDealerGenerateKeys(uint32_t committee_size,
                                                         uint32_t threshold)
{
  std::vector<DkgKeyInformation> output;
  Generator                      generator;
  SetGenerator(generator);

  // Construct polynomial of degree threshold - 1
  std::vector<PrivateKey> vec_a;
  Init(vec_a, threshold);
  for (uint32_t ii = 0; ii < threshold; ++ii)
  {
    vec_a[ii].setRand();
  }

  std::vector<PublicKey>  public_key_shares;
  std::vector<PrivateKey> private_key_shares;
  Init(public_key_shares, committee_size);
  Init(private_key_shares, committee_size);

  // Group secret key is polynomial evaluated at 0
  PublicKey group_public_key;
  group_public_key.clear();
  PrivateKey group_private_key = vec_a[0];
  bn::G2::mul(group_public_key, generator, group_private_key);

  // Generate committee public keys from their private key contributions
  for (uint32_t i = 0; i < committee_size; ++i)
  {
    PrivateKey pow;
    PrivateKey tmpF;
    PrivateKey private_key;
    private_key.clear();
    // Private key is polynomial evaluated at index i
    private_key = vec_a[0];
    for (uint32_t k = 1; k < vec_a.size(); k++)
    {
      bn::Fr::pow(pow, i + 1, k);        // adjust index in computation
      bn::Fr::mul(tmpF, pow, vec_a[k]);  // j^k * a_i[k]
      bn::Fr::add(private_key, private_key, tmpF);
    }
    // Public key from private
    PublicKey public_key;
    bn::G2::mul(public_key, generator, private_key);
    public_key_shares[i]  = public_key;
    private_key_shares[i] = private_key;
  }

  // Compute outputs for each member
  for (uint32_t i = 0; i < committee_size; ++i)
  {
    output.emplace_back(group_public_key, public_key_shares, private_key_shares[i]);
  }
  return output;
}

/**
 * Generates a private key and the corresponding public key
 *
 * @param generator Choice of generator on the elliptic curve
 * @return Pair of private and public keys
 */
std::pair<PrivateKey, PublicKey> GenerateKeyPair(Generator const &generator)
{
  std::pair<PrivateKey, PublicKey> key_pair;
  key_pair.first.setRand();
  key_pair.second.clear();
  bn::G2::mul(key_pair.second, generator, key_pair.first);
  return key_pair;
}

/**
 * Computes a deterministic hash to the finite prime field from one public key and the set
 * of all eligible notarisation keys
 *
 * @param notarisation_key Particular public key of a cabinet member
 * @param cabinet_notarisation_keys Public keys of all cabinet members
 * @return Element of prime field
 */
bn::Fr SignatureAggregationCoefficient(PublicKey const &             notarisation_key,
                                       std::vector<PublicKey> const &cabinet_notarisation_keys)
{
  bn::Fr coefficient;
  coefficient.clear();

  // Reserve first 48 bytes for some fixed value for hygenic reuse of the
  // hashing function
  std::string concatenated_keys = "BLS Aggregation";
  concatenated_keys.reserve(cabinet_notarisation_keys.size() * 310);
  while (concatenated_keys.length() < 48)
  {
    concatenated_keys.push_back('0');
  }

  concatenated_keys += notarisation_key.getStr();
  for (auto const &key : cabinet_notarisation_keys)
  {
    concatenated_keys += key.getStr();
  }
  coefficient.setHashOf(concatenated_keys);
  return coefficient;
}

/**
 * Computes aggregrate signature from signatures of a message
 *
 * @param signatures Map of the signer index and their signature of a message
 * @param public_keys Public keys of all eligible signers
 * @return Pair consisting of aggregate signature and a vector indicating who's signatures were
 * aggregated
 */
AggregateSignature ComputeAggregateSignature(
    std::unordered_map<uint32_t, Signature> const &signatures,
    std::vector<PublicKey> const &                 public_keys)
{
  Signature aggregate_signature;
  aggregate_signature.clear();
  std::vector<bool> signers;
  signers.resize(public_keys.size(), false);

  // Compute signature
  for (auto const &sig : signatures)
  {
    uint32_t  index = sig.first;
    Signature modified_sig;
    modified_sig.clear();
    bn::Fr aggregate_coefficient = SignatureAggregationCoefficient(public_keys[index], public_keys);
    bn::G1::mul(modified_sig, sig.second, aggregate_coefficient);
    bn::G1::add(aggregate_signature, aggregate_signature, modified_sig);
    signers[index] = true;
  }
  return std::make_pair(aggregate_signature, signers);
}

/**
 * Computes the aggregated public key from a set of parties who signed a particular message
 *
 * @param signers Vector of booleans indicated whether this member participated in the aggregate
 * signature
 * @param cabinet_public_keys Public keys of all eligible signers
 * @return Aggregated public key
 */
PublicKey ComputeAggregatePublicKey(std::vector<bool> const &     signers,
                                    std::vector<PublicKey> const &cabinet_public_keys)
{
  PublicKey aggregate_key;
  aggregate_key.clear();
  assert(signers.size() == cabinet_public_keys.size());
  for (size_t i = 0; i < cabinet_public_keys.size(); ++i)
  {
    if (signers[i])
    {
      // Compute public_key_i ^ coefficient_i
      PublicKey modified_public_key;
      modified_public_key.clear();
      bn::Fr aggregate_coefficient =
          SignatureAggregationCoefficient(cabinet_public_keys[i], cabinet_public_keys);
      bn::G2::mul(modified_public_key, cabinet_public_keys[i], aggregate_coefficient);

      bn::G2::add(aggregate_key, aggregate_key, modified_public_key);
    }
  }
  return aggregate_key;
}

/**
 * Verifies an aggregate signature
 *
 * @param message Message that was signed
 * @param aggregate_signature Pair of signature and vector of booleans indicating who participated
 * in the aggregate signature
 * @param cabinet_public_keys Public keys of all eligible signers
 * @param generator Generator of elliptic curve
 * @return Bool for whether the signature passed verification
 */
bool VerifyAggregateSignature(MessagePayload const &        message,
                              AggregateSignature const &    aggregate_signature,
                              std::vector<PublicKey> const &cabinet_public_keys,
                              Generator const &             generator)
{
  bn::Fp12 e1, e2;

  // hash and map message to point on curve
  bn::Fp Hm;
  bn::G1 PH;
  Hm.setHashOf(message.pointer(), message.size());
  bn::mapToG1(PH, Hm);

  // Compute aggregate  public key
  if (aggregate_signature.second.size() != cabinet_public_keys.size())
  {
    return false;
  }
  PublicKey aggregate_key =
      ComputeAggregatePublicKey(aggregate_signature.second, cabinet_public_keys);

  bn::pairing(e1, aggregate_signature.first, generator);
  bn::pairing(e2, PH, aggregate_key);

  return e1 == e2;
}

}  // namespace mcl
}  // namespace crypto
}  // namespace fetch
