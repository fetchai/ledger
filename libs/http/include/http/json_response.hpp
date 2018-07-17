#ifndef FETCH_JSON_RESPONSE_HPP
#define FETCH_JSON_RESPONSE_HPP

#include "http/response.hpp"
#include "http/mime_types.hpp"

namespace fetch {
namespace http {

inline http::HTTPResponse CreateJsonResponse(byte_array::ConstByteArray const &body, Status const &status = status_code::SUCCESS_OK) {
  static const auto jsonMimeType = mime_types::GetMimeTypeFromExtension(".json");
  return http::HTTPResponse(body, jsonMimeType, status);
}

} // namespace http
} // namespace fetch

#endif //FETCH_JSON_RESPONSE_HPP
