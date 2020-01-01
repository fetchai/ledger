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
#include <vector>

#include "Leaf.hpp"
#include "Node.hpp"
#include "logging/logging.hpp"
#include "oef-messages/dap_interface.hpp"

class Branch : public Node, public std::enable_shared_from_this<Branch>
{
public:
  static constexpr char const *LOGGING_NAME = "Branch";
  using Node::dap_names_;

  Branch() = default;
  explicit Branch(ConstructQueryObjectRequest const &proto)
    : proto_{std::make_shared<ConstructQueryObjectRequest>(proto)}
  {
    buildTree();
  }
  ~Branch() override          = default;
  Branch(const Branch &other) = delete;
  Branch &operator=(const Branch &other) = delete;

  bool operator==(const Branch &other) = delete;
  bool operator<(const Branch &other)  = delete;

  std::string GetNodeType() override
  {
    return "branch";
  }

  void SetOperator(const std::string &op)
  {
    proto_->set_operator_(op);
  }

  void AddBranch(std::shared_ptr<Branch> branch)
  {
    subnodes_.push_back(std::move(branch));
  }

  std::string ToString() override
  {
    std::string s = "Branch " + proto_->node_name() + " -- ";
    s += "\"" + proto_->operator_() + "\" over daps(";
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
    s += " (" + std::to_string(subnodes_.size()) + " children, " + std::to_string(leaves_.size()) +
         " leaves)";
    s += " (mementos= " + std::to_string(mementos_.size()) + " )";
    return s;
  }

  void Print()
  {
    auto this_pt = shared_from_this();
    Print(this_pt, 0);
  }

  virtual const std::string &GetOperator()
  {
    return proto_->operator_();
  }

  const std::shared_ptr<ConstructQueryObjectRequest> &GetProto() const
  {
    return proto_;
  }

  std::shared_ptr<ConstructQueryObjectRequest> ToProto(const std::string &dap_name)
  {
    auto pt = std::make_shared<ConstructQueryObjectRequest>();
    pt->set_operator_(proto_->operator_());
    pt->set_node_name(proto_->node_name());
    for (const std::string &name : dap_names_)
    {
      pt->add_dap_names(name);
    }
    for (const auto &leaf : leaves_)
    {
      pt->add_constraints()->CopyFrom(*(leaf->ToProto(dap_name)));
    }
    for (const auto &node : subnodes_)
    {
      pt->add_children()->CopyFrom(*(node->ToProto(dap_name)));
    }
    return pt;
  }

  std::vector<std::shared_ptr<Branch>> &GetSubnodes()
  {
    return subnodes_;
  }

  std::vector<std::shared_ptr<Leaf>> &GetLeaves()
  {
    return leaves_;
  }

  void SetName(const std::string &name)
  {
    proto_->set_node_name(name);
  }

  void MergeDaps()
  {
    std::vector<const std::unordered_set<std::string> *> dap_names_list{};
    for (auto &node : subnodes_)
    {
      dap_names_list.push_back(&(node->GetDapNames()));
    }
    for (auto &leaf : leaves_)
    {
      dap_names_list.push_back(&(leaf->GetDapNames()));
    }
    if (dap_names_list.empty())
    {
      return;
    }
    auto first    = dap_names_list[0];
    bool no_match = false;
    for (uint32_t i = 1; i < dap_names_list.size(); ++i)
    {
      if (*first != *(dap_names_list[i]))
      {
        no_match = true;
        break;
      }
    }
    if (!no_match)
    {
      dap_names_.insert(first->begin(), first->end());
      for (auto &node : subnodes_)
      {
        node->ClearDapNames();
      }
      std::string table_name;
      for (auto &leaf : leaves_)
      {
        leaf->ClearDapNames();
        UpdateTargetString(table_name, leaf->GetTargetTableName());
      }
      for (auto &leaf : leaves_)
      {
        leaf->SetTargetTableName(table_name);
      }
    }
  }

protected:
  void UpdateTargetString(std::string &target, const std::string &source)
  {
    if (!source.empty())
    {
      if (target.empty())
      {
        target = source;
      }
      else if (target != source)
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Target table or field name not the same for all merged leaves! ",
                       "Target: '", target, "', Source: '", source, "'");
      }
    }
  }

  void Print(std::shared_ptr<Branch> &root, uint32_t depth = 0)
  {
    FETCH_LOG_INFO(LOGGING_NAME, std::string(depth, ' '), root->ToString());
    for (const auto &leaf : root->leaves_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, std::string(depth + 1, ' '), leaf->ToString());
    }
    for (auto &node : root->subnodes_)
    {
      Print(node, depth + 1);
    }
  }

  void buildTree()
  {
    for (auto &node : proto_->children())
    {
      subnodes_.push_back(std::make_shared<Branch>(node));
    }
    for (auto &leaf : proto_->constraints())
    {
      leaves_.push_back(std::make_shared<Leaf>(leaf));
    }
  }

protected:
  std::shared_ptr<ConstructQueryObjectRequest> proto_;
  std::vector<std::shared_ptr<Branch>>         subnodes_;
  std::vector<std::shared_ptr<Leaf>>           leaves_;

private:
};
