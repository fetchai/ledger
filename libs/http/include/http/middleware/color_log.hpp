#pragma once
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

  std::cout << "[ " << color << ToString(res.status()) << DefaultAttributes() << " ] "
            << req.uri();
  std::cout << ", " << GetColor(5, 9) << res.mime_type().type << DefaultAttributes() << std::endl;
}
}  // namespace middleware
}  // namespace http
}  // namespace fetch
