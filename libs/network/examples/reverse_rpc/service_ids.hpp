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

#include <string>
#include <vector>

struct AEAToNode
{
  enum
  {
    REGISTER = 1
  };
};

struct NodeToAEA
{
  enum
  {
    SEARCH = 1
  };
};

struct FetchProtocols
{

  enum
  {
    AEA_TO_NODE = 1,
    NODE_TO_AEA = 2
  };
};

static const int SERVICE_TEST = 1;
static const int CHANNEL_RPC  = 1;

using Strings = std::vector<std::string>;