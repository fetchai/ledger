#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fetch_mcl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_map>

namespace bn = mcl::bn256;

namespace fetch {
namespace crypto {
namespace mcl {

using PrivateKey     = bn::Fr;
using PublicKey      = bn::G2;
using Signature      = bn::G1;
using Group          = bn::G2;
using MessagePayload = byte_array::ConstByteArray;
using CabinetIndex   = uint32_t;

/**
 * Helper functions for computations used in the DKG
 */
bn::G2 ComputeLHS(bn::G2 &tmpG, bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1,
                  bn::Fr const &share2);

bn::G2 ComputeLHS(bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1, bn::Fr const &share2);

void UpdateRHS(uint32_t rank, bn::G2 &rhsG, std::vector<bn::G2> const &input);

bn::G2 ComputeRHS(uint32_t rank, std::vector<bn::G2> const &input);

void ComputeShares(bn::Fr &s_i, bn::Fr &sprime_i, std::vector<bn::Fr> const &a_i,
                   std::vector<bn::Fr> const &b_i, uint32_t rank);

bn::Fr ComputeZi(std::set<uint32_t> const &parties, std::vector<bn::Fr> const &shares);

std::vector<bn::Fr> InterpolatePolynom(std::vector<bn::Fr> const &a, std::vector<bn::Fr> const &b);

// For signatures
Signature SignShare(MessagePayload const &message, PrivateKey const &x_i);

bool VerifySign(PublicKey const &y, MessagePayload const &message, Signature const &sign,
                Group const &G);

Signature LagrangeInterpolation(std::unordered_map<CabinetIndex, Signature> const &shares);

/**
 * Vector initialisation for mcl data structures
 *
 * @tparam T Type in vector
 * @param data Vector to be initialised
 * @param i Number of columns
 */
template <typename T>
void Init(std::vector<T> &data, uint32_t i)
{
  data.resize(i);
  for (auto &data_i : data)
  {
    data_i.clear();
  }
}

/**
 * Matrix initialisation for mcl data structures
 *
 * @tparam T Type in matrix
 * @param data Matrix to be initialised
 * @param i Number of rows
 * @param j Number of columns
 */
template <typename T>
void Init(std::vector<std::vector<T>> &data, uint32_t i, uint32_t j)
{
  data.resize(i);
  for (auto &data_i : data)
  {
    data_i.resize(j);
    for (auto &data_ij : data_i)
    {
      data_ij.clear();
    }
  }
}

}  // namespace mcl
}  // namespace crypto
}  // namespace fetch
