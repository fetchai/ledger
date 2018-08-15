#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

namespace fetch {
namespace service {

using serializer_type = serializers::TypedByteArrayBuffer;

// using serializer_type = serializers::ByteArrayBuffer;

using protocol_handler_type       = uint64_t;
using function_handler_type       = uint64_t;
using feed_handler_type           = uint8_t;
using subscription_handler_type   = uint8_t;
using service_classification_type = uint64_t;
}  // namespace service
}  // namespace fetch
