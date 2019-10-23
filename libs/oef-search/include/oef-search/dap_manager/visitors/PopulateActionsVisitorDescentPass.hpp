#pragma once

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
  virtual ~PopulateActionsVisitorDescentPass()
  {}

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t depth) override;
  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t depth) override;

protected:
  std::unordered_set<std::string>                                                    dap_names_;
  std::string                                                                        current_dap_;
  std::shared_ptr<FutureComplexType<std::shared_ptr<ConstructQueryMementoResponse>>> future_ =
      nullptr;

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
