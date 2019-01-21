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

#include "http/status.hpp"

namespace fetch {
namespace http {

Status FromCode(std::size_t code)
{
  switch (code)
  {
  case 100:
    return Status::INFORMATION_CONTINUE;
  case 101:
    return Status::INFORMATION_SWITCHING_PROTOCOLS;
  case 102:
    return Status::INFORMATION_PROCESSING;
  case 200:
    return Status::SUCCESS_OK;
  case 201:
    return Status::SUCCESS_CREATED;
  case 202:
    return Status::SUCCESS_ACCEPTED;
  case 203:
    return Status::SUCCESS_NON_AUTHORITATIVE_INFORMATION;
  case 204:
    return Status::SUCCESS_NO_CONTENT;
  case 205:
    return Status::SUCCESS_RESET_CONTENT;
  case 206:
    return Status::SUCCESS_PARTIAL_CONTENT;
  case 207:
    return Status::SUCCESS_MULTI_STATUS;
  case 208:
    return Status::SUCCESS_ALREADY_REPORTED;
  case 226:
    return Status::SUCCESS_IM_USED;
  case 300:
    return Status::REDIRECTION_MULTIPLE_CHOICES;
  case 301:
    return Status::REDIRECTION_MOVED_PERMANENTLY;
  case 302:
    return Status::REDIRECTION_FOUND;
  case 303:
    return Status::REDIRECTION_SEE_OTHER;
  case 304:
    return Status::REDIRECTION_NOT_MODIFIED;
  case 305:
    return Status::REDIRECTION_USE_PROXY;
  case 306:
    return Status::REDIRECTION_SWITCH_PROXY;
  case 307:
    return Status::REDIRECTION_TEMPORARY_REDIRECT;
  case 308:
    return Status::REDIRECTION_PERMANENT_REDIRECT;
  case 400:
    return Status::CLIENT_ERROR_BAD_REQUEST;
  case 401:
    return Status::CLIENT_ERROR_UNAUTHORIZED;
  case 402:
    return Status::CLIENT_ERROR_PAYMENT_REQUIRED;
  case 403:
    return Status::CLIENT_ERROR_FORBIDDEN;
  case 404:
    return Status::CLIENT_ERROR_NOT_FOUND;
  case 405:
    return Status::CLIENT_ERROR_METHOD_NOT_ALLOWED;
  case 406:
    return Status::CLIENT_ERROR_NOT_ACCEPTABLE;
  case 407:
    return Status::CLIENT_ERROR_PROXY_AUTHENTICATION_REQUIRED;
  case 408:
    return Status::CLIENT_ERROR_REQUEST_TIMEOUT;
  case 409:
    return Status::CLIENT_ERROR_CONFLICT;
  case 411:
    return Status::CLIENT_ERROR_LENGTH_REQUIRED;
  case 412:
    return Status::CLIENT_ERROR_PRECONDITION_FAILED;
  case 413:
    return Status::CLIENT_ERROR_PAYLOAD_TOO_LARGE;
  case 414:
    return Status::CLIENT_ERROR_URI_TOO_LONG;
  case 415:
    return Status::CLIENT_ERROR_UNSUPPORTED_MEDIA_TYPE;
  case 416:
    return Status::CLIENT_ERROR_RANGE_NOT_SATISFIABLE;
  case 417:
    return Status::CLIENT_ERROR_EXPECTATION_FAILED;
  case 418:
    return Status::CLIENT_ERROR_IM_A_TEAPOT;
  case 421:
    return Status::CLIENT_ERROR_MISDIRECTION_REQUIRED;
  case 422:
    return Status::CLIENT_ERROR_UNPROCESSABLE_ENTITY;
  case 423:
    return Status::CLIENT_ERROR_LOCKED;
  case 424:
    return Status::CLIENT_ERROR_FAILED_DEPENDENCY;
  case 426:
    return Status::CLIENT_ERROR_UPGRADE_REQUIRED;
  case 428:
    return Status::CLIENT_ERROR_PRECONDITION_REQUIRED;
  case 429:
    return Status::CLIENT_ERROR_TOO_MANY_REQUESTS;
  case 431:
    return Status::CLIENT_ERROR_REQUEST_HEADER_FIELDS_TOO_LARGE;
  case 451:
    return Status::CLIENT_ERROR_UNAVAILABLE_FOR_LEGAL_REASONS;
  case 500:
    return Status::SERVER_ERROR_INTERNAL_SERVER_ERROR;
  case 501:
    return Status::SERVER_ERROR_NOT_IMPLEMENTED;
  case 502:
    return Status::SERVER_ERROR_BAD_GATEWAY;
  case 503:
    return Status::SERVER_ERROR_SERVICE_UNAVAILABLE;
  case 504:
    return Status::SERVER_ERROR_GATEWAY_TIMEOUT;
  case 505:
    return Status::SERVER_ERROR_HTTP_VERSION_NOT_SUPPORTED;
  case 506:
    return Status::SERVER_ERROR_VARIANT_ALSO_NEGOTIATES;
  case 507:
    return Status::SERVER_ERROR_INSUFFICIENT_STORAGE;
  case 508:
    return Status::SERVER_ERROR_LOOP_DETECTED;
  case 510:
    return Status::SERVER_ERROR_NOT_EXTENDED;
  case 511:
    return Status::SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED;
  default:
    return Status::UNKNOWN;
  }
}

char const *ToString(Status status)
{
  switch (status)
  {
  case Status::INFORMATION_CONTINUE:
    return "100 Continue";
  case Status::INFORMATION_SWITCHING_PROTOCOLS:
    return "101 Switching Protocols";
  case Status::INFORMATION_PROCESSING:
    return "102 Processing";
  case Status::SUCCESS_OK:
    return "200 OK";
  case Status::SUCCESS_CREATED:
    return "201 Created";
  case Status::SUCCESS_ACCEPTED:
    return "202 Accepted";
  case Status::SUCCESS_NON_AUTHORITATIVE_INFORMATION:
    return "203 Non-Authoritative Information";
  case Status::SUCCESS_NO_CONTENT:
    return "204 No Content";
  case Status::SUCCESS_RESET_CONTENT:
    return "205 Reset Content";
  case Status::SUCCESS_PARTIAL_CONTENT:
    return "206 Partial Content";
  case Status::SUCCESS_MULTI_STATUS:
    return "207 Multi-Status";
  case Status::SUCCESS_ALREADY_REPORTED:
    return "208 Already Reported";
  case Status::SUCCESS_IM_USED:
    return "226 IM Used";
  case Status::REDIRECTION_MULTIPLE_CHOICES:
    return "300 Multiple Choices";
  case Status::REDIRECTION_MOVED_PERMANENTLY:
    return "301 Moved Permanently";
  case Status::REDIRECTION_FOUND:
    return "302 Found";
  case Status::REDIRECTION_SEE_OTHER:
    return "303 See Other";
  case Status::REDIRECTION_NOT_MODIFIED:
    return "304 Not Modified";
  case Status::REDIRECTION_USE_PROXY:
    return "305 Use Proxy";
  case Status::REDIRECTION_SWITCH_PROXY:
    return "306 Switch Proxy";
  case Status::REDIRECTION_TEMPORARY_REDIRECT:
    return "307 Temporary Redirect";
  case Status::REDIRECTION_PERMANENT_REDIRECT:
    return "308 Permanent Redirect";
  case Status::CLIENT_ERROR_BAD_REQUEST:
    return "400 Bad Request";
  case Status::CLIENT_ERROR_UNAUTHORIZED:
    return "401 Unauthorized";
  case Status::CLIENT_ERROR_PAYMENT_REQUIRED:
    return "402 Payment Required";
  case Status::CLIENT_ERROR_FORBIDDEN:
    return "403 Forbidden";
  case Status::CLIENT_ERROR_NOT_FOUND:
    return "404 Not Found";
  case Status::CLIENT_ERROR_METHOD_NOT_ALLOWED:
    return "405 Method Not Allowed";
  case Status::CLIENT_ERROR_NOT_ACCEPTABLE:
    return "406 Not Acceptable";
  case Status::CLIENT_ERROR_PROXY_AUTHENTICATION_REQUIRED:
    return "407 Proxy Authentication Required";
  case Status::CLIENT_ERROR_REQUEST_TIMEOUT:
    return "408 Request Timeout";
  case Status::CLIENT_ERROR_CONFLICT:
    return "409 Conflict";
  case Status::CLIENT_ERROR_LENGTH_REQUIRED:
    return "411 Length Required";
  case Status::CLIENT_ERROR_PRECONDITION_FAILED:
    return "412 Precondition Failed";
  case Status::CLIENT_ERROR_PAYLOAD_TOO_LARGE:
    return "413 Payload Too Large";
  case Status::CLIENT_ERROR_URI_TOO_LONG:
    return "414 URI Too Long";
  case Status::CLIENT_ERROR_UNSUPPORTED_MEDIA_TYPE:
    return "415 Unsupported Media Type";
  case Status::CLIENT_ERROR_RANGE_NOT_SATISFIABLE:
    return "416 Range Not Satisfiable";
  case Status::CLIENT_ERROR_EXPECTATION_FAILED:
    return "417 Expectation Failed";
  case Status::CLIENT_ERROR_IM_A_TEAPOT:
    return "418 I'm a teapot";
  case Status::CLIENT_ERROR_MISDIRECTION_REQUIRED:
    return "421 Misdirected Request";
  case Status::CLIENT_ERROR_UNPROCESSABLE_ENTITY:
    return "422 Unprocessable Entity";
  case Status::CLIENT_ERROR_LOCKED:
    return "423 Locked";
  case Status::CLIENT_ERROR_FAILED_DEPENDENCY:
    return "424 Failed Dependency";
  case Status::CLIENT_ERROR_UPGRADE_REQUIRED:
    return "426 Upgrade Required";
  case Status::CLIENT_ERROR_PRECONDITION_REQUIRED:
    return "428 Precondition Required";
  case Status::CLIENT_ERROR_TOO_MANY_REQUESTS:
    return "429 Too Many Requests";
  case Status::CLIENT_ERROR_REQUEST_HEADER_FIELDS_TOO_LARGE:
    return "431 Request Header Fields Too Large";
  case Status::CLIENT_ERROR_UNAVAILABLE_FOR_LEGAL_REASONS:
    return "451 Unavailable For Legal Reasons";
  case Status::SERVER_ERROR_INTERNAL_SERVER_ERROR:
    return "500 Internal Server Error";
  case Status::SERVER_ERROR_NOT_IMPLEMENTED:
    return "501 Not Implemented";
  case Status::SERVER_ERROR_BAD_GATEWAY:
    return "502 Bad Gateway";
  case Status::SERVER_ERROR_SERVICE_UNAVAILABLE:
    return "503 Service Unavailable";
  case Status::SERVER_ERROR_GATEWAY_TIMEOUT:
    return "504 Gateway Timeout";
  case Status::SERVER_ERROR_HTTP_VERSION_NOT_SUPPORTED:
    return "505 HTTP Version Not Supported";
  case Status::SERVER_ERROR_VARIANT_ALSO_NEGOTIATES:
    return "506 Variant Also Negotiates";
  case Status::SERVER_ERROR_INSUFFICIENT_STORAGE:
    return "507 Insufficient Storage";
  case Status::SERVER_ERROR_LOOP_DETECTED:
    return "508 Loop Detected";
  case Status::SERVER_ERROR_NOT_EXTENDED:
    return "510 Not Extended";
  case Status::SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED:
    return "511 Network Authentication Required";

  case Status::UNKNOWN:
  default:
    return "0 Unknown";
  }
}

}  // namespace http
}  // namespace fetch
