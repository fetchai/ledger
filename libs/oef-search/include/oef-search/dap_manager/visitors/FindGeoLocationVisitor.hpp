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

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class FindGeoLocationVisitor : public Visitor<Queue>
{
public:
  static constexpr char const *LOGGING_NAME = "FindGeoLocationVisitor";
  using VisitNodeExitStates                 = typename Visitor<Queue>::VisitNodeExitStates;

  explicit FindGeoLocationVisitor(std::shared_ptr<DapStore> const &dap_store)
    : geo_dap_{dap_store->GetGeoDap()}
    , location_root_{nullptr}
  {}

  virtual ~FindGeoLocationVisitor() = default;

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t /*depth*/)
  {
    const auto &daps = node.GetDapNames();

    if (daps.find(geo_dap_) != daps.end())
    {
      location_root_ = std::make_shared<Branch>(*node.GetProto());
      return VisitNodeExitStates ::STOP;
    }

    return VisitNodeExitStates::COMPLETE;
  }
  virtual VisitNodeExitStates VisitLeaf(Leaf & /*leaf*/, uint32_t /*depth*/)
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
