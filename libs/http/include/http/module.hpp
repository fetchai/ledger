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
#include "http/validators.hpp"
#include "http/view_parameters.hpp"

#include <vector>
namespace fetch {
namespace http {

struct HTTPParameter
{
  byte_array::ConstByteArray name;
  std::string                description;
  validators::Validator      validator;
};

class HTTPModule
{
public:
  enum Interface : uint8_t
  {
    JSON = 1,
    YAML = 2
  };

  HTTPModule() = default;

  HTTPModule(HTTPModule const &rhs) = delete;
  HTTPModule(HTTPModule &&rhs)      = delete;
  HTTPModule &operator=(HTTPModule const &rhs) = delete;
  HTTPModule &operator=(HTTPModule &&rhs) = delete;

  using view_type = std::function<HTTPResponse(ViewParameters, HTTPRequest)>;

  struct UnmountedView
  {
    byte_array::ConstByteArray description;
    Method                     method;
    byte_array::ByteArray      route;
    std::vector<HTTPParameter> parameters;
    view_type                  view;
  };

  void Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
            std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::POST, path, description, parameters, view);
  }

  void Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
            view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::POST, path, description, {}, view);
  }

  void Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
           std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::GET, path, description, parameters, view);
  }

  void Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
           view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::GET, path, description, {}, view);
  }

  void Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
           std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PUT, path, description, parameters, view);
  }

  void Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
           view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PUT, path, description, {}, view);
  }

  void Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
             std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PATCH, path, description, parameters, view);
  }

  void Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
             view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::PATCH, path, description, {}, view);
  }

  void Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
              std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::DELETE, path, description, parameters, view);
  }

  void Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
              view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    AddView(Method::DELETE, path, description, {}, view);
  }

  void AddView(Method method, byte_array::ByteArray const &path,
               byte_array::ByteArray const &     description,
               std::vector<HTTPParameter> const &parameters, view_type const &view)
  {
    LOG_STACK_TRACE_POINT;

    views_.push_back({description, method, path, parameters, view});
  }

  std::vector<UnmountedView> const &views() const
  {
    LOG_STACK_TRACE_POINT;

    return views_;
  }

private:
  std::vector<UnmountedView> views_;
  fetch::variant::Variant    interface_description_;
  std::string                name_;
};
}  // namespace http
}  // namespace fetch
