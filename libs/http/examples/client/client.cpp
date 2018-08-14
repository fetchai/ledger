#include "core/commandline/params.hpp"
#include "http/client.hpp"

#include <iostream>

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::Params parser;

  std::string host;
  uint16_t    port = 0;
  std::string method;
  std::string endpoint;

  parser.add(host, "host", "The hostname or IP to connect to");
  parser.add(port, "port", "The port number to connect to", uint16_t{80});
  parser.add(method, "method", "The http method to be used", std::string{"GET"});
  parser.add(endpoint, "endpoint", "The endpoint to be requested", std::string{"/"});

  parser.Parse(argc, argv);

  // create the client
  fetch::http::HTTPClient client(host, port);

  // make the request
  fetch::http::HTTPRequest req;
  req.SetMethod(fetch::http::Method::GET);
  req.SetURI("/api/");

  fetch::http::HTTPResponse response;
  if (client.Request(req, response))
  {
    std::cout << response.body() << std::endl;

    exit_code = EXIT_SUCCESS;
  }

  return exit_code;
}
