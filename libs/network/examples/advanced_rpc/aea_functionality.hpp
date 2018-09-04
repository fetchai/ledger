#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "node_functionality.hpp"

#include <iostream>
#include <string>
#include <vector>

class AEAFunctionality
{
public:
  AEAFunctionality(std::string node_info) : node_info_(node_info)
  {}

  void Connect(std::string address, uint16_t port)
  {
    if (node_ != nullptr)
    {
      std::cout << "Remote asking to connect to " << address << " " << port << std::endl;
      node_->Connect(address, port);
    }
  }

  void Disconnect(uint64_t handle)
  {}

  std::string get_info()
  {
    std::cout << "Sending info to client" << std::endl;
    return node_info_;
  }

  std::vector<std::string> const &peers() const
  {
    return peers_;
  }

  void set_node(NodeToNodeFunctionality *node)
  {
    node_ = node;
  }

private:
  NodeToNodeFunctionality *node_ = nullptr;
  std::string              node_info_;
  std::vector<std::string> peers_;
};
