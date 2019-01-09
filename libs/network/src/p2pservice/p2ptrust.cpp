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

#include "network/p2pservice/p2ptrust.hpp"

#include <cmath>

namespace fetch {
namespace p2p {
const trust_modifiers_type trust_modifiers_ = {{
    /*                       LIED,          BAD_CONNECTION,      DUPLICATE,      NEW_INFORMATION  */
    /* BLOCK       */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {3.0, NAN, 15.0}}},
    /* TRANSACTION */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {3.0, NAN, 15.0}}},
    /* PEER        */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {20.0, NAN, 100.0}}},
}};
}
}  // namespace fetch
