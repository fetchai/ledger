#include "core/commandline/params.hpp"
#include "http/json_client.hpp"

#include <iostream>

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::Params parser;

  std::string host;
  uint16_t    port = 0;
  std::string method;
  std::string endpoint;

  parser.add(host, "host", "The hostname or IP to connect to", std::string{"api.ipify.org"});
  parser.add(port, "port", "The port number to connect to", uint16_t{80});
  parser.add(method, "method", "The http method to be used", std::string{"GET"});
  parser.add(endpoint, "endpoint", "The endpoint to be requested", std::string{"/"});

  parser.Parse(argc, argv);

  // create the client
  fetch::http::JsonHttpClient client(host, port);

  // construct the request
//  fetch::script::Variant request;
//  request.MakeObject();
//  request["public_key"] = "foobar";
//  request["host"] = "127.0.0.1";
//  request["port"] = 8080;
//  request["network"] = 16;
//  request["client_name"] = "json-client-example";
//  request["client_version"] = "v0.0.1";

  fetch::script::Variant response;
  if (client.Get("/?format=json", response))
  {
    std::cout << "Response\n\n" << response << std::endl;

    exit_code = EXIT_SUCCESS;
  }
  else
  {
    std::cout << "ERROR: Unable to make query" << std::endl;
  }

  return exit_code;
}
