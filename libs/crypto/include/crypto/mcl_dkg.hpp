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
#include <sstream>
#include <unordered_map>

namespace bn = mcl::bn256;

namespace fetch {
namespace dkg {

bn::G2 ComputeLHS(bn::G2 &tmpG, bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1,
                  bn::Fr const &share2);

bn::G2 ComputeLHS(bn::G2 const &G, bn::G2 const &H, bn::Fr const &share1, bn::Fr const &share2);

void UpdateRHS(uint32_t rank, bn::G2 &rhsG, std::vector<bn::G2> const &input);

bn::G2 ComputeRHS(uint32_t rank, std::vector<bn::G2> const &input);

void ComputeShares(bn::Fr &s_i, bn::Fr &sprime_i, std::vector<bn::Fr> const &a_i,
                   std::vector<bn::Fr> const &b_i, uint32_t rank);

bn::Fr ComputeZi(std::vector<uint32_t> const &parties, std::vector<bn::Fr> const &shares);

std::vector<bn::Fr> InterpolatePolynom(std::vector<bn::Fr> const &a, std::vector<bn::Fr> const &b);

bn::G1 SignShare(byte_array::ConstByteArray const &message, bn::Fr const &x_i);

bool VerifyShare(bn::G2 const &v_i, byte_array::ConstByteArray const &message, const bn::G1 &sign,
                 bn::G2 const &G);

bool VerifySign(bn::G2 const &y, byte_array::ConstByteArray const &message, bn::G1 const &sign,
                bn::G2 const &G);

bn::G1 LagrangeInterpolation(std::unordered_map<uint32_t, bn::G1> const &shares);

bool MyLagrangeInterpolation(bn::G1 &out, const std::vector<bn::Fr> &S,
                             const std::vector<bn::G1> &vec, size_t k);
}  // namespace dkg
}  // namespace fetch
