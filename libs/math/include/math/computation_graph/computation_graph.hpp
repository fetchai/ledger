#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>       // std::cout
#include <stack>          // std::stack
#include <deque>          // std::deque
#include <string>         // std::string
#include <memory>

//#include<stdio.h>
//#include<stdlib.h>

namespace fetch {
namespace math {
namespace computation_graph {

namespace helper_funcs{
static bool IsOperator(char &c)
{
  switch (c)
  {
    case '+':
      return true;
    case '-':
      return true;
    case '*':
      return true;
    case '/':
      return true;
    case '%':
      return true;
    default:
      return false;
  }

}

enum OPERATOR_PRECEDENCE
{
//  PARENTHESIS = 10,
  DIVIDE = 9,
  MULTIPLY = 8,
  ADD = 7,
  SUBTRACT = 6,
  DEFAULT = -1
};

static OPERATOR_PRECEDENCE GetPrecedence(char &c)
{
  switch (c)
  {
    case '+':
      return OPERATOR_PRECEDENCE::ADD;
    case '-':
      return OPERATOR_PRECEDENCE::SUBTRACT;
    case '*':
      return OPERATOR_PRECEDENCE::MULTIPLY;
    case '/':
      return OPERATOR_PRECEDENCE::DIVIDE;
//    case '(':
//      return OPERATOR_PRECEDENCE::PARENTHESIS;
//    case ')':
//      return OPERATOR_PRECEDENCE::PARENTHESIS;
    default:
      return OPERATOR_PRECEDENCE::DEFAULT;
  }
}
}



 class ExpressionNode : public std::enable_shared_from_this<ExpressionNode>
{
 public:

  std::string name;

  // if this is a leaf node VAL will be a number, otherwise it will be the operation to apply on the child nodes
  // when we compute the graph VAL will again be replaced by the result of the operation
  union VAL
   {
    char c;             // occupies 1 byte
    double val;         // occupies 8 bytes
   };                      // the whole union occupies 4 bytes

  std::shared_ptr<ExpressionNode> left_node_ptr = nullptr;
  std::shared_ptr<ExpressionNode> right_node_ptr = nullptr;
  std::shared_ptr<ExpressionNode> parent_node_ptr = nullptr;
  bool evaluated = false;
  VAL val;

  ExpressionNode()                    = default;
  ExpressionNode(ExpressionNode &&other)      = default;
  ExpressionNode(double num)
  {
    val.val = num;
  }
  ExpressionNode(char num, std::shared_ptr<ExpressionNode> left_node, std::shared_ptr<ExpressionNode> right_node)
  {
    val.c = num;
    left_node_ptr = std::move(left_node);
    right_node_ptr = std::move(right_node);
  }

  /**
   * inform the child nodes that this node is their parent
   * @return
   */
  void SetChildNodesParent()
  {
    left_node_ptr->parent_node_ptr = shared_from_this();
    right_node_ptr->parent_node_ptr = shared_from_this();
  }
};


//template <typename T>
class ComputationGraph
{

 public:
  using expression_node_type = typename fetch::math::computation_graph::ExpressionNode;
  using expression_node_graph = std::deque<std::shared_ptr<expression_node_type>>;
//  using EXPRESSION_STACK = std::stack<std::shared_ptr<expression_node_type>, expression_container_type>;

  expression_node_graph expression_graph;
  std::stack<char, std::deque<char>> operator_stack;

  ComputationGraph()
  {
  }

  /**
   * converts a string into a graph of expressions to compute
   * @param input
   * @param ret
   */
  void ParseExpression(std::string input)
  {
    char c;

    for (std::size_t i = 0; i < input.size(); ++i)
    {
      c = input[i];

      if (c == ' ')
      {
        // ignore white space
      }
      else if (c == '(')
      {
        operator_stack.push(c);
      }
      else if (isdigit(c))
      {
        expression_graph.emplace_back(std::make_unique<ExpressionNode>(double(c - '0')));
      }
      else if (helper_funcs::IsOperator(c))
      {

        while (!(operator_stack.empty()) && (helper_funcs::GetPrecedence(operator_stack.top()) >= helper_funcs::GetPrecedence(c)))
        {
          std::shared_ptr<ExpressionNode> e2 = std::move(expression_graph.back());
          expression_graph.pop_back();
          std::shared_ptr<ExpressionNode> e1 = std::move(expression_graph.back());
          expression_graph.pop_back();

          expression_graph.emplace_back(std::make_unique<ExpressionNode>(operator_stack.top(), std::move(e1), std::move(e2)));
          operator_stack.pop();

          // instruct the child nodes about their parentage for later traversal
          expression_graph.back()->SetChildNodesParent();
        }

        operator_stack.push(c);
      }
      else if (c == ')')
      {
        while(operator_stack.top() != '(')
        {
          std::shared_ptr<ExpressionNode> e2 = std::move(expression_graph.back());
          expression_graph.pop_back();
          std::shared_ptr<ExpressionNode> e1 = std::move(expression_graph.back());
          expression_graph.pop_back();

          expression_graph.emplace_back(std::make_unique<ExpressionNode>(operator_stack.top(), std::move(e1), std::move(e2)));
          operator_stack.pop();

          // instruct the child nodes about their parentage for later traversal
          expression_graph.back()->SetChildNodesParent();
        }

        // pop the ( off the operator stack
        operator_stack.pop();
      }
      else
      {
        std::cout << " unhandled error. character type: " << c << std::endl;
      }
    }

    while (!(operator_stack.empty()))
    {
      std::shared_ptr<ExpressionNode> e2 = std::move(expression_graph.back());
      expression_graph.pop_back();
      std::shared_ptr<ExpressionNode> e1 = std::move(expression_graph.back());
      expression_graph.pop_back();

      expression_graph.emplace_back(std::make_unique<ExpressionNode>(operator_stack.top(), std::move(e1), std::move(e2)));
      operator_stack.pop();

      // instruct the child nodes about their parentage for later traversal
      expression_graph.back()->SetChildNodesParent();
    }

    return;

  }

  /**
   * compute a single op on the graph
   * @param l
   * @param r
   * @param op
   * @return
   */
  double compute_op(double l, double r, char op)
  {
    switch (op)
    {
      case '+':
        return l + r;
      case '-':
        return l - r;
      case '*':
        return l * r;
      case '/':
        return l / r;
      default:
        std::cout << "should never occur error!!" << std::endl;
        return 0;
    }

  }

  /**
   * Runs the computation graph generated from parsing the input string
   */
//  void Run(std::vector<>)
  void Run(double &ret)
  {
    bool unfinished = true;

    std::shared_ptr<ExpressionNode> cur_node_ptr = expression_graph.front();

    while (unfinished)
    {
      if (cur_node_ptr->left_node_ptr == nullptr && cur_node_ptr->right_node_ptr == nullptr) // ascend - nothing below
      {
//        std::cout << "c: " << cur_node_ptr->val.val << std::endl;
        cur_node_ptr->evaluated = true;
        cur_node_ptr = cur_node_ptr->parent_node_ptr;
      }
      else if (cur_node_ptr->left_node_ptr->evaluated && cur_node_ptr->right_node_ptr->evaluated) // compute and ascend - both evaluated
      {
        // compute the op
        cur_node_ptr->val.val = compute_op(cur_node_ptr->left_node_ptr->val.val, cur_node_ptr->right_node_ptr->val.val, cur_node_ptr->val.c);
        cur_node_ptr->evaluated = true;

        // ascend if not root node
        if (!(cur_node_ptr->parent_node_ptr == nullptr))
        {
          cur_node_ptr = cur_node_ptr->parent_node_ptr;
        }
        else
        {
          unfinished = false;
        }
      }
      else if ((cur_node_ptr->left_node_ptr == nullptr) || (cur_node_ptr->left_node_ptr->evaluated)) // descend right
      {
        cur_node_ptr = cur_node_ptr->right_node_ptr;
      }
      else // descend left
      {
        cur_node_ptr = cur_node_ptr->left_node_ptr;
      }
    }

  }
};



}
}
}
