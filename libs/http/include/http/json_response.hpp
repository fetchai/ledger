#pragma once

#include "core/script/variant.hpp"
#include "http/mime_types.hpp"
#include "http/response.hpp"

#include <sstream>
namespace fetch {
namespace http {

inline http::HTTPResponse CreateJsonResponse(byte_array::ConstByteArray const &body,
                                             Status status = Status::SUCCESS_OK)
{
  static const auto jsonMimeType = mime_types::GetMimeTypeFromExtension(".json");
  return http::HTTPResponse(body, jsonMimeType, status);
}

inline http::HTTPResponse CreateJsonResponse(script::Variant const &doc,
                                             Status status = Status::SUCCESS_OK)
{
  static const auto jsonMimeType = mime_types::GetMimeTypeFromExtension(".json");
  std::stringstream body;
  body << doc;
  return http::HTTPResponse(body.str(), jsonMimeType, status);
}

}  // namespace http
}  // namespace fetch
