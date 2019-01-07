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

#include "core/serializers/exception.hpp"

namespace fetch {
namespace service {
namespace error {

using error_type = serializers::error::error_type;

error_type const ERROR_SERVICE_PROTOCOL = 1 << 16;  // TODO(issue 11): move to global place
error_type const USER_ERROR             = 0 | ERROR_SERVICE_PROTOCOL;
error_type const MEMBER_NOT_FOUND       = 1 | ERROR_SERVICE_PROTOCOL;
error_type const MEMBER_EXISTS          = 2 | ERROR_SERVICE_PROTOCOL;
error_type const PROTOCOL_NOT_FOUND     = 11 | ERROR_SERVICE_PROTOCOL;
error_type const PROTOCOL_EXISTS        = 12 | ERROR_SERVICE_PROTOCOL;
error_type const PROMISE_NOT_FOUND      = 21 | ERROR_SERVICE_PROTOCOL;
error_type const COULD_NOT_DELIVER      = 31 | ERROR_SERVICE_PROTOCOL;
error_type const UNKNOWN_MESSAGE        = 1001 | ERROR_SERVICE_PROTOCOL;

error_type const PROTOCOL_RANGE = 13 | ERROR_SERVICE_PROTOCOL;

}  // namespace error
}  // namespace service
}  // namespace fetch
