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

#include "ledger/fees/storage_fee.hpp"
#include "ledger/state_sentinel_adapter.hpp"

namespace fetch {
namespace ledger {

StorageFee::StorageFee(StateSentinelAdapter &storage_adapter)
  : storage_adapter_{storage_adapter}
{}

uint64_t StorageFee::CalculateFee() const
{
  return storage_adapter_.num_bytes_written() * 2u;
}

}  // namespace ledger
}  // namespace fetch
