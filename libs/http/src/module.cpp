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

#include "http/module.hpp"

namespace fetch {
namespace http {

bool NormalAccessAuthentication(HTTPRequest const &req)
{
  if (req.authentication_level() < AuthenticationLevel::DEFUALT_LEVEL)
  {
    return false;
  }

  return true;
}

/// Post methods
/// @{
void HTTPModule::Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                      std::vector<HTTPParameter> const &parameters,
                      HTTPModule::ViewType const &      view)
{
  AddView(Method::POST, path, description, parameters, view);
}

void HTTPModule::Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                      HTTPModule::ViewType const &view)
{
  AddView(Method::POST, path, description, {}, view);
}

void HTTPModule::Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                      std::vector<HTTPParameter> const &parameters,
                      HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::POST, path, description, parameters, view, auth);
}

void HTTPModule::Post(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                      HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::POST, path, description, {}, view, auth);
}
/// @}

/// Get methods
/// @{
void HTTPModule::Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     std::vector<HTTPParameter> const &parameters, HTTPModule::ViewType const &view)
{
  AddView(Method::GET, path, description, parameters, view);
}

void HTTPModule::Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     HTTPModule::ViewType const &view)
{
  AddView(Method::GET, path, description, {}, view);
}

void HTTPModule::Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     std::vector<HTTPParameter> const &parameters,
                     HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::GET, path, description, parameters, view, auth);
}

void HTTPModule::Get(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::GET, path, description, {}, view, auth);
}
/// @}

/// Put methods
/// @{
void HTTPModule::Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     std::vector<HTTPParameter> const &parameters, HTTPModule::ViewType const &view)
{
  AddView(Method::PUT, path, description, parameters, view);
}

void HTTPModule::Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     HTTPModule::ViewType const &view)
{
  AddView(Method::PUT, path, description, {}, view);
}

void HTTPModule::Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     std::vector<HTTPParameter> const &parameters,
                     HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::PUT, path, description, parameters, view, auth);
}

void HTTPModule::Put(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                     HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::PUT, path, description, {}, view, auth);
}
/// @}

/// Patch methods
/// @{
void HTTPModule::Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                       std::vector<HTTPParameter> const &parameters,
                       HTTPModule::ViewType const &      view)
{
  AddView(Method::PATCH, path, description, parameters, view);
}

void HTTPModule::Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                       HTTPModule::ViewType const &view)
{
  AddView(Method::PATCH, path, description, {}, view);
}

void HTTPModule::Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                       std::vector<HTTPParameter> const &parameters,
                       HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::PATCH, path, description, parameters, view, auth);
}

void HTTPModule::Patch(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                       HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::PATCH, path, description, {}, view, auth);
}
/// @}

/// Delete methods
/// @{
void HTTPModule::Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                        std::vector<HTTPParameter> const &parameters,
                        HTTPModule::ViewType const &      view)
{
  AddView(Method::DELETE, path, description, parameters, view);
}

void HTTPModule::Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                        HTTPModule::ViewType const &view)
{
  AddView(Method::DELETE, path, description, {}, view);
}

void HTTPModule::Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                        HTTPModule::Authenticator const & auth,
                        std::vector<HTTPParameter> const &parameters,
                        HTTPModule::ViewType const &      view)
{
  AddView(Method::DELETE, path, description, parameters, view, auth);
}

void HTTPModule::Delete(byte_array::ByteArray const &path, byte_array::ByteArray const &description,
                        HTTPModule::Authenticator const &auth, HTTPModule::ViewType const &view)
{
  AddView(Method::DELETE, path, description, {}, view, auth);
}
/// @}

void HTTPModule::AddView(Method method, byte_array::ByteArray const &path,
                         byte_array::ByteArray const &     description,
                         std::vector<HTTPParameter> const &parameters,
                         HTTPModule::ViewType const &view, HTTPModule::Authenticator const &auth)
{
  views_.push_back({description, method, path, parameters, view, auth});
}

std::vector<HTTPModule::UnmountedView> const &HTTPModule::views() const
{
  return views_;
}

}  // namespace http
}  // namespace fetch
