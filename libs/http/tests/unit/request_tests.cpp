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

#include "http/request.hpp"

#include "gmock/gmock.h"

namespace {

using namespace ::testing;

class RequestTests : public Test
{
public:
  using Request = fetch::http::HTTPRequest;
};

TEST_F(RequestTests, no_error_when_reading_zero_bytes_from_empty_buffer)
{
  auto BYTES_REQUESTED = 0u;

  asio::streambuf buffer;
  Request         req;

  ASSERT_NO_THROW(req.ParseHeader(buffer, BYTES_REQUESTED));
}

}  // namespace
