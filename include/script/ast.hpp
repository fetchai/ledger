#ifndef SCRIPT_AST_HPP
#define SCRIPT_AST_HPP

#include "byte_array/tokenizer/token.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <memory>
#include <vector>
namespace fetch {
namespace script {

enum ASTProperty {
  TOKEN = 1,
  OP_RIGHT = 2,
  OP_LEFT = 4,
  //  OP_LEFT_LEFT = ,
  //  OP_RIGHT_RIGHT = ,
  GROUP = 16,
  GROUP_OPEN = 32,
  GROUP_CLOSE = 64
};

struct ASTNode;

struct ASTOperationType {
  typedef std::shared_ptr<ASTNode> ast_node_ptr;

  typedef std::vector<ast_node_ptr> node_ptr_list;
  //  typedef std::function< bool(node_ptr_list const &nodes, std::size_t const
  //  &i) > qualify_function_type;
  uint16_t type = uint16_t(-1);
  uint16_t properties = 0;
  uint16_t precendence = 0;
  //  qualify_function_type test = [](std::vector< ast_node_ptr > const &,
  //  std::size_t const&) { return true; };
  uint16_t next = uint16_t(-1);

  ASTOperationType(uint16_t t = uint16_t(-1), uint16_t pro = 0,
                   uint16_t pre = 0, uint16_t n = uint16_t(-1))
      : type(t), properties(pro), precendence(pre), next(n) {}

  bool operator<(ASTOperationType const &other) const {
    return precendence < other.precendence;
  }
};

struct ASTGroupOperationType : ASTOperationType {
  typedef std::shared_ptr<ASTNode> ast_node_ptr;
  ASTGroupOperationType(uint16_t t = uint16_t(-1), uint16_t pro = 0,
                        uint16_t pre = 0, uint16_t n = uint16_t(-1))
      : ASTOperationType(t, pro | ASTProperty::GROUP, pre, n),
        open_(t, pro | ASTProperty::GROUP | ASTProperty::GROUP_OPEN, pre, n),
        close_(t, pro | ASTProperty::GROUP | ASTProperty::GROUP_CLOSE, pre, n) {
  }

  ASTOperationType const &open() const { return open_; }
  ASTOperationType const &close() const { return close_; }

 private:
  ASTOperationType open_, close_;
};

struct ASTNode {
  typedef std::shared_ptr<ASTNode> ast_node_ptr;
  typedef byte_array::Token byte_array_type;
  ASTNode(ASTOperationType const &t, byte_array_type const &s)
      : token_class(t), symbol(s) {
    children.clear();
  }

  ASTNode(ASTNode const &other)
      : token_class(other.token_class),
        symbol(other.symbol),
        children(other.children) {}

  ~ASTNode() = default;

  std::size_t count() const { return children.size(); }

  ASTOperationType token_class;
  byte_array_type symbol;
  std::vector<ast_node_ptr> children;  // TODO: Use shared pointer
};

class AbstractSyntaxTree {
 public:
  typedef std::shared_ptr<ASTNode> ast_node_ptr;
  typedef ASTNode node_type;
  //  typedef ASTToken token_type;
  typedef byte_array::Token byte_array_type;

  ~AbstractSyntaxTree() {}

  ASTGroupOperationType AddGroup(uint16_t type, uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTGroupOperationType ret = {type, ASTProperty::GROUP, prec, next};
    token_types_.push_back(ret);
    StoreGroup(ret);
    return ret;
  }

  ASTOperationType AddLeft(uint16_t type, uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTOperationType ret = {type, ASTProperty::OP_LEFT, prec, next};
    token_types_.push_back(ret);
    StoreOperation(ret);
    return ret;
  }

  ASTOperationType AddToken(uint16_t type, uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTOperationType ret = {type, ASTProperty::TOKEN, prec, next};
    token_types_.push_back(ret);
    StoreOperation(ret);
    return ret;
  }

  ASTOperationType AddRight(uint16_t type, uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTOperationType ret = {type, ASTProperty::OP_RIGHT, prec, next};
    token_types_.push_back(ret);
    StoreOperation(ret);
    return ret;
  }

  ASTOperationType AddLeftRight(uint16_t type, uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTOperationType ret = {type, ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT,
                            prec, next};
    token_types_.push_back(ret);
    StoreOperation(ret);
    return ret;
  }

  ASTOperationType AddOperation(uint16_t type, uint16_t properties,
                                uint16_t next = uint16_t(-1)) {
    uint16_t prec = token_types_.size();
    ASTOperationType ret = {type, properties, prec, next};
    token_types_.push_back(ret);
    StoreOperation(ret);
    return ret;
  }

  //  typedef std::function< bool(byte_array_type const &, std::size_t &) >
  //  function_type;
  void PushTokenType(ASTOperationType const &type) {
    token_types_.push_back(type);
  }

  void Clear() { tree_.clear(); }

  void PushToken(node_type const &v) {
    tree_.push_back(std::make_shared<node_type>(v));
  }

  void Push(uint16_t op, byte_array_type const &t) {
    if (operations_.find(op) == operations_.end()) {
      std::cerr << "error, could not find type I, ast.hpp" << std::endl;
      exit(-1);
    }
    node_type n = {operations_[op], t};
    tree_.push_back(std::make_shared<node_type>(n));
  }

  void PushOpen(uint16_t op, byte_array_type const &t) {
    if (groups_.find(op) == groups_.end()) {
      std::cerr << "error, could not find type II, ast.hpp" << std::endl;
      exit(-1);
    }
    node_type n = {groups_[op].open(), t};
    tree_.push_back(std::make_shared<node_type>(n));
  }

  void PushClose(uint16_t op, byte_array_type const &t) {
    if (groups_.find(op) == groups_.end()) {
      std::cerr << "error, could not find type III, ast.hpp" << std::endl;
      exit(-1);
    }
    node_type n = {groups_[op].close(), t};
    tree_.push_back(std::make_shared<node_type>(n));
  }

  void Build() {
    std::sort(token_types_.begin(), token_types_.end());
    std::size_t i = 1;
    for (auto &tt : token_types_) {
      if (tt.next == uint16_t(-1)) {
        tt.next = i;  //( (tt.properties & GROUP) ? 0 : i);
      }
      ++i;
    }

    BuildSubset(0, tree_);

    if (tree_.size() != 1) {
      std::cerr << "Tree does not have exactly one root: " << tree_.size()
                << std::endl;
      for (auto &t : tree_) {
        std::cout << t << " ";
      }
      exit(-1);
    }

    root_ = tree_[0];
  }

  std::vector<ast_node_ptr> const &tree() const { return tree_; };
  node_type const &root() const { return *root_; }
  node_type &root() { return *root_; }

  ast_node_ptr const &root_shared_pointer() const { return root_; }
  ast_node_ptr &root_shared_pointer() { return root_; }

 private:
  void StoreOperation(ASTOperationType const &ret) {
    if (operations_.find(ret.type) != operations_.end()) {
      std::cerr << "error, type already exists, ast.hpp" << std::endl;
      exit(-1);
    }
    operations_[ret.type] = ret;
  }

  void StoreGroup(ASTGroupOperationType const &ret) {
    if (groups_.find(ret.type) != groups_.end()) {
      std::cerr << "error, type already exists, ast.hpp" << std::endl;
      exit(-1);
    }

    groups_[ret.type] = ret;
  }

  void BuildSubset(uint16_t n, std::vector<ast_node_ptr> &nodes) {
    /*
      TODO: Make it possible to inject modifiers here
      std::cout << "Building " << n << ": ";
      for(auto &n : nodes) {
      std::cout << n->symbol <<  " (" << n->token_class.type << ") ";
      }
      std::cout << std::endl;
    */
    std::size_t i = 0;
    auto &token_type = token_types_[n];
    uint16_t type = token_type.type;

    for (; i < nodes.size(); ++i) {
      if (!nodes[i]) {
        std::cerr << "internal error - null pointer in nodes" << std::endl;
        exit(-1);
      }

      auto &node = *nodes[i];
      auto &op = node.token_class;

      if (node.token_class.type == type) {
        if (op.properties & ASTProperty::GROUP) {
          // Groups
          if (op.properties & ASTProperty::GROUP_CLOSE) {
            std::cerr << "Cannot close unopened group: " << node.symbol
                      << std::endl;
            exit(-1);
          }

          if ((op.properties & ASTProperty::GROUP_OPEN) == 0) {
            std::cerr << "Expected group opening: " << op.properties
                      << std::endl;
            exit(-1);
          }

          std::size_t j = i + 1;
          int nbrac = 1;
          while ((j < nodes.size()) && (nbrac != 0)) {
            if (nodes[j]->token_class.type == type) {
              if ((nodes[j]->token_class.properties &
                   ASTProperty::GROUP_CLOSE) != 0)
                --nbrac;
              if ((nodes[j]->token_class.properties &
                   ASTProperty::GROUP_OPEN) != 0)
                ++nbrac;
            }
            ++j;
          }

          if (nbrac != 0) {
            std::cerr << "Could not find group closing:" << std::endl;
            exit(-1);
          }

          node.symbol = "{ ... }";  // TODO: Make reference over span
          ++i;
          --j;
          while (i < j) {
            node.children.push_back(nodes[i]);
            nodes.erase(nodes.begin() + i);
            --j;
          }
          nodes.erase(nodes.begin() + i);  // Group closing
          --i;

          // Building subset for next token type
          BuildSubset(0, node.children);  // TODO: define first

        } else {
          if (op.properties & ASTProperty::OP_LEFT) {
            if (i == 0) {
              std::cerr << "Cannot consume left" << std::endl;
              exit(-1);
            }
            node.children.push_back(nodes[i - 1]);
            nodes.erase(nodes.begin() + i - 1);
            --i;
          }

          if (op.properties & ASTProperty::OP_RIGHT) {
            if ((i + 1) >= nodes.size()) {
              std::cerr << "Cannot consume right" << std::endl;
              exit(-1);
            }
            node.children.push_back(nodes[i + 1]);
            nodes.erase(nodes.begin() + i + 1);
          }
        }
      }
    }

    if ((token_type.next < token_types_.size()) &&
        (token_type.next != uint16_t(-1))) {
      BuildSubset(token_type.next, nodes);
    }

    /*
      TODO: Make it possible to inject modifiers
      std::cout << "After " << n << " / " << nodes.size() << ": ";
      for(auto &n : nodes) {
      std::cout << n->symbol << " (" << n->token_class.type << ") " ;
      }
      std::cout << std::endl;
    */
  }

  ast_node_ptr root_;
  std::vector<ast_node_ptr> tree_;
  std::vector<ASTOperationType> token_types_;
  std::map<uint64_t, ASTOperationType> operations_;
  std::map<uint64_t, ASTGroupOperationType> groups_;
};
};
};

#endif
