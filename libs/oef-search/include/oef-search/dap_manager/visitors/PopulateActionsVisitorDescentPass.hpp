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

#include "oef-base/threading/Future.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>
#include <stack>

class DapManager;
class DapStore;

class PopulateActionsVisitorDescentPass : public Visitor<Queue>
{
public:
  static constexpr char const *LOGGING_NAME = "PopulateActionsVisitorDescentPass";
  using VisitNodeExitStates                 = typename Visitor<Queue>::VisitNodeExitStates;

  PopulateActionsVisitorDescentPass(std::shared_ptr<DapManager> dap_manager,
                                    std::shared_ptr<DapStore>   dap_store);
  virtual ~PopulateActionsVisitorDescentPass() = default;

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t depth) override;
  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t depth) override;

protected:
  std::unordered_set<std::string> dap_names_;
  std::string                     current_dap_;
  std::shared_ptr<
      fetch::oef::base::FutureComplexType<std::shared_ptr<ConstructQueryMementoResponse>>>
      future_ = nullptr;

  std::shared_ptr<DapManager> dap_manager_;
  std::shared_ptr<DapStore>   dap_store_;

private:
  PopulateActionsVisitorDescentPass(const PopulateActionsVisitorDescentPass &other) =
      delete;  // { copy(other); }
  PopulateActionsVisitorDescentPass &operator=(const PopulateActionsVisitorDescentPass &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const PopulateActionsVisitorDescentPass &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const PopulateActionsVisitorDescentPass &other) =
      delete;  // const { return compare(other)==-1; }
};
