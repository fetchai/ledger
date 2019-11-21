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

#include "crypto/sha256.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace fetch {
namespace ledger {
namespace consensus {

ProofOfWork::ProofOfWork(HeaderType header)
  : header_{std::move(header)}
{}

bool ProofOfWork::operator()()
{
  crypto::SHA256 hasher;
  hasher.Reset();
  hasher.Update(header_);
  hasher.Update(this->pointer(), this->size());

  // Forcing little endian representation of the hash (even if it is actually
  // represented in big endian encoding) in order to keep compatibility with
  // proofs recorded in previously generated blocks.
  digest_.FromArray(hasher.Final(), true);

  hasher.Reset();
  hasher.Update(digest_.pointer(), digest_.size());

  digest_.FromArray(hasher.Final(), true);

  return digest_ < target_;
}

void ProofOfWork::SetTarget(std::size_t zeros)
{
  target_ = 1;
  target_ <<= 8 * sizeof(uint8_t) * UInt256::size() - 1 - zeros;
}

void ProofOfWork::SetTarget(vectorise::UInt<256> &&target)
{
  target_ = target;
}

void ProofOfWork::SetHeader(byte_array::ByteArray const &header)
{
  header_ = header;
  assert(header_ == header);
}

byte_array::ConstByteArray const &ProofOfWork::header() const
{
  return header_;
}

vectorise::UInt<256> const &ProofOfWork::digest() const
{
  return digest_;
}

vectorise::UInt<256> const &ProofOfWork::target() const
{
  return target_;
}

}  // namespace consensus
}  // namespace ledger
}  // namespace fetch
