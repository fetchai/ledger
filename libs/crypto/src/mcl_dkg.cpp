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

#include <mcl/bn256.hpp>

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

namespace bn = mcl::bn256;

namespace fetch {
namespace dkg {

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
 * Computation of the a polynomial (whose coefficients are unknown) evaluated at 0
 *
 * @param parties Set of points (not equal to 0) at which the polynomial has been evaluated
 * @param shares The value of polynomial at the points parties
 * @return The value of the polynomial evaluated at 0 (z_i)
 */
bn::Fr ComputeZi(std::set<uint32_t> const &parties, std::vector<bn::Fr> const &shares)
{
  // compute $z_i$ using Lagrange interpolation (without corrupted parties)
  bn::Fr z{0};
  for (auto jt : parties)
  {
    // compute optimized Lagrange coefficients
    bn::Fr rhsF{1}, lhsF{1}, tmpF;
    for (auto lt : parties)
    {
      if (lt != jt)
      {
        bn::Fr::mul(rhsF, rhsF, lt + 1);  // adjust index in computation
      }
    }
    for (auto lt : parties)
    {
      if (lt != jt)
      {
        tmpF = (lt + 1);
        bn::Fr::sub(tmpF, tmpF, (jt + 1));
        bn::Fr::mul(lhsF, lhsF, tmpF);
      }
    }
    bn::Fr::inv(lhsF, lhsF);

    bn::Fr::mul(rhsF, rhsF, lhsF);
    bn::Fr::mul(tmpF, rhsF, shares[jt]);  // use the provided shares (interpolation points)
    bn::Fr::add(z, z, tmpF);
  }
  return z;
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
  size_t m = a.size();
  if ((b.size() != m) || (m == 0))
  {
    throw std::invalid_argument("mcl_interpolate_polynom: bad m");
  }
  std::vector<bn::Fr> prod{a}, res(m, 0);
  bn::Fr              t1, t2;
  for (size_t k = 0; k < m; k++)
  {
    t1 = 1;
    for (long i = static_cast<long>(k - 1); i >= 0; i--)
    {
      bn::Fr::mul(t1, t1, a[k]);
      bn::Fr::add(t1, t1, prod[static_cast<size_t>(i)]);
    }

    t2 = 0;
    for (long i = static_cast<long>(k - 1); i >= 0; i--)
    {
      bn::Fr::mul(t2, t2, a[k]);
      bn::Fr::add(t2, t2, res[static_cast<size_t>(i)]);
    }
    bn::Fr::inv(t1, t1);

    bn::Fr::sub(t2, b[k], t2);
    bn::Fr::mul(t1, t1, t2);

    for (size_t i = 0; i < k; i++)
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
        for (long i = static_cast<long>(k - 1); i >= 1; i--)
        {
          bn::Fr::mul(t2, prod[static_cast<size_t>(i)], t1);
          bn::Fr::add(prod[static_cast<size_t>(i)], t2, prod[static_cast<size_t>(i - 1)]);
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
bn::G1 SignShare(byte_array::ConstByteArray const &message, bn::Fr const &x_i)
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
 * @param G Group used in DKG
 * @return
 */
bool VerifySign(bn::G2 const &y, byte_array::ConstByteArray const &message, bn::G1 const &sign,
                bn::G2 const &G)
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
bn::G1 LagrangeInterpolation(std::unordered_map<uint32_t, bn::G1> const &shares)
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

}  // namespace dkg
}  // namespace fetch
