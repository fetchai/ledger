#pragma once

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class PopulateFieldInformationVisitor : public Visitor<Queue>
{
public:
  static constexpr char const *LOGGING_NAME = "PopulateFieldInformationVisitor";
  using VisitNodeExitStates                 = typename Visitor<Queue>::VisitNodeExitStates;

  PopulateFieldInformationVisitor(std::shared_ptr<DapStore> dap_store)
    : dap_store_{std::move(dap_store)}
    , node_counter_{1}
    , leaf_counter_{1}
  {}

  virtual ~PopulateFieldInformationVisitor()
  {}

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t depth)
  {
    node.SetName("node" + std::to_string(node_counter_));
    ++node_counter_;
    return VisitNodeExitStates::COMPLETE;
  }
  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t depth)
  {
    dap_store_->UpdateTargetFieldAndTableNames(leaf);

    auto dap_names = dap_store_->GetDapsForAttributeName(leaf.GetTargetFieldName());
    leaf.SetDapNames(std::move(dap_names));
    leaf.SetName("leaf" + std::to_string(leaf_counter_));

    ++leaf_counter_;
    return VisitNodeExitStates::COMPLETE;
  }

protected:
  std::shared_ptr<DapStore> dap_store_;
  uint32_t                  node_counter_, leaf_counter_;

private:
  PopulateFieldInformationVisitor(const PopulateFieldInformationVisitor &other) =
      delete;  // { copy(other); }
  PopulateFieldInformationVisitor &operator=(const PopulateFieldInformationVisitor &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const PopulateFieldInformationVisitor &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const PopulateFieldInformationVisitor &other) =
      delete;  // const { return compare(other)==-1; }
};
