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

#include <cstddef>

namespace fetch {
namespace http {

enum class Status
{
  UNKNOWN = 0,

  INFORMATION_CONTINUE            = 100,
  INFORMATION_SWITCHING_PROTOCOLS = 101,
  INFORMATION_PROCESSING          = 102,

  SUCCESS_OK                            = 200,
  SUCCESS_CREATED                       = 201,
  SUCCESS_ACCEPTED                      = 202,
  SUCCESS_NON_AUTHORITATIVE_INFORMATION = 203,
  SUCCESS_NO_CONTENT                    = 204,
  SUCCESS_RESET_CONTENT                 = 205,
  SUCCESS_PARTIAL_CONTENT               = 206,
  SUCCESS_MULTI_STATUS                  = 207,
  SUCCESS_ALREADY_REPORTED              = 208,
  SUCCESS_IM_USED                       = 226,

  REDIRECTION_MULTIPLE_CHOICES   = 300,
  REDIRECTION_MOVED_PERMANENTLY  = 301,
  REDIRECTION_FOUND              = 302,
  REDIRECTION_SEE_OTHER          = 303,
  REDIRECTION_NOT_MODIFIED       = 304,
  REDIRECTION_USE_PROXY          = 305,
  REDIRECTION_SWITCH_PROXY       = 306,
  REDIRECTION_TEMPORARY_REDIRECT = 307,
  REDIRECTION_PERMANENT_REDIRECT = 308,

  CLIENT_ERROR_BAD_REQUEST                     = 400,
  CLIENT_ERROR_UNAUTHORIZED                    = 401,
  CLIENT_ERROR_PAYMENT_REQUIRED                = 402,
  CLIENT_ERROR_FORBIDDEN                       = 403,
  CLIENT_ERROR_NOT_FOUND                       = 404,
  CLIENT_ERROR_METHOD_NOT_ALLOWED              = 405,
  CLIENT_ERROR_NOT_ACCEPTABLE                  = 406,
  CLIENT_ERROR_PROXY_AUTHENTICATION_REQUIRED   = 407,
  CLIENT_ERROR_REQUEST_TIMEOUT                 = 408,
  CLIENT_ERROR_CONFLICT                        = 409,
  CLIENT_ERROR_LENGTH_REQUIRED                 = 411,
  CLIENT_ERROR_PRECONDITION_FAILED             = 412,
  CLIENT_ERROR_PAYLOAD_TOO_LARGE               = 413,
  CLIENT_ERROR_URI_TOO_LONG                    = 414,
  CLIENT_ERROR_UNSUPPORTED_MEDIA_TYPE          = 415,
  CLIENT_ERROR_RANGE_NOT_SATISFIABLE           = 416,
  CLIENT_ERROR_EXPECTATION_FAILED              = 417,
  CLIENT_ERROR_IM_A_TEAPOT                     = 418,
  CLIENT_ERROR_MISDIRECTION_REQUIRED           = 421,
  CLIENT_ERROR_UNPROCESSABLE_ENTITY            = 422,
  CLIENT_ERROR_LOCKED                          = 423,
  CLIENT_ERROR_FAILED_DEPENDENCY               = 424,
  CLIENT_ERROR_UPGRADE_REQUIRED                = 426,
  CLIENT_ERROR_PRECONDITION_REQUIRED           = 428,
  CLIENT_ERROR_TOO_MANY_REQUESTS               = 429,
  CLIENT_ERROR_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  CLIENT_ERROR_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,

  SERVER_ERROR_INTERNAL_SERVER_ERROR           = 500,
  SERVER_ERROR_NOT_IMPLEMENTED                 = 501,
  SERVER_ERROR_BAD_GATEWAY                     = 502,
  SERVER_ERROR_SERVICE_UNAVAILABLE             = 503,
  SERVER_ERROR_GATEWAY_TIMEOUT                 = 504,
  SERVER_ERROR_HTTP_VERSION_NOT_SUPPORTED      = 505,
  SERVER_ERROR_VARIANT_ALSO_NEGOTIATES         = 506,
  SERVER_ERROR_INSUFFICIENT_STORAGE            = 507,
  SERVER_ERROR_LOOP_DETECTED                   = 508,
  SERVER_ERROR_NOT_EXTENDED                    = 510,
  SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED = 511,
};

Status      FromCode(std::size_t code);
char const *ToString(Status status);

}  // namespace http
}  // namespace fetch
