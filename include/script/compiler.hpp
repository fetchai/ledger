#ifndef SCRIPT_COMPILER_HPP
#define SCRIPT_COMPILER_HPP

#include "byte_array/tokenizer/tokenizer.hpp"
#include "script/ast.hpp"
#include "script/function.hpp"

namespace fetch {
namespace script {
// FIXME: Group must be able to offset to a subset of the presendence list
struct TokenMatch {
  typedef fetch::byte_array::ReferencedByteArray byte_array_type;
  typedef std::function<bool(byte_array_type const &, std::size_t &)>
      consumer_type;
  typedef std::function<bool(byte_array::Token const &)> accept_if_type;
  enum { OPERATOR = 1, CONSUMER = 2, GROUP = 4 };

  TokenMatch(uint16_t op, byte_array_type open, byte_array_type close)
      : type(GROUP), operation(op), first(open), second(close) {}

  TokenMatch(uint16_t op, byte_array_type symbol, uint64_t cons)
      : type(OPERATOR), operation(op), first(symbol), consumption(cons) {}

  TokenMatch(uint16_t op, consumer_type cons,
             accept_if_type accept_test =
                 [](byte_array::Token const &) { return true; })
      : type(CONSUMER), operation(op), consumer(cons), accept_if(accept_test) {}

  uint16_t type;
  uint16_t operation;
  byte_array_type first, second;

  consumer_type consumer;
  accept_if_type accept_if;

  std::function<bool(ASTToken *)> qualifier;
  uint64_t consumption;
};

template <typename L>
class FunctionCompiler {
 public:
  typedef L language_type;
  typedef fetch::byte_array::ReferencedByteArray byte_array_type;
  typedef fetch::byte_array::Tokenizer tokenizer_type;
  enum {
    TOK_TOKEN = 0,
    TOK_NUMBER = 1,
    TOK_BYTE_ARRAY = 2,
    TOK_OPERATOR = 300000,
    TOK_WHITESPACE = 4,
    TOK_CATCH_ALL = 5
  };

  FunctionCompiler() {
    // Creating a operator
    std::vector<byte_array_type> ops;
    for (auto const &l : language_type::language) {
      if (l.type == TokenMatch::OPERATOR)
        ops.push_back(l.first);
      else if (l.type == TokenMatch::GROUP) {
        ops.push_back(l.first);
        ops.push_back(l.second);
      }
    }

    // Ensuring to consume largest operators first
    std::sort(ops.begin(), ops.end(),
              [](byte_array_type const &a, byte_array_type const &b) -> bool {
                return a.size() > b.size();
              });

    // Adding consumer
    tokenizer_.AddConsumer(TOK_OPERATOR, [ops](byte_array_type const &str,
                                               std::size_t &pos) -> bool {
      for (auto &op : ops)
        if (str.Match(op, pos)) {
          pos += op.size();
          return true;
        }
      return false;
    });

    // Adding remaining consumers
    for (auto const &l : language_type::language) {
      if (l.type == TokenMatch::CONSUMER)
        tokenizer_.AddConsumer(l.operation, l.consumer);
    }
  }

  ASTOperationType LanguageTokenToASTToken(TokenMatch const &l,
                                           uint16_t const &precendence,
                                           uint16_t const &type_add = 0) {
    uint16_t type = 0;
    bool group = false;
    switch (l.type) {
      case TokenMatch::OPERATOR:
        if ((l.consumption & (language_type::LEFT)) &&
            (l.consumption & (language_type::RIGHT)))
          type = ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT;
        else if (l.consumption & (language_type::LEFT))
          type = ASTProperty::OP_LEFT;
        else if (l.consumption & (language_type::RIGHT))
          type = ASTProperty::OP_RIGHT;
        break;
      case TokenMatch::CONSUMER:
        type = ASTProperty::TOKEN;
        break;
      case TokenMatch::GROUP:
        type = ASTProperty::GROUP;
        group = true;
        break;
    }
    return ASTOperationType{l.operation, uint16_t(type | type_add),
                            precendence};
  }

  Function<language_type> operator()(byte_array_type const &file,
                                     byte_array_type const &code) {
    tokenizer_.Parse(file, code);
    std::size_t precendence = 0;
    for (auto const &l : language_type::language) {
      auto def = LanguageTokenToASTToken(l, precendence);
      tree_.PushTokenType(def);
      std::cout << def.type << " - " << def.properties << " - "
                << def.precendence << std::endl;
      ++precendence;
    }

    for (auto &t : tokenizer_) {
      if (t.type() == TOK_OPERATOR) {
        std::size_t precendence = 0;
        for (auto const &l : language_type::language) {
          if ((t == l.first) || (t == l.second)) {
            uint64_t extra = 0;
            if ((l.type == TokenMatch::GROUP) && (t == l.first))
              extra = ASTProperty::GROUP_OPEN;
            if ((l.type == TokenMatch::GROUP) && (t == l.second))
              extra = ASTProperty::GROUP_CLOSE;
            auto token = LanguageTokenToASTToken(l, precendence, extra);
            std::cout << t << " " << token.properties << " " << precendence
                      << " -- " << extra << std::endl;
            assert(token.type & ASTProperty::GROUP);
            tree_.PushToken({token, t});
            break;
          }
          ++precendence;
        }
      } else {
        std::size_t precendence = 0;
        for (auto const &l : language_type::language) {
          if (t.type() == l.operation) {
            if ((l.accept_if) && (!l.accept_if(t))) {
              std::cout << "Skipping: " << t << " " << precendence << std::endl;
              break;
            }
            auto token = LanguageTokenToASTToken(l, precendence);
            tree_.PushToken({token, t});
            std::cout << t << " " << token.type << " " << precendence
                      << std::endl;
            break;
          }
          ++precendence;
        }
      }
    }
    //    exit(-1);
    tree_.Build();

    Function<language_type> fnc;
    Visit(fnc, tree_.root());

    return fnc;
  }

 private:
  void Visit(Function<language_type> &fnc, ASTNode *node) {
    if (node == nullptr) {
      std::cout << "Node is null - TODO throw error" << std::endl;
      return;
    }

    for (std::size_t i = 0; i < node->count; ++i) Visit(fnc, node->children[i]);

    FunctionOperation<typename language_type::register_type> op;
    op.type = node->type;
    op.symbol = node->symbol;
    // FIXME: parse symbol
    fnc.operations_.push_back(op);
  };

  tokenizer_type tokenizer_;
  AbstractSyntaxTree tree_;
};
};
};

#endif
