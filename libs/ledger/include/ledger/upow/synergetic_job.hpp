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

#include "chain/address.hpp"

namespace fetch {
namespace ledger {

class SynergeticJob
{
public:
  using Address = chain::Address;
  SynergeticJob() = default;
  virtual ~SynergeticJob() = default;

  void set_id(uint64_t const &id);
  void set_epoch(uint64_t const &epoch);
  void set_problem_charge(uint64_t const &charge);
  void set_work_charge(uint64_t const &charge);
  void set_clear_charge(uint64_t const &charge);
  void set_contract_address(Address address);

  uint64_t const &id() const;
  uint64_t const &epoch() const;
  uint64_t const &problem_charge() const;
  uint64_t const &work_charge() const;
  uint64_t const &clear_charge() const;
  uint64_t const &total_charge() const;
  Address  const &contract_address() const;


private:
  uint64_t   id_{0};
  Address    contract_address_;
  uint64_t   epoch_{0};
  uint64_t   problem_charge_{0};
  uint64_t   work_charge_{0};
  uint64_t   clear_charge_{0};

};

}  // namespace ledger
}  // namespace fetch
