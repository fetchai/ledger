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

#include "chain/transaction.hpp"
#include "vm/module.hpp"
#include "vm_modules/ledger/transaction.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

using namespace fetch::vm;

namespace {

AddressPtr CreateAddress(VM *vm, fetch::chain::Address const &address,
                         fetch::chain::Transaction::Signatories const &signatories)
{
  // TODO(pb): Performance - `std::vector<...>` in not suited for effective search (signatories are
  // returned as `std::vector<Signatory>` type.
  auto const has_signed{signatories.cend() != std::find_if(signatories.cbegin(), signatories.cend(),
                                                           [&address](auto const &signatory) {
                                                             return signatory.address == address;
                                                           })};
  return vm->CreateNewObject<Address>(address, has_signed);
}

AddressesPtr CreateSignatories(VM *vm, fetch::chain::Transaction const &tx)
{
  auto const size{tx.signatories().size()};
  auto       signatories{vm->CreateNewObject<vm::Array<AddressPtr>>(vm->GetTypeId<Address>(),
                                                              static_cast<int32_t>(size))};

  for (std::size_t i{0}; i < size; ++i)
  {
    signatories->elements[i] = vm->CreateNewObject<Address>(tx.signatories()[i].address, true);
  }
  return signatories;
}

TransfersPtr CreateTransfers(VM *vm, fetch::chain::Transaction const &tx)
{
  auto const &transfers{tx.transfers()};
  auto const  size{tx.transfers().size()};
  auto        vm_transfers{vm->CreateNewObject<vm::Array<TransferPtr>>(vm->GetTypeId<Transfer>(),
                                                                static_cast<int32_t>(size))};

  for (std::size_t i{0}; i < size; ++i)
  {
    auto const &t{transfers[i]};
    vm_transfers->elements[i] =
        vm->CreateNewObject<Transfer>(CreateAddress(vm, t.to, tx.signatories()), t.amount);
  }

  return vm_transfers;
}

}  // namespace

void Transaction::Bind(vm::Module &module)
{
  module.CreateClassType<Transaction>("Transaction")
      .CreateMemberFunction("digest", &Transaction::digest)
      .CreateMemberFunction("from", &Transaction::from)
      .CreateMemberFunction("transfers", &Transaction::transfers)
      .CreateMemberFunction("getTotalTransferAmount", &Transaction::get_total_transfer_amount)
      .CreateMemberFunction("validFrom", &Transaction::valid_from)
      .CreateMemberFunction("validUntil", &Transaction::valid_until)
      .CreateMemberFunction("chargeRate", &Transaction::charge_rate)
      .CreateMemberFunction("chargeLimit", &Transaction::charge_limit)
      .CreateMemberFunction("contractAddress", &Transaction::contract_address)
      .CreateMemberFunction("action", &Transaction::action)
      .CreateMemberFunction("signatories", &Transaction::signatories);
}

Transaction::Transaction(vm::VM *vm, vm::TypeId type_id, fetch::chain::Transaction const &tx)
  : Object{vm, type_id}
  , tx_{std::make_shared<fetch::chain::Transaction>(tx)}
  , digest_{vm->CreateNewObject<math::UInt256Wrapper>(tx.digest(), platform::Endian::BIG)}
  , from_{CreateAddress(vm, tx.from(), tx.signatories())}
  , transfers_{CreateTransfers(vm, tx)}
  , contract_address_{CreateAddress(vm, tx.contract_address(), tx.signatories())}
  , action_{new String{vm, static_cast<std::string>(tx.action())}}
  , signatories_{CreateSignatories(vm, tx)}
{}

DigestPtr Transaction::digest() const
{
  return digest_;
}

AddressPtr Transaction::from() const
{
  return from_;
}

TransfersPtr Transaction::transfers() const
{
  return transfers_;
}

NativeTokenAmount Transaction::get_total_transfer_amount() const
{
  return tx_->GetTotalTransferAmount();
}

BlockIndex Transaction::valid_from() const
{
  return tx_->valid_from();
}

BlockIndex Transaction::valid_until() const
{
  return tx_->valid_until();
}

NativeTokenAmount Transaction::charge_rate() const
{
  return tx_->charge_rate();
}

NativeTokenAmount Transaction::charge_limit() const
{
  return tx_->charge_limit();
}

AddressPtr Transaction::contract_address() const
{
  return contract_address_;
}

StringPtr Transaction::action() const
{
  return action_;
}

AddressesPtr Transaction::signatories() const
{
  return signatories_;
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
