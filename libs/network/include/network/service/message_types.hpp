#pragma once
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

#include "network/service/types.hpp"

namespace fetch {
namespace service {

service_classification_type const SERVICE_FUNCTION_CALL = 0ull;
service_classification_type const SERVICE_RESULT        = 10ull;
service_classification_type const SERVICE_SUBSCRIBE     = 20ull;
service_classification_type const SERVICE_UNSUBSCRIBE   = 30ull;
service_classification_type const SERVICE_FEED          = 40ull;

service_classification_type const SERVICE_ERROR = 999ull;
}  // namespace service
}  // namespace fetch
