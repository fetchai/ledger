//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "oef-base/utils/Uri.hpp"

#include <iostream>
#include <iterator>
#include <regex>
#include <string>

const char *what2str[] = {"*", "PROTO", "HOST", "PORT", "PATH"};

void Uri::parse(const std::string &value)
{
  std::regex uri_regex("^([-_a-zA-Z0-9\\.]+://)?([-_a-zA-Z0-9\\.]+)(:[0-9]+)?(/.*)?$",
                       std::regex_constants::icase);

  valid = false;
  proto = "";
  host  = "";
  path  = "";
  port  = 0;

  std::smatch parts_result;
  if (std::regex_match(value, parts_result, uri_regex))
  {
    std::vector<std::sub_match<std::string::const_iterator>> v(parts_result.begin(),
                                                               parts_result.end());

    using What        = enum { ALL, PROTO, HOST, PORT, PATH };
    What expectedpart = ALL;

    for (auto &part : v)
    {
      switch (expectedpart)
      {
      case ALL:
      {
        expectedpart = PROTO;
        break;
      }
      case PROTO:
      {
        if ((part.length() != 0) && part.str().back() == '/')
        {
          proto = part.str().substr(0, static_cast<unsigned long>(part.length() - 3));
        }
        expectedpart = HOST;
        break;
      }
      case HOST:
      {
        host         = part;
        expectedpart = PORT;
        break;
      }
      case PORT:
      {
        if ((part.length() != 0) && part.str().front() == ':')
        {
          auto tmp = part.str().substr(1);
          port     = static_cast<uint16_t>(std::stoi(tmp));
        }
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
