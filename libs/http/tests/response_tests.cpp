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

#include "core/byte_array/const_byte_array.hpp"
#include "http/response.hpp"
#include "network/fetch_asio.hpp"

#include <gtest/gtest.h>
#include <memory>

class ResponseTests : public ::testing::Test
{
protected:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Response       = fetch::http::HTTPResponse;
  using ResponsePtr    = std::unique_ptr<Response>;

  void SetUp() override
  {
    response_ = std::make_unique<Response>();
  }

  void ConvertToBuffer(char const *text, asio::streambuf &buffer)
  {
    std::ostream stream(&buffer);
    stream << text;
  }

  void VerifyHeaderValue(ConstByteArray key, ConstByteArray value)
  {
    ASSERT_TRUE(response_->header().Has(key));
    EXPECT_EQ(response_->header()[key], value);
  }

  ResponsePtr response_;
};

TEST_F(ResponseTests, HeaderCase1)
{
  static const char *raw_header =
      "HTTP/1.0 404 NOT FOUND\r\n"
      "Content-Type : text/html\r\n"
      "Content-Length: 233\r\n"
      "Server : Werkzeug/0.14.1 Python/3.6.5\r\n"
      "Date: Sat, 11 Aug 2018 09:55:11 GMT\r\n"
      "\r\n";

  // convert the raw request into a network buffer like object
  asio::streambuf buffer;
  ConvertToBuffer(raw_header, buffer);

  ASSERT_TRUE(response_->ParseHeader(buffer, buffer.size()));
  ASSERT_EQ(response_->status(), fetch::http::Status::CLIENT_ERROR_NOT_FOUND);

  VerifyHeaderValue("content-type", "text/html");
  VerifyHeaderValue("content-length", "233");
  VerifyHeaderValue("server", "Werkzeug/0.14.1 Python/3.6.5");
  VerifyHeaderValue("date", "Sat, 11 Aug 2018 09:55:11 GMT");
}

TEST_F(ResponseTests, HeaderCase2)
{
  static const char *raw_header =
      "HTTP/1.0 200 NOT FOUND\r\n"
      "Content-Type : application/json\r\n"
      "Content-Length: 10\r\n"
      "\r\n";

  // convert the raw request into a network buffer like object
  asio::streambuf buffer;
  ConvertToBuffer(raw_header, buffer);

  ASSERT_TRUE(response_->ParseHeader(buffer, buffer.size()));
  ASSERT_EQ(response_->status(), fetch::http::Status::SUCCESS_OK);

  VerifyHeaderValue("content-type", "application/json");
  VerifyHeaderValue("content-length", "10");
}
