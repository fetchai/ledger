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

#include "core/logging.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"

#include <string>

namespace fetch {

class LoggingHttpModule : public http::HTTPModule
{
public:
  LoggingHttpModule()
  {
    Get("/api/logging/",
      "Returns a log of events.",
      [](http::ViewParameters const &, http::HTTPRequest const &) {
      Variant response{Variant::Object()};
      for (auto const &element : GetLogLevelMap())
      {
        response[element.first] = ToString(element.second);
      }

      return http::CreateJsonResponse(response);
    });

    Patch("/api/logging/", 
      "TODO(tfr): yet to be written",
      [](http::ViewParameters const &, http::HTTPRequest const &req) {
      std::string error{};
      try
      {
        json::JSONDocument doc{req.body()};
        auto const &       root = doc.root();

        if (root.IsObject())
        {
          root.IterateObject([&error](ConstByteArray const &k, Variant const &value) {
            std::string const key{k};
            LogLevel          level{LogLevel::INFO};

            if (!(value.IsString() && Parse(value.As<ConstByteArray>(), level)))
            {
              error = "Unable to parse log level entry";
              return false;
            }

            // update the logging level
            SetLogLevel(key.c_str(), level);

            return true;
          });
        }
        else
        {
          error = "Root is not a object";
        }
      }
      catch (json::JSONParseException const &)
      {
        error = "Unable to parse input request";
      }

      if (!error.empty())
      {
        std::string const text = R"({"error": ")" + error + "\"}";
        return http::CreateJsonResponse(text, http::Status::CLIENT_ERROR_BAD_REQUEST);
      }

      return http::CreateJsonResponse("{}");
    });
  }

private:
  using Variant        = variant::Variant;
  using ConstByteArray = byte_array::ConstByteArray;

  static bool Parse(ConstByteArray const &text, LogLevel &level)
  {
    bool success{false};

    if (text == "trace")
    {
      level   = LogLevel::TRACE;
      success = true;
    }
    else if (text == "debug")
    {
      level   = LogLevel::DEBUG;
      success = true;
    }
    else if (text == "info")
    {
      level   = LogLevel::INFO;
      success = true;
    }
    else if (text == "warning")
    {
      level   = LogLevel::WARNING;
      success = true;
    }
    else if (text == "error")
    {
      level   = LogLevel::ERROR;
      success = true;
    }
    else if (text == "critical")
    {
      level   = LogLevel::CRITICAL;
      success = true;
    }

    return success;
  }

  static char const *ToString(LogLevel level)
  {
    char const *text = "unknown";

    switch (level)
    {
    case LogLevel::TRACE:
      text = "trace";
      break;
    case LogLevel::DEBUG:
      text = "debug";
      break;
    case LogLevel::INFO:
      text = "info";
      break;
    case LogLevel::WARNING:
      text = "warning";
      break;
    case LogLevel::ERROR:
      text = "error";
      break;
    case LogLevel::CRITICAL:
      text = "critical";
      break;
    }

    return text;
  }
};

}  // namespace fetch
