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

#include "network/service/types.hpp"

namespace fetch {
namespace service {

ServiceClassificationType const SERVICE_FUNCTION_CALL = 0ull;
ServiceClassificationType const SERVICE_RESULT        = 10ull;
ServiceClassificationType const SERVICE_SUBSCRIBE     = 20ull;
ServiceClassificationType const SERVICE_UNSUBSCRIBE   = 30ull;
ServiceClassificationType const SERVICE_FEED          = 40ull;

ServiceClassificationType const SERVICE_ERROR = 999ull;
}  // namespace service
}  // namespace fetch
