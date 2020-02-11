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

#include <memory>

#include "Node.hpp"
#include "oef-messages/dap_interface.hpp"

class Leaf : public Node
{
public:
  static constexpr char const *LOGGING_NAME = "Leaf";
  using Node::dap_names_;

  Leaf() = default;
  explicit Leaf(ConstructQueryConstraintObjectRequest const &proto)
    : proto_{std::make_shared<ConstructQueryConstraintObjectRequest>(proto)}
  {}
  ~Leaf() override        = default;
  Leaf(const Leaf &other) = delete;
  Leaf &operator=(const Leaf &other) = delete;

  bool operator==(const Leaf &other) = delete;
  bool operator<(const Leaf &other)  = delete;

  std::string GetNodeType() override
  {
    return "leaf";
  }

  void SetName(const std::string &name)
  {
    proto_->set_node_name(name);
  }

  const std::string &GetTargetFieldName() const
  {
    return proto_->target_field_name();
  }

  const std::string &GetTargetTableName() const
  {
    return proto_->target_table_name();
  }

  const std::string &GetQueryFieldType() const
  {
    return proto_->query_field_type();
  }

  const ValueMessage &GetQueryFieldValue() const
  {
    return proto_->query_field_value();
  }

  void SetTargetFieldName(const std::string &name)
  {
    proto_->set_target_field_name(name);
  }

  void SetTargetTableName(const std::string &name)
  {
    proto_->set_target_table_name(name);
  }

  std::shared_ptr<ConstructQueryConstraintObjectRequest> ToProto(const std::string &dap_name)
  {
    auto pt = std::make_shared<ConstructQueryConstraintObjectRequest>(*proto_);
    pt->set_dap_name(dap_name);
    return pt;
  }

  std::string ToString() override
  {
    std::string s = "Leaf " + proto_->node_name() + " -- ";
    s += proto_->target_field_name() + " " + proto_->operator_() + " ";
    if (proto_->query_field_type() == "data_model")
    {
      s += " DATA_MODEL";
    }
    else
    {
      s += "(" + proto_->query_field_value().ShortDebugString() +
           ") (query_field_type=" + proto_->query_field_type() + ")";
    }
    s += " daps=(";
    for (const std::string &dap_name : dap_names_)
    {
      s += dap_name + ", ";
    }
    if (!dap_names_.empty())
    {
      s.pop_back();
      s.pop_back();
    }
    s += ")";
    s += " mementos=" + std::to_string(mementos_.size());
    return s;
  }

protected:
  std::shared_ptr<ConstructQueryConstraintObjectRequest> proto_;
};
