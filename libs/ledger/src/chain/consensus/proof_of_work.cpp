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

#include "ledger/chain/consensus/proof_of_work.hpp"

#include "crypto/sha256.hpp"

namespace fetch {
namespace ledger {
namespace consensus {

ProofOfWork::ProofOfWork(header_type header)
  : header_{std::move(header)}
{}

bool ProofOfWork::operator()()
{
  crypto::SHA256 hasher;
  hasher.Reset();
  hasher.Update(header_);
  hasher.Update(this->pointer(), this->size());

  digest_ = hasher.Final();

  hasher.Reset();
  hasher.Update(digest_.pointer(), digest_.size());

  digest_ = hasher.Final();

  return digest_ < target_;
}

void ProofOfWork::SetTarget(std::size_t zeros)
{
  target_ = 1;
  target_ <<= 8 * sizeof(uint8_t) * UInt256::size() - 1 - zeros;
}

void ProofOfWork::SetTarget(vectorise::UInt<256> &&target)
{
  target_ = std::move(target);
}

void ProofOfWork::SetHeader(byte_array::ByteArray header)
{
  header_ = header;
  assert(header_ == header);
}

}  // namespace consensus
}  // namespace ledger
}  // namespace fetch
