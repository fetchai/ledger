#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "vm/node.hpp"
#include "vm/ir.hpp"

namespace fetch {
namespace vm {

struct IRBuilder
{
  IRBuilder()
  {
    ir_ = nullptr;
  }

  ~IRBuilder() = default;

  void Build(std::string const &name, BlockNodePtr const &root, IR &ir);
  IRNodePtr BuildNode(NodePtr const &node);
  IRNodePtrArray BuildChildren(NodePtrArray const &children);
  IRTypePtr BuildType(TypePtr const &type);
  IRVariablePtr BuildVariable(VariablePtr const &variable);
  IRFunctionPtr BuildFunction(FunctionPtr const &function);
  IRTypePtrArray BuildTypes(const TypePtrArray &types);
  IRVariablePtrArray BuildVariables(const VariablePtrArray &variables);

  IR *ir_;
  IR::Map<TypePtr, IRTypePtr> type_map_;
  IR::Map<VariablePtr, IRVariablePtr> variable_map_;
  IR::Map<FunctionPtr, IRFunctionPtr> function_map_;
};

}  // namespace vm
}  // namespace fetch
