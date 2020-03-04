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

#include <cstdint>

namespace fetch {
namespace ledger {

class ChargeConfiguration
{
public:
  class Builder
  {
  public:
    Builder()                = default;
    ~Builder()               = default;
    Builder(Builder const &) = delete;
    Builder(Builder &&)      = delete;
    Builder &operator=(Builder const &) = delete;
    Builder &operator=(Builder &&) = delete;

    Builder &SetVmChargeMultiplier(uint64_t multiplier);

    ChargeConfiguration Build() const;

  private:
    uint64_t vm_charge_multiplier_{};
  };

  ChargeConfiguration() = default;

  uint64_t charge_multiplier{};

private:
  explicit ChargeConfiguration(uint64_t vm__charge_multiplier);

  friend class Builder;
};

}  // namespace ledger
}  // namespace fetch
