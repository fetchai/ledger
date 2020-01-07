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

#include "core/mutex.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/visitors/PopulateActionsVisitorDescentPass.hpp"

PopulateActionsVisitorDescentPass::PopulateActionsVisitorDescentPass(
    std::shared_ptr<DapManager> dap_manager, std::shared_ptr<DapStore> dap_store)
  : dap_manager_{std::move(dap_manager)}
  , dap_store_{std::move(dap_store)}
{}

PopulateActionsVisitorDescentPass::VisitNodeExitStates PopulateActionsVisitorDescentPass::VisitNode(
    Branch &node, uint32_t /*depth*/)
{
  if (future_)
  {
    auto result = future_->get();

    if (result != nullptr && result->success())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Gotcha ", current_dap_, " node now at ", node.ToString());
      node.AddMemento(current_dap_, std::move(result));
    }

    future_ = nullptr;
    dap_names_.erase(current_dap_);
    if (!dap_store_->IsDapLate(current_dap_))
    {
      return VisitNodeExitStates ::STOP;
    }
    if (dap_names_.empty())
    {
      return VisitNodeExitStates ::COMPLETE;
    }
  }
  else
  {
    dap_names_ = node.GetDapNames();
  }

  std::weak_ptr<PopulateActionsVisitorDescentPass> this_wp =
      this->shared_from_base<PopulateActionsVisitorDescentPass>();
  for (const auto &dap_name : dap_names_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Dear ", dap_name, " would you like to consume ", node.ToString(),
                   " ?");

    current_dap_ = dap_name;
    future_ =
        dap_manager_->SingleDapCall<ConstructQueryObjectRequest, ConstructQueryMementoResponse>(
            dap_name, "prepare", node.ToProto(dap_name));

    if (future_->MakeNotification()
            .Then([this_wp]() {
              auto sp = this_wp.lock();
              if (sp)
              {
                sp->MakeRunnable();
              }
            })
            .Waiting())
    {
      return VisitNodeExitStates ::DEFER;
    }
    return VisitNodeExitStates ::RERUN;
  }

  return VisitNodeExitStates ::COMPLETE;
}

PopulateActionsVisitorDescentPass::VisitNodeExitStates PopulateActionsVisitorDescentPass::VisitLeaf(
    Leaf &leaf, uint32_t /*depth*/)
{
  if (future_)
  {
    auto result = future_->get();

    if (result != nullptr && result->success())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Gotcha ", current_dap_, " leaf now at ", leaf.ToString());
      leaf.AddMemento(current_dap_, std::move(result));
    }

    future_ = nullptr;
    dap_names_.erase(current_dap_);
    if (dap_names_.empty())
    {
      return VisitNodeExitStates ::COMPLETE;
    }
  }
  else
  {
    dap_names_ = leaf.GetDapNames();
  }

  FETCH_LOG_INFO(LOGGING_NAME, "SIZE: ", dap_names_.size());

  std::weak_ptr<PopulateActionsVisitorDescentPass> this_wp =
      this->shared_from_base<PopulateActionsVisitorDescentPass>();
  for (const auto &dap_name : dap_names_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Dear ", dap_name, " would you write a constraint for ",
                   leaf.ToString(), " ?");

    current_dap_ = dap_name;
    future_ =
        dap_manager_
            ->SingleDapCall<ConstructQueryConstraintObjectRequest, ConstructQueryMementoResponse>(
                dap_name, "prepareConstraint", leaf.ToProto(dap_name));

    if (future_->MakeNotification()
            .Then([this_wp]() {
              auto sp = this_wp.lock();
              if (sp)
              {
                sp->MakeRunnable();
              }
            })
            .Waiting())
    {
      return VisitNodeExitStates ::DEFER;
    }
    return VisitNodeExitStates ::RERUN;
  }

  return VisitNodeExitStates ::COMPLETE;
}
