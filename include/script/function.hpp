#ifndef SCRIPT_MATCHING_FUNCTION_HPP
#define SCRIPT_MATCHING_FUNCTION_HPP
#include <vector>
#include "byte_array/consumers.hpp"
#include "byte_array/tokenizer/tokenizer.hpp"
#include "script/ast.hpp"
#include "script/variant.hpp"

namespace fetch {
namespace script {

class Function {
 public:
  typedef byte_array::Token byte_array_type;
  typedef script::Variant register_type;

  enum FunctionOperation {
    FO_SEQUENCE = 100,
    FO_STATEMENT,
    FO_VARIANT_CONSTANT,
    FO_VARIANT,
    FO_ADD,
    FO_SUBTRACT,
    FO_MULTIPLICATION,
    FO_DIVISION,
    FO_REMAINDER,
    FO_LOGIC_AND,
    FO_LOGIC_OR,
    FO_LOGIC_NOT,
    FO_BITWISE_AND,
    FO_BITWISE_OR,
    FO_BITWISE_XOR,
    FO_BITWISE_NEG,
    FO_ASSIGN,
    FO_ASSIGN_ADD,
    FO_ASSIGN_SUBTRACT,
    FO_ASSIGN_DIVIDE,
    FO_ASSIGN_MULTIPLICATION,

    FO_LT,
    FO_LTE,
    FO_GTE,
    FO_GT,
    FO_EQUAL,
    FO_NOT_EQUAL,

    FO_MEMBER_ACCESS,
    FO_CONTEXT,
    FO_PARENTHESIS,
    FO_INDEX
  };

  enum Type {
    F_CONTEXT_START = 100,
    F_CONTEXT_END = 101,
    F_KEYWORD = 103,
    F_TOKEN = 104,
    F_BYTE_ARRAY = 105,
    F_INTEGER = 106,
    F_FLOAT = 107,
    F_SYNTAX = 504,
    F_OPERATOR = 505,
    F_WHITESPACE = 506,
    F_CATCH_ALL = 507
  };

  struct Operation {
    uint64_t operation;
    byte_array_type symbol;
  };

  static void ConfigureTokenizer(byte_array::Tokenizer &tokenizer,
                                 byte_array_type const &space = "") {
    //    if(space != "") {
    tokenizer.CreateSubspace(space, Function::F_CONTEXT_START, "{",
                             Function::F_CONTEXT_END, "}");
    /*
      } else {
      tokenizer.AddConsumer(Function::F_CONTEXT_START, Keyword("{"));
      tokenizer.AddConsumer(Function::F_CONTEXT_END, Keyword("}"));
      }
    */
    // Function language
    std::vector<std::string> syntax_symbols = {".", "(", ")", "[",
                                               "]", ",", ":", ";"};

    std::vector<std::string> operators = {
        "==", "!=", "<=", ">=", "+=", "-=", "/=", "*=", "|=",
        "&=", "^=", "&&", "||", "|",  "&",  "^",  "~",  "=",
        "!",  "<",  ">",  "+",  "-",  "/",  "*",  "?",  "%"};

    tokenizer.AddConsumer(Function::F_KEYWORD,
                          byte_array::consumers::TokenFromList(
                              {"return", "var", "let", "null", "undefined"}),
                          space);
    tokenizer.AddConsumer(Function::F_INTEGER, byte_array::consumers::Integer,
                          space);
    tokenizer.AddConsumer(Function::F_FLOAT, byte_array::consumers::Float,
                          space);
    tokenizer.AddConsumer(Function::F_TOKEN,
                          byte_array::consumers::AlphaNumericLetterFirst,
                          space);
    tokenizer.AddConsumer(Function::F_WHITESPACE,
                          byte_array::consumers::Whitespace, space);
    tokenizer.AddConsumer(Function::F_BYTE_ARRAY,
                          byte_array::consumers::StringEnclosedIn('"'), space);
    tokenizer.AddConsumer(Function::F_SYNTAX,
                          byte_array::consumers::TokenFromList(syntax_symbols),
                          space);
    tokenizer.AddConsumer(Function::F_OPERATOR,
                          byte_array::consumers::TokenFromList(operators),
                          space);
    tokenizer.AddConsumer(Function::F_CATCH_ALL, byte_array::consumers::AnyChar,
                          space);
  }

  static void ConfigureAST(script::AbstractSyntaxTree &tree) {
    tree.AddGroup(Function::FO_CONTEXT);
    tree.AddGroup(Function::FO_PARENTHESIS);
    tree.AddGroup(Function::FO_INDEX);
    tree.AddLeftRight(Function::FO_MEMBER_ACCESS);

    tree.AddRight(Function::FO_LOGIC_NOT);
    tree.AddRight(Function::FO_BITWISE_NEG);

    tree.AddLeftRight(Function::FO_MULTIPLICATION);
    tree.AddLeftRight(Function::FO_DIVISION);
    tree.AddLeftRight(Function::FO_REMAINDER);

    tree.AddLeftRight(Function::FO_ADD);
    tree.AddLeftRight(Function::FO_SUBTRACT);

    tree.AddLeftRight(Function::FO_LT);
    tree.AddLeftRight(Function::FO_GT);
    tree.AddLeftRight(Function::FO_LTE);
    tree.AddLeftRight(Function::FO_GTE);

    tree.AddLeftRight(Function::FO_EQUAL);
    tree.AddLeftRight(Function::FO_NOT_EQUAL);
    ;

    tree.AddLeftRight(Function::FO_BITWISE_AND);
    tree.AddLeftRight(Function::FO_BITWISE_XOR);
    tree.AddLeftRight(Function::FO_BITWISE_OR);

    tree.AddLeftRight(Function::FO_LOGIC_AND);
    tree.AddLeftRight(Function::FO_LOGIC_OR);

    tree.AddLeftRight(Function::FO_ASSIGN);
    tree.AddLeftRight(Function::FO_ASSIGN_ADD);
    tree.AddLeftRight(Function::FO_ASSIGN_SUBTRACT);
    tree.AddLeftRight(Function::FO_ASSIGN_DIVIDE);
    tree.AddLeftRight(Function::FO_ASSIGN_MULTIPLICATION);

    tree.AddLeftRight(Function::FO_SEQUENCE);

    tree.AddToken(Function::FO_VARIANT);
    tree.AddToken(Function::FO_VARIANT_CONSTANT);

    tree.AddLeft(Function::FO_STATEMENT);
  }

  static void BuildFunctionSubtree(byte_array::Tokenizer &tokenizer,
                                   script::AbstractSyntaxTree &tree,
                                   std::size_t &i) {
    tree.PushOpen(Function::FO_CONTEXT, tokenizer[i]);
    ++i;
    BuildFunctionTree(tokenizer, tree, i, 1);
  }

  static void BuildFunctionTree(byte_array::Tokenizer &tokenizer,
                                script::AbstractSyntaxTree &tree,
                                std::size_t &i,
                                std::size_t context_parans = 1) {
    while ((i < tokenizer.size()) && (context_parans != 0)) {
      auto const &t = tokenizer[i];

      switch (t.type()) {
        case Function::F_CONTEXT_START:
          tree.PushOpen(Function::FO_CONTEXT, t);

          ++context_parans;
          break;
        case Function::F_CONTEXT_END:
          tree.PushClose(Function::FO_CONTEXT, t);
          //        tree.PushToken( {fnc_context_.close(), t } );
          --context_parans;
          break;
        case Function::F_KEYWORD:
          if (t == "var") {
            std::cout << "deep declaration" << std::endl;
          } else if (t == "let") {
            std::cout << "light declaration" << std::endl;
          }
          break;
        case Function::F_TOKEN:
          tree.Push(Function::FO_VARIANT, t);
          //        tree.PushToken( {fnc_variant_, t } );
          break;
        case Function::F_FLOAT:
        case Function::F_INTEGER:
        case Function::F_BYTE_ARRAY:
          tree.Push(Function::FO_VARIANT_CONSTANT, t);
          //        tree.PushToken( {fnc_variant_constant_, t } );
          break;
        case Function::F_SYNTAX:
          switch (t[0]) {
            case '.':
              tree.Push(Function::FO_MEMBER_ACCESS, t);
              //          tree.PushToken( {fnc_member_access_, t } );
              break;
            case '(':
              tree.PushOpen(Function::FO_PARENTHESIS, t);
              //          tree.PushToken( {fnc_paranthesis_.open(), t } );
              break;
            case ')':
              tree.PushClose(Function::FO_PARENTHESIS, t);
              //          tree.PushToken( {fnc_paranthesis_.close(), t } );
              break;
            case '[':
              tree.PushOpen(Function::FO_INDEX, t);
              break;
            case ']':
              tree.PushClose(Function::FO_INDEX, t);
              break;
            case ',':
              tree.Push(Function::FO_SEQUENCE, t);
              //          tree.PushToken( {fnc_sequence_, t } );
              break;
            case ';':
              tree.Push(Function::FO_STATEMENT, t);
              //          tree.PushToken( {fnc_statement_, t } );
              break;
          }
          break;
        case Function::F_OPERATOR:
          if (t.size() == 1) {
            switch (t[0]) {
              case '|':
                tree.Push(Function::FO_BITWISE_OR, t);
                //            tree.PushToken( {fnc_bitwise_or_, t } );
                break;
              case '&':
                tree.Push(Function::FO_BITWISE_AND, t);
                //            tree.PushToken( {fnc_bitwise_and_, t } );
                break;
              case '^':

                tree.Push(Function::FO_BITWISE_XOR, t);
                //            tree.PushToken( {fnc_bitwise_xor_, t } );
                break;
              case '~':
                tree.Push(Function::FO_BITWISE_NEG, t);
                //            tree.PushToken( {fnc_bitwise_neg_, t } );
                break;
              case '=':
                tree.Push(Function::FO_ASSIGN, t);
                //            tree.PushToken( {fnc_assign_, t } );
                break;
              case '!':
                tree.Push(Function::FO_LOGIC_NOT, t);
                //            tree.PushToken( {fnc_logic_not_, t } );
                break;
              case '<':
                tree.Push(Function::FO_LT, t);
                //            tree.PushToken( {fnc_lt_, t } );
                break;
              case '>':
                tree.Push(Function::FO_GT, t);
                //            tree.PushToken( {fnc_gt_, t } );
                break;
              case '+':
                tree.Push(Function::FO_ADD, t);
                //            tree.PushToken( {fnc_add_, t } );
                break;
              case '-':
                tree.Push(Function::FO_SUBTRACT, t);
                //            tree.PushToken( {fnc_subtract_, t } );
                break;
              case '/':
                tree.Push(Function::FO_DIVISION, t);
                //            tree.PushToken( {fnc_division_, t } );
                break;
              case '*':
                tree.Push(Function::FO_MULTIPLICATION, t);
                //            tree.PushToken( {fnc_multiplication_, t } );
                break;
              case '%':
                tree.Push(Function::FO_REMAINDER, t);
                //            tree.PushToken( {fnc_remainder_, t } );
                break;
              case '?':
              default:
                std::cerr << "Did not implement support for " << t << std::endl;
                exit(-1);
            }
          } else if (t.size() == 2) {
            if (t[1] == '=') {
              switch (t[0]) {
                case '=':
                  tree.Push(Function::FO_EQUAL, t);
                  //              tree.PushToken( {fnc_equal_, t } );
                  break;
                case '!':
                  tree.Push(Function::FO_NOT_EQUAL, t);
                  //              tree.PushToken( {fnc_not_equal_, t } );
                  break;
                case '<':
                  tree.Push(Function::FO_LTE, t);
                  //              tree.PushToken( {fnc_lte_, t } );
                  break;
                case '>':
                  tree.Push(Function::FO_GTE, t);
                  //              tree.PushToken( {fnc_gte_, t } );
                  break;
                case '+':
                  tree.Push(Function::FO_ASSIGN_ADD, t);
                  //              tree.PushToken( {fnc_assign_add_, t } );
                  break;
                case '-':
                  tree.Push(Function::FO_ASSIGN_SUBTRACT, t);
                  //              tree.PushToken( {fnc_assign_subtract_, t } );
                  break;
                case '/':
                  tree.Push(Function::FO_ASSIGN_DIVIDE, t);
                  //              tree.PushToken( {fnc_assign_divide_, t } );
                  break;
                case '*':
                  tree.Push(Function::FO_ASSIGN_MULTIPLICATION, t);
                  //              tree.PushToken( {fnc_assign_multiplication_, t
                  //              } );
                  break;
                case '%':
                //              tree.Push( Function::FO_ASSIGN_REMAINDER, t );

                // break;
                default:
                  std::cerr << "Did not implement support (II) for " << t
                            << std::endl;
                  exit(-1);
              };
            }
          } else {
            std::cerr << "unexpected operator with more than 2 chars"
                      << std::endl;
            exit(-1);
          }
          break;
        case Function::F_WHITESPACE:
          break;
        case Function::F_CATCH_ALL:
          break;
      }

      ++i;
    }
  }

  Function() {}

  script::Variant operator()() {
    for (auto &t : operations_) {
      //      std::size_t last =stack_.size() -1;
      switch (t.operation) {
        case FO_VARIANT_CONSTANT:
          std::cout << "Constant: " << t.symbol << " " << t.symbol.type() << " "
                    << t.operation << std::endl;
          break;
        case FO_VARIANT:
          std::cout << "Variant" << std::endl;
          break;
        case FO_CONTEXT:
          break;
        case FO_PARENTHESIS:
          break;
        case FO_INDEX:
          break;
        case FO_ADD:
          std::cout << "ADDING" << std::endl;
          break;
        case FO_SUBTRACT:
          break;
        case FO_MULTIPLICATION:
          break;
        case FO_DIVISION:
          break;
        case FO_REMAINDER:
          break;
        case FO_LOGIC_AND:
          break;
        case FO_LOGIC_OR:
          break;
        case FO_LOGIC_NOT:
          break;
        case FO_BITWISE_AND:
          break;
        case FO_BITWISE_OR:
          break;
        case FO_BITWISE_XOR:
          break;
        case FO_BITWISE_NEG:
          break;
        case FO_ASSIGN:
          break;
        case FO_ASSIGN_ADD:
          break;
        case FO_ASSIGN_SUBTRACT:
          break;
        case FO_ASSIGN_DIVIDE:
          break;
        case FO_ASSIGN_MULTIPLICATION:
          break;
        case FO_LT:
          break;
        case FO_LTE:
          break;
        case FO_GTE:
          break;
        case FO_GT:
          break;
        case FO_EQUAL:
          break;
        case FO_NOT_EQUAL:
          break;
        case FO_MEMBER_ACCESS:
          break;
        case FO_SEQUENCE:
          break;
        case FO_STATEMENT:
          break;
      }
    }
    return script::Variant();
  }

  void PushOperation(Operation op) { operations_.push_back(op); }

  void Clear() { operations_.clear(); }

  std::size_t size() const { return operations_.size(); }

 public:
  std::vector<Operation> operations_;
  std::vector<register_type> stack_;
  std::vector<script::Variant> context_;
};
};
};

#endif
