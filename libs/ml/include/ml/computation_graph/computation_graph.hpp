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

#include <deque>     // std::deque
#include <iostream>  // std::cout
#include <set>       // std::set
#include <stack>     // std::stack
#include <string>    // std::string
#include <vector>    // std::vector

#include <algorithm>  // std::find_if
#include <cassert>
#include <iterator>  // std::istream_iterator
#include <memory>
#include <sstream>  // std::istringstream

#include "math/free_functions/free_functions.hpp"
#include "math/ndarray.hpp"

#include "ml/variable.hpp"
#include <unordered_set>

namespace fetch {
namespace ml {
namespace computation_graph {

template <typename T>
class SessionManager;

namespace helper_funcs {
static inline bool IsOperator(char &c)
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
    //    case '%':
    //      return true;
    //    case '^':
    //      return true;
  default:
    return false;
  }
}

enum OPERATOR_PRECEDENCE
{
  MODULO   = 10,
  POWER    = 9,
  DIVIDE   = 8,
  MULTIPLY = 7,
  ADD      = 6,
  SUBTRACT = 5,
  DEFAULT  = -1
};

static inline OPERATOR_PRECEDENCE GetPrecedence(char &c)
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
  case '%':
    return OPERATOR_PRECEDENCE::MODULO;
  case '^':
    return OPERATOR_PRECEDENCE::POWER;
  default:
    return OPERATOR_PRECEDENCE::DEFAULT;
  }
}
}  // namespace helper_funcs

/**
 * An ExpressionNode is a node in the computation graph. Since the computation graph is always a
 * binary tree, it will hold some value (or operation), two pointers to it's two children (if any)
 * and one for its parent (if any).
 */
template <typename T, typename ARRAY_TYPE>
class ExpressionNode : public std::enable_shared_from_this<ExpressionNode<T, ARRAY_TYPE>>
{
public:
  std::string name;

  // VAL is the value of this node on the graph
  // During parsing val is assigned as either a leaf node (in which case VAL will be a number),
  // otherwise it will be an operation to apply on child nodes where it will be stored as a char
  // During Run the graph will be computed and VAL will be replaced by the result of the operations
  // (as a number)
  union VAL
  {
    char   c;    // occupies 1 byte
    double val;  // occupies 8 bytes
  };             // the whole union occupies 4 bytes

  // VAL might go unused if the node stores an array since that can't easily be put into a VAL
  ARRAY_TYPE array;

  std::shared_ptr<ExpressionNode> left_node_ptr   = nullptr;
  std::shared_ptr<ExpressionNode> right_node_ptr  = nullptr;
  std::shared_ptr<ExpressionNode> parent_node_ptr = nullptr;
  bool                            evaluated       = false;
  VAL                             val;

  ExpressionNode()                       = default;
  ExpressionNode(ExpressionNode &&other) = default;

  ExpressionNode(ARRAY_TYPE arr)
  {
    array = arr;
  }
  ExpressionNode(T num)
  {
    val.val = num;
  }
  ExpressionNode(char num, std::shared_ptr<ExpressionNode> left_node,
                 std::shared_ptr<ExpressionNode> right_node)
  {
    val.c          = num;
    left_node_ptr  = std::move(left_node);
    right_node_ptr = std::move(right_node);
  }

  /**
   * inform the child nodes that this node is their parent
   * @return
   */
  void SetChildNodesParent()
  {
    left_node_ptr->parent_node_ptr  = this->shared_from_this();
    right_node_ptr->parent_node_ptr = this->shared_from_this();
  }
};

template <typename T, typename ARRAY_TYPE>
class ComputationGraph
{
private:
  enum TOKEN_TYPE
  {
    OPEN_PAREN,
    CLOSE_PAREN,
    OPERATOR,
    NUMERIC,
    ALPHA,
    IGNORE_TYPE,
    NONE
  };

  /**
   * checks whether the new token type is different from the previous token type and thus should
   * count as a new token
   * @return
   */
  bool NewTokenType(TOKEN_TYPE &prev_type, TOKEN_TYPE &cur_type)
  {
    bool change_type = false;
    if (prev_type != TOKEN_TYPE::IGNORE_TYPE)
    {
      if ((cur_type != prev_type) ||
          ((prev_type == TOKEN_TYPE::OPERATOR) && (cur_type == TOKEN_TYPE::OPERATOR)))

      //          ((prev_type == TOKEN_TYPE::OPERATOR) && (cur_type == TOKEN_TYPE::OPERATOR)) ||
      //          ((prev_type == TOKEN_TYPE::OPEN_PAREN) && (cur_type == TOKEN_TYPE::OPEN_PAREN)) ||
      //          ((prev_type == TOKEN_TYPE::CLOSE_PAREN) && (cur_type == TOKEN_TYPE::CLOSE_PAREN)))
      {
        change_type = true;
      }
    }
    return change_type;
  }

  /**
   * takes the two most recent numerics from the expression graph and replaces them with one
   * expression combining them together with the most recent operation on the operator stack
   */
  void AssignBinaryTree()
  {
    std::shared_ptr<ExpressionNode<T, ARRAY_TYPE>> e2 = std::move(expression_graph.back());
    expression_graph.pop_back();
    std::shared_ptr<ExpressionNode<T, ARRAY_TYPE>> e1 = std::move(expression_graph.back());
    expression_graph.pop_back();

    expression_graph.emplace_back(std::make_unique<ExpressionNode<T, ARRAY_TYPE>>(
        operator_stack.top(), std::move(e1), std::move(e2)));
    operator_stack.pop();

    // instruct the child nodes about their parentage for later traversal
    expression_graph.back()->SetChildNodesParent();
  }

  bool IsRegistered(std::string name, std::string token)
  {
    if (name == token)
    {
      return true;
    }
    return false;
  }

public:
  using EXPRESSION_NODE_TYPE = typename fetch::ml::computation_graph::ExpressionNode<T, ARRAY_TYPE>;
  using EXPRESSION_NODE_GRAPH = std::deque<std::shared_ptr<EXPRESSION_NODE_TYPE>>;
  using OPERATOR_STACK        = std::stack<char, std::deque<char>>;
  //  using EXPRESSION_STACK = std::stack<std::shared_ptr<expression_node_type>,
  //  expression_container_type>;

  EXPRESSION_NODE_GRAPH expression_graph;
  OPERATOR_STACK        operator_stack;

  std::deque<std::pair<std::string, ARRAY_TYPE>> registered_arrays;

  ComputationGraph()
  {}

  /**
   * Allows reuse of a computation graph by resetting the expression and operator storage
   */
  void Reset()
  {
    EXPRESSION_NODE_GRAPH empty_graph;
    std::swap(expression_graph, empty_graph);

    OPERATOR_STACK empty_operator_stack;
    std::swap(operator_stack, empty_operator_stack);
  }

  /**
   * methods for preparing the computation graph for adding predefined arrays
   * @param input
   * @param name
   */
  void RegisterArray(ARRAY_TYPE input, std::string name)
  {
    registered_arrays.push_back(std::pair<std::string, ARRAY_TYPE>(name, input));
  }

  /**
   * A method for extracting tokens from an input string
   * @param input
   * @param ret
   * @param token_types
   */
  void Tokenize(std::string &input, std::vector<std::string> &ret,
                std::vector<TOKEN_TYPE> &token_types)
  {
    TOKEN_TYPE  prev_type = TOKEN_TYPE::NONE;
    TOKEN_TYPE  cur_type  = TOKEN_TYPE::NONE;
    std::string cur_token;
    for (std::size_t j = 0; j < input.size(); ++j)
    {

      // figure out what type this character is
      if (isalpha(input[j]))
      {
        cur_type = TOKEN_TYPE::ALPHA;
      }
      else if (isdigit(input[j]))
      {
        cur_type = TOKEN_TYPE::NUMERIC;
      }
      else if (input[j] == '(')
      {
        cur_type = TOKEN_TYPE::OPEN_PAREN;
      }
      else if (input[j] == ')')
      {
        cur_type = TOKEN_TYPE::CLOSE_PAREN;
      }
      else if (input[j] == '.')
      {
        cur_type = TOKEN_TYPE::NUMERIC;
      }
      else if (ispunct(input[j]))
      {
        if (helper_funcs::IsOperator(input[j]))
        {
          cur_type = TOKEN_TYPE::OPERATOR;
        }
        else
        {
          cur_type = TOKEN_TYPE::IGNORE_TYPE;
        }
      }
      else if (isspace(input[j]))
      {
        cur_type = TOKEN_TYPE::IGNORE_TYPE;
      }
      else
      {
        cur_type = TOKEN_TYPE::NONE;
        std::cout << "unhandled errror!!!" << std::endl;
      }

      // final character so push previous and then current token as necessary
      if (j == (input.size() - 1))
      {
        if (NewTokenType(prev_type, cur_type))
        {
          token_types.push_back(prev_type);
          ret.push_back(cur_token);
          cur_token = {};
        }

        if (cur_type != TOKEN_TYPE::IGNORE_TYPE)
        {
          token_types.push_back(cur_type);
          cur_token.push_back(input[j]);
          ret.push_back(cur_token);
        }
      }
      else
      {
        // if theres a type change from the previous character
        if ((j > 0) && (NewTokenType(prev_type, cur_type)))
        {
          ret.push_back(cur_token);
          token_types.push_back(prev_type);

          cur_token = {};
        }
        if (cur_type != TOKEN_TYPE::IGNORE_TYPE)
        {
          cur_token.push_back(input[j]);
        }
      }

      prev_type = cur_type;
    }
  }

  /**
   * converts a string into a graph of expressions to compute
   * @param input
   * @param ret
   */
  void ParseExpression(std::string input)
  {
    std::vector<std::string> tokens{};
    std::vector<TOKEN_TYPE>  token_types{};
    Tokenize(input, tokens, token_types);

    //    for (std::size_t i = 0; i < input.size(); ++i)
    std::string open_paren  = ("(");
    std::string close_paren = (")");

    for (std::size_t i = 0; i < tokens.size(); ++i)
    {
      // spaces should have been removed
      assert(token_types[i] != TOKEN_TYPE::IGNORE_TYPE);

      switch (token_types[i])
      {
      case TOKEN_TYPE::OPEN_PAREN:
      {
        operator_stack.push('(');
        break;
      }
      case TOKEN_TYPE::ALPHA:
      {
        std::string cur_token = tokens[i];
        // if token in registered_tokens
        typename std::deque<std::pair<std::string, ARRAY_TYPE>>::iterator it =
            std::find_if(registered_arrays.begin(), registered_arrays.end(),
                         [&cur_token](std::pair<std::string, ARRAY_TYPE> name) {
                           return (name.first == cur_token);
                         });
        if (it != registered_arrays.end())
        {
          expression_graph.emplace_back(
              std::make_unique<ExpressionNode<T, ARRAY_TYPE>>(it->second));
        }
        // TODO(private issue 231)
        break;
      }
      case TOKEN_TYPE::NUMERIC:
      {
        expression_graph.emplace_back(
            std::make_unique<ExpressionNode<T, ARRAY_TYPE>>(std::stod(tokens[i])));
        break;
      }
      case TOKEN_TYPE::OPERATOR:
      {
        assert(helper_funcs::IsOperator(tokens[i][0]));

        // we now have enough information to combined two numerics and one operator into a sub-tree
        while (!(operator_stack.empty()) && (helper_funcs::GetPrecedence(operator_stack.top()) >=
                                             helper_funcs::GetPrecedence(tokens[i][0])))
        {
          AssignBinaryTree();
        }

        operator_stack.push(tokens[i][0]);
        break;
      }
      case TOKEN_TYPE::CLOSE_PAREN:
      {
        // since its a close parenthesis we should set up the sub-tree for operations inside
        while (operator_stack.top() != '(')
        {
          AssignBinaryTree();
        }

        // pop the ( off the operator stack
        operator_stack.pop();
        break;
      }
      case TOKEN_TYPE::NONE:
      {
        std::cout << "NONE token: " << tokens[i] << std::endl;
        break;
      }
      case TOKEN_TYPE::IGNORE_TYPE:
      {
        std::cout << "IGNORE_TYPE token: " << tokens[i] << std::endl;
        break;
      }
      }
    }

    // combine dangling binary trees together
    while (!(operator_stack.empty()))
    {
      if (operator_stack.top() != '(')
      {
        AssignBinaryTree();
      }

      // pop the ( off the operator stack
      if (!(operator_stack.empty()))
      {
        if (operator_stack.top() == '(')
        {
          operator_stack.pop();
        }
      }
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
  template <typename S>
  S compute_op(S l, S r, char op)
  {
    switch (op)
    {
    case '+':
      return fetch::math::Add(l, r);
    case '-':
      return fetch::math::Subtract(l, r);
    case '*':
      return fetch::math::Multiply(l, r);
    case '/':
      return fetch::math::Divide(l, r);
      //      case '%':
      //        return (l % r);
      //      case '^':
      //        return (l ^ r);
    default:
      throw;
    }
  }

  /**
   * Runs the computation graph generated from parsing the input string
   */
  template <typename S>
  void Run(S &ret)
  {
    bool unfinished = true;

    std::shared_ptr<ExpressionNode<T, ARRAY_TYPE>> cur_node_ptr = expression_graph.front();

    while (unfinished)
    {
      if (cur_node_ptr->left_node_ptr == nullptr &&
          cur_node_ptr->right_node_ptr == nullptr)  // ascend - nothing below
      {
        cur_node_ptr->evaluated = true;
        cur_node_ptr            = cur_node_ptr->parent_node_ptr;
      }
      else if (cur_node_ptr->left_node_ptr->evaluated &&
               cur_node_ptr->right_node_ptr->evaluated)  // compute and ascend - both evaluated
      {
        // compute the op
        cur_node_ptr->val.val =
            compute_op(cur_node_ptr->left_node_ptr->val.val, cur_node_ptr->right_node_ptr->val.val,
                       cur_node_ptr->val.c);
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
      else if ((cur_node_ptr->left_node_ptr == nullptr) ||
               (cur_node_ptr->left_node_ptr->evaluated))  // descend right
      {
        cur_node_ptr = cur_node_ptr->right_node_ptr;
      }
      else  // descend left
      {
        cur_node_ptr = cur_node_ptr->left_node_ptr;
      }
    }

    ret = cur_node_ptr->val.val;
  }

  /**
   * Specialization for NDArrays
   * @param ret
   */
  void Run(math::NDArray<T> &ret)
  {
    bool unfinished = true;

    std::shared_ptr<ExpressionNode<T, ARRAY_TYPE>> cur_node_ptr = expression_graph.front();

    while (unfinished)
    {
      if (cur_node_ptr->left_node_ptr == nullptr &&
          cur_node_ptr->right_node_ptr == nullptr)  // ascend - nothing below
      {
        cur_node_ptr->evaluated = true;
        cur_node_ptr            = cur_node_ptr->parent_node_ptr;
      }
      else if (cur_node_ptr->left_node_ptr->evaluated &&
               cur_node_ptr->right_node_ptr->evaluated)  // compute and ascend - both evaluated
      {
        // compute the op
        cur_node_ptr->array     = compute_op(cur_node_ptr->left_node_ptr->array,
                                         cur_node_ptr->right_node_ptr->array, cur_node_ptr->val.c);
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
      else if ((cur_node_ptr->left_node_ptr == nullptr) ||
               (cur_node_ptr->left_node_ptr->evaluated))  // descend right
      {
        cur_node_ptr = cur_node_ptr->right_node_ptr;
      }
      else  // descend left
      {
        cur_node_ptr = cur_node_ptr->left_node_ptr;
      }
    }

    ret = cur_node_ptr->array;
  }
};

}  // namespace computation_graph
}  // namespace ml
}  // namespace fetch
