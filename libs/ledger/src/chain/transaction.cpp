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

#include "ledger/chain/transaction.hpp"

#include "crypto/verifier.hpp"
#include "ledger/chain/transaction_serializer.hpp"

namespace fetch {
namespace ledger {

/**
 * Compute the total amount being transfered in this transaction
 *
 * @return The total amount being transfered
 */
uint64_t Transaction::GetTotalTransferAmount() const
{
  uint64_t amount{0};

  for (auto const &transfer : transfers_)
  {
    amount += transfer.amount;
  }

  return amount;
}

/**
 * Verify the contents of the transaction
 *
 * @return
 */
bool Transaction::Verify()
{
  if (!verification_completed_)
  {
    // clear the verified flag
    verified_ = false;

    // generate the payload for this transaction
    ConstByteArray payload = TransactionSerializer::SerializePayload(*this);

    // ensure that there are some signatories (otherwise it is invalid)
    if (!signatories_.empty())
    {
      // loop through all the signatories
      bool all_verified{true};
      for (auto const &signatory : signatories_)
      {
        // verify the signature
        if (!crypto::Verifier::Verify(signatory.identity, payload, signatory.signature))
        {
          // exit as soon as the first non valid signature is detected
          all_verified = false;
          break;
        }

        // ensure is well formed
        assert(!signatory.address.address().empty());
      }

      // only valid when there are more that 1 signature present and that they matched the computed
      // payload.
      verified_ = all_verified;
    }

    // signal that the verification has been completed
    verification_completed_ = true;
  }

  return verified_;
}

bool Transaction::IsSignedByFromAddress() const
{
  auto const it = std::find_if(
      signatories_.begin(), signatories_.end(),
      [this](Transaction::Signatory const &signatory) { return signatory.address == from_; });

  return it != signatories_.end();
}

}  // namespace ledger
}  // namespace fetch
