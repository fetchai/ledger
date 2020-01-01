#pragma once
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

#include "vm/array.hpp"
#include "vm_modules/ledger/forward_decl.hpp"
#include "vm_modules/ledger/transfer.hpp"
#include "vm_modules/math/bignumber.hpp"

namespace fetch {

namespace chain {
class Transaction;
class Address;
}  // namespace chain

namespace vm_modules {
namespace ledger {

class Transaction : public fetch::vm::Object
{

public:
  Transaction(vm::VM *vm, vm::TypeId type_id, fetch::chain::Transaction const &tx);

  static void Bind(vm::Module &module);

  /// @name Identification
  /// @{
  DigestPtr digest() const;
  /// @}

  /// @name Transfer Accessors
  /// @{
  AddressPtr        from() const;
  TransfersPtr      transfers() const;
  NativeTokenAmount get_total_transfer_amount() const;
  /// @}

  /// @name Validity Accessors
  /// @{
  BlockIndex valid_from() const;
  BlockIndex valid_until() const;
  /// @}

  /// @name Charge Accessors
  /// @{
  NativeTokenAmount charge_rate() const;
  NativeTokenAmount charge_limit() const;
  /// @}

  /// @name Contract Accessors
  /// @{
  AddressPtr   contract_address() const;
  StringPtr    action() const;
  AddressesPtr signatories() const;

private:
  std::shared_ptr<fetch::chain::Transaction> tx_;
  DigestPtr                                  digest_;
  AddressPtr                                 from_;
  TransfersPtr                               transfers_;
  AddressPtr                                 contract_address_;
  StringPtr                                  action_;
  AddressesPtr                               signatories_;
};

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
