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

#include "core/commandline/vt100.hpp"
#include <iostream>
namespace fetch {
namespace http {
namespace middleware {

inline void ColorLog(fetch::http::HTTPResponse &res, fetch::http::HTTPRequest const &req)
{
  using namespace fetch::commandline::VT100;
  std::string color = "";

  switch (static_cast<uint16_t>(res.status()) / 100u)
  {
  case 1:
    color = GetColor(4, 9);
    break;
  case 2:
    color = GetColor(3, 9);
    break;
  case 3:
    color = GetColor(5, 9);
    break;
  case 4:
    color = GetColor(1, 9);
    break;
  case 5:
    color = GetColor(6, 9);
    break;
  default:
    color = GetColor(9, 9);
    break;
  };

  std::cout << "[ " << color << ToString(res.status()) << DefaultAttributes() << " ] " << req.uri();
  std::cout << ", " << GetColor(5, 9) << res.mime_type().type << DefaultAttributes() << std::endl;
}
}  // namespace middleware
}  // namespace http
}  // namespace fetch
