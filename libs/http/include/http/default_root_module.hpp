#pragma once
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

#include "http/method.hpp"
#include "http/module.hpp"
#include "http/route.hpp"

#include <vector>

namespace fetch {
namespace http {

struct MountedView
{
  using ViewType      = HTTPModule::ViewType;
  using Authenticator = HTTPModule::Authenticator;

  byte_array::ConstByteArray description;
  Method                     method;
  Route                      route;
  HTTPModule::ViewType       view;
  HTTPModule::Authenticator  authenticator;
};

using MountedViews = std::vector<MountedView>;

HTTPModule DefaultRootModule(MountedViews const &views);

}  // namespace http
}  // namespace fetch
