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

#include "oef-messages/dap_interface.hpp"

#include <unordered_set>

class Node
{
public:
  static constexpr char const *LOGGING_NAME = "Node";
  using DapMemento = std::pair<std::string, std::shared_ptr<ConstructQueryMementoResponse>>;

  Node()                  = default;
  virtual ~Node()         = default;
  Node(const Node &other) = delete;
  Node &operator=(const Node &other) = delete;

  bool operator==(const Node &other) = delete;
  bool operator<(const Node &other)  = delete;

  virtual void SetDapNames(std::unordered_set<std::string> dap_names)
  {
    dap_names_ = std::move(dap_names);
  }

  virtual void AddDapName(const std::string &name)
  {
    dap_names_.insert(name);
  }

  virtual std::unordered_set<std::string> &GetDapNames()
  {
    return dap_names_;
  }

  virtual const std::unordered_set<std::string> &GetDapNames() const
  {
    return dap_names_;
  }

  virtual void ClearDapNames()
  {
    dap_names_.clear();
  }

  virtual void AddMemento(std::string                                    dap_name,
                          std::shared_ptr<ConstructQueryMementoResponse> memento)
  {
    mementos_.push_back(std::make_pair(std::move(dap_name), std::move(memento)));
  }

  virtual std::vector<DapMemento> &GetMementos()
  {
    return mementos_;
  }

  virtual std::string GetNodeType() = 0;
  virtual std::string ToString()    = 0;

protected:
  std::unordered_set<std::string> dap_names_{};
  std::vector<DapMemento>         mementos_;
};
