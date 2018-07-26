#pragma once
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

  typedef std::function<HTTPResponse(ViewParameters, HTTPRequest)> view_type;

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

  void AddView(Method method, byte_array::ByteArray const &path,
               view_type const &view)
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

