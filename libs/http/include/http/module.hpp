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

#include "core/byte_array/byte_array.hpp"
#include "http/method.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/view_parameters.hpp"

#include <vector>
namespace fetch {
namespace http {

class HTTPModule
{
public:
  HTTPModule()                      = default;
  HTTPModule(HTTPModule const &rhs) = delete;
  HTTPModule(HTTPModule &&rhs)      = delete;
  HTTPModule &operator=(HTTPModule const &rhs) = delete;
  HTTPModule &operator=(HTTPModule &&rhs) = delete;

  using view_type = std::function<HTTPResponse(ViewParameters, HTTPRequest)>;

  struct UnmountedView
  {
    Method                method;
    byte_array::ByteArray route;
    view_type             view;
  };

  void Post(byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::POST, path, view);
  }

  void Get(byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::GET, path, view);
  }

  void Put(byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PUT, path, view);
  }

  void Patch(byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PATCH, path, view);
  }

  void Delete(byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::DELETE, path, view);
  }

  void AddView(Method method, byte_array::ByteArray const &path, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    views_.push_back({method, path, view});
  }

  std::vector<UnmountedView> const &views() const
  {
    LOG_STACK_TRACE_POINT;

    return views_;
  }

private:
  std::vector<UnmountedView> views_;
};
}  // namespace http
}  // namespace fetch
