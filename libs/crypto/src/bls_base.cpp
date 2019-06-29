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

#include "crypto/bls_base.hpp"

#include <atomic>

namespace fetch {
namespace crypto {
namespace bls {
namespace {

std::atomic<bool> g_initialised{false};

} // namespace

/**
 * Initialise the BLS library
 */
void Init()
{
  // determine if the library was previously initialised
  bool const was_previously_initialised = g_initialised.exchange(true);

  if (!was_previously_initialised)
  {
    if (blsInit(E_MCLBN_CURVE_FP254BNB, MCLBN_COMPILED_TIME_VAR) != 0)
    {
      throw std::runtime_error("unable to initalize BLS.");
    }
  }
}

} // namespace bls
} // namespace crypto
} // namespace fetch
