#include "Uri.hpp"

#include <iostream>
#include <iterator>
#include <string>
#include <regex>

const char *what2str[] = {
  "*", "PROTO", "HOST", "PORT", "PATH"
};

void Uri::parse(const std::string &s)
{
  std::regex uri_regex("^([-a-zA-Z0-9\\.]+:)?//([-a-zA-Z0-9\\.]+)(:[0-9]+)?(/.*)?$", std::regex_constants::icase);

  valid = false;
  proto = "";
  host = "";
  path = "";
  port = 0;

  std::smatch parts_result;
  if (std::regex_match(s, parts_result, uri_regex)) {
    std::vector<std::sub_match<std::string::const_iterator>> v(parts_result.begin(), parts_result.end());

    using What = enum { ALL, PROTO, HOST, PORT, PATH };
    What expectedpart = ALL;

    for( auto& part: v)
    {
      switch(expectedpart)
      {
      case ALL:
        expectedpart = PROTO;
        break;
      case PROTO:
        if (part.length() && part.str().back() == ':')
        {
          proto = part;
          expectedpart = HOST;
          break;
        }
      case HOST:
        {
          host = part;
          expectedpart = PORT;
          break;
        }
      case PORT:
        if (part.length() && part.str().front() == ':')
        {
          auto tmp = part.str().substr(1);
          port = static_cast<uint16_t>(std::stoi( tmp ));
          expectedpart = PATH;
          break;
        }
      case PATH:
        {
          path = part;
          break;
        }
      }
    }
    valid = true;
  }
}
