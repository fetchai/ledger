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

#include "chain/transaction.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/identifier.hpp"
#include "ledger/transaction_validator.hpp"

namespace fetch {
namespace ledger {
namespace {

bool IsCreateWealth(chain::Transaction const &tx)
{
  return (tx.contract_mode() == chain::Transaction::ContractMode::CHAIN_CODE) &&
         (tx.chain_code() == "fetch.token") && (tx.action() == "wealth");
}

}  // namespace

TransactionValidator::TransactionValidator(StorageInterface &storage, TokenContract &token_contract)
  : storage_{storage}
  , token_contract_{token_contract}
{}

/**
 * Validate if a transaction can be included at the specified block index.
 *
 * This will also perform deed validation on if applicable
 *
 * @param tx The target transaciotn
 * @param block_index The designated block index
 * @return SUCCESS if successful, otherwise a corresponding error code
 */
ContractExecutionStatus TransactionValidator::operator()(chain::Transaction const &tx,
                                                         uint64_t block_index) const
{
  // CHECK: Determine if the transaction is valid for the given block
  auto const tx_validity = tx.GetValidity(block_index);
  if (chain::Transaction::Validity::VALID != tx_validity)
  {
    return ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK;
  }

  // SHORT TERM EXEMPTION - While no state file exists (and the wealth endpoint is still present)
  // this and only this contract is exempt from the pre-validation checks
  if (IsCreateWealth(tx))
  {
    return ContractExecutionStatus::SUCCESS;
  }

  // attach the token contract to the storage engine
  StateAdapter storage_adapter{storage_, Identifier{"fetch.token"}};

  {
    ContractContext ctx{&token_contract_, tx.contract_address(), &storage_adapter, block_index};
    ContractContextAttacher attacher{token_contract_, ctx};

    // CHECK: Ensure there is permission from the originating address to perform the transaction
    //        (essentially take fees)
    auto const deed = token_contract_.GetDeed(tx.from());
    if (deed)
    {
      // if a deed is present then minimally the signers of the transaction need to have transfer
      // permission in order to pay for the fees
      if (!deed->Verify(tx, Deed::TRANSFER))
      {
        return ContractExecutionStatus::TX_PERMISSION_DENIED;
      }

      // additionally if a smart contract is present in the transaction we also need the execute
      // permission
      if (chain::Transaction::ContractMode::PRESENT == tx.contract_mode())
      {
        if (!deed->Verify(tx, Deed::EXECUTE))
        {
          return ContractExecutionStatus::TX_PERMISSION_DENIED;
        }
      }
    }
    else
    {
      // CHECK: In the case where there is no deed present. Then there should only be one signature
      //        present in the transaction and it must match the from address
      if (!((tx.signatories().size() == 1) && tx.IsSignedByFromAddress()))
      {
        return ContractExecutionStatus::TX_PERMISSION_DENIED;
      }
    }

    // CHECK: Ensure that the originator has funds available to make both all the transfers in the
    //        contract as well as the maximum fees
    uint64_t const min_charge =
        tx.transfers().size() +
        ((tx.contract_mode() == chain::Transaction::ContractMode::NOT_PRESENT) ? 0 : 1);
    if ((tx.charge_limit() < min_charge) || (tx.charge_rate() == 0))
    {
      return ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE;
    }

    // CHECK: Ensure that the originator has funds available to make both all the transfers in the
    //        contract as well as the maximum fees
    uint64_t const balance    = token_contract_.GetBalance(tx.from());
    uint64_t const max_charge = tx.charge_rate() * tx.charge_limit();
    if (balance < max_charge)
    {
      return ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS;
    }
  }

  return ContractExecutionStatus::SUCCESS;
}

}  // namespace ledger
}  // namespace fetch
