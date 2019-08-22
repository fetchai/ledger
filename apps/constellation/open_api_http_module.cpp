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

#include "open_api_http_module.hpp"

#include <mutex>

namespace fetch {

OpenAPIHttpModule::OpenAPIHttpModule()
{
  Get("/api/definitions", "Returns the API definition.",
      [this](http::ViewParameters const &, http::HTTPRequest const &) {
        std::lock_guard<std::mutex> lock(server_lock_);

        Variant response{Variant::Object()};
        if (server_ == nullptr)
        {
          return http::CreateJsonResponse(response);
        }

        Variant paths{Variant::Object()};
        for (auto const &view : server_->views_unsafe())
        {

          std::string method = ToString(view.method);
          std::transform(method.begin(), method.end(), method.begin(),
                         [](unsigned char c) { return std::tolower(c); });

          if (!paths.Has(view.route.path()))
          {
            paths[view.route.path()] = Variant::Object();
          }
          Variant &path    = paths[view.route.path()];
          Variant  details = Variant::Object();

          auto    params     = view.route.path_parameters();
          Variant parameters = Variant::Array(params.size());

          uint64_t i = 0;
          for (auto &param : params)
          {
            Variant x = Variant::Object();

            x["in"]   = "path";
            x["name"] = param;

            if (view.route.HasParameterDetails(param))
            {
              x["description"] = view.route.GetDescription(param);
              x["schema"]      = view.route.GetSchema(param);
            }

            parameters[i] = x;
            ++i;
          }

          details["description"] = view.description;
          details["parameters"]  = parameters;

          path[method] = details;
        }

        response["paths"] = paths;

        return http::CreateJsonResponse(response);
      });
}

void OpenAPIHttpModule::Reset(http::HTTPServer *srv)
{
  std::lock_guard<std::mutex> lock(server_lock_);
  server_ = srv;
}

}  // namespace fetch
