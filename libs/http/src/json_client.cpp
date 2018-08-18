#include "http/json_client.hpp"
#include "core/json/document.hpp"

#include <sstream>
#include <utility>

namespace fetch {
namespace http {

JsonHttpClient::JsonHttpClient(std::string host, uint16_t port) : client_(std::move(host), port) {}

bool JsonHttpClient::Get(ConstByteArray const &endpoint, Variant const &request, Variant &response)
{
  return Request(Method::GET, endpoint, &request, response);
}

bool JsonHttpClient::Get(JsonHttpClient::ConstByteArray const &endpoint,
                         JsonHttpClient::Variant &             response)
{
  return Request(Method::GET, endpoint, nullptr, response);
}

bool JsonHttpClient::Post(ConstByteArray const &endpoint, Variant const &request, Variant &response)
{
  return Request(Method::POST, endpoint, &request, response);
}

bool JsonHttpClient::Post(JsonHttpClient::ConstByteArray const &endpoint,
                          JsonHttpClient::Variant &             response)
{
  return Request(Method::POST, endpoint, nullptr, response);
}

bool JsonHttpClient::Request(Method method, JsonHttpClient::ConstByteArray const &endpoint,
                             JsonHttpClient::Variant const *request,
                             JsonHttpClient::Variant &      response)
{
  bool success = false;

  // make the request
  fetch::http::HTTPRequest http_request;
  http_request.SetMethod(method);
  http_request.SetURI(endpoint);

  if (request)
  {
    std::ostringstream oss;
    oss << *request;
    http_request.SetBody(oss.str());
  }

  fetch::http::HTTPResponse http_response;
  if (client_.Request(http_request, http_response))
  {
    success = true;

    json::JSONDocument doc(http_response.body());
    response = doc.root();
  }

  return success;
}

}  // namespace http
}  // namespace fetch
