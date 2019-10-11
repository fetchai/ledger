#pragma once

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class CollectDapsVisitor : public Visitor<Queue>
{
public:
  static constexpr char const *LOGGING_NAME = "CollectDapsVisitor";
  using VisitNodeExitStates                 = typename Visitor<Queue>::VisitNodeExitStates;

  CollectDapsVisitor()
  {}

  virtual ~CollectDapsVisitor()
  {}

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t /*depth*/)
  {
    node.MergeDaps();
    return VisitNodeExitStates::COMPLETE;
  }
  virtual VisitNodeExitStates VisitLeaf(Leaf &/*leaf*/, uint32_t /*depth*/)
  {
    return VisitNodeExitStates::COMPLETE;
  }

protected:
private:
  CollectDapsVisitor(const CollectDapsVisitor &other) = delete;  // { copy(other); }
  CollectDapsVisitor &operator                        =(const CollectDapsVisitor &other) =
      delete;                                                 // { copy(other); return *this; }
  bool operator==(const CollectDapsVisitor &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const CollectDapsVisitor &other) = delete;  // const { return compare(other)==-1; }
};
