#pragma once

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class AddMoreDapsBasedOnOptionsVisitor : public Visitor<Stack>
{
public:
  static constexpr char const *LOGGING_NAME = "AddMoreDapsBasedOnOptionsVisitor";
  using VisitNodeExitStates                 = typename Visitor<Stack>::VisitNodeExitStates;

  AddMoreDapsBasedOnOptionsVisitor(std::shared_ptr<DapStore> dap_store)
    : dap_store_{std::move(dap_store)}
  {}

  virtual ~AddMoreDapsBasedOnOptionsVisitor()
  {}

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t /*depth*/)
  {
    std::vector<std::string> options{"all-branches", "all-node"};
    for (const auto &dap_name : dap_store_->GetDapNamesByOptions(options))
    {
      node.AddDapName(dap_name);
    }
    return VisitNodeExitStates::COMPLETE;
  }

  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t /*depth*/)
  {
    std::vector<std::string> options{"all-leaf", "all-nodes"};
    for (const auto &dap_name : dap_store_->GetDapNamesByOptions(options))
    {
      leaf.AddDapName(dap_name);
    }
    return VisitNodeExitStates::COMPLETE;
  }

protected:
  std::shared_ptr<DapStore> dap_store_;

private:
  AddMoreDapsBasedOnOptionsVisitor(const AddMoreDapsBasedOnOptionsVisitor &other) =
      delete;  // { copy(other); }
  AddMoreDapsBasedOnOptionsVisitor &operator=(const AddMoreDapsBasedOnOptionsVisitor &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const AddMoreDapsBasedOnOptionsVisitor &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const AddMoreDapsBasedOnOptionsVisitor &other) =
      delete;  // const { return compare(other)==-1; }
};
