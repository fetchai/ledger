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

/**
 * Chargeable is an abstract interface that exposes fees incurred during transaction, contract, etc.
 * execution.
 */
class Chargeable
{
public:
  // Construction / Destruction
  Chargeable()          = default;
  virtual ~Chargeable() = default;

  /**
   * Calculate the fee as a result of the execution calls on this object.
   *
   * @return The fee
   */
  virtual uint64_t CalculateFee() const = 0;
};

}  // namespace ledger
}  // namespace fetch
