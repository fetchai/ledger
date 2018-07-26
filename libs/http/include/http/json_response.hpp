#pragma once

#include "http/response.hpp"
#include "http/mime_types.hpp"
#include "core/script/variant.hpp"

#include <sstream>
namespace fetch {
namespace http {

inline http::HTTPResponse CreateJsonResponse(byte_array::ConstByteArray const &body, Status const &status = status_code::SUCCESS_OK) {
  static const auto jsonMimeType = mime_types::GetMimeTypeFromExtension(".json");
  return http::HTTPResponse(body, jsonMimeType, status);
}

inline http::HTTPResponse CreateJsonResponse(script::Variant const &doc, Status const &status = status_code::SUCCESS_OK) {
  static const auto jsonMimeType = mime_types::GetMimeTypeFromExtension(".json");
  std::stringstream body;
  body << doc;
  return http::HTTPResponse(body.str(), jsonMimeType, status);
}

} // namespace http
} // namespace fetch

