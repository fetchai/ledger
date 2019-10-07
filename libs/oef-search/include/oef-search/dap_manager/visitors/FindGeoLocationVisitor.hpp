#pragma once

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class FindGeoLocationVisitor : public Visitor<Queue>
{
public:
  static constexpr char const *LOGGING_NAME = "FindGeoLocationVisitor";
  using VisitNodeExitStates                 = typename Visitor<Queue>::VisitNodeExitStates;

  FindGeoLocationVisitor(const std::shared_ptr<DapStore> &dap_store)
    : geo_dap_{dap_store->GetGeoDap()}
    , location_root_{nullptr}
  {}

  virtual ~FindGeoLocationVisitor() = default;

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t depth)
  {
    const auto &daps = node.GetDapNames();

    if (daps.find(geo_dap_) != daps.end())
    {
      location_root_ = std::make_shared<Branch>(*node.GetProto());
      return VisitNodeExitStates ::STOP;
    }

    return VisitNodeExitStates::COMPLETE;
  }
  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t depth)
  {
    return VisitNodeExitStates::COMPLETE;
  }

  std::shared_ptr<Branch> &GetLocationRoot()
  {
    return location_root_;
  }

protected:
  std::string             geo_dap_;
  std::shared_ptr<Branch> location_root_;

private:
  FindGeoLocationVisitor(const FindGeoLocationVisitor &other) = delete;  // { copy(other); }
  FindGeoLocationVisitor &operator=(const FindGeoLocationVisitor &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const FindGeoLocationVisitor &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const FindGeoLocationVisitor &other) =
      delete;  // const { return compare(other)==-1; }
};
