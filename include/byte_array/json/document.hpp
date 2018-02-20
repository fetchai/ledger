#ifndef BYTE_ARRAY_JSON_DOCUMENT_HPP
#define BYTE_ARRAY_JSON_DOCUMENT_HPP

#include "byte_array/consumers.hpp"
#include "byte_array/json/exceptions.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "byte_array/tokenizer/tokenizer.hpp"
#include "script/ast.hpp"
#include "script/variant.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace byte_array {

class JSONDocument : private Tokenizer {
 public:
  typedef ByteArray string_type;
  typedef ConstByteArray const_string_type;  
  typedef script::Variant variant_type;

  enum Type {
    TOKEN = 0,
    STRING = 1,
    SYNTAX = 2,
    NUMBER = 3,
    WHITESPACE = 5,
    CATCH_ALL = 6
  };

  JSONDocument() {
    root_ = std::make_shared<variant_type>();

    AddConsumer(Type::TOKEN, consumers::AlphaNumericLetterFirst);
    AddConsumer(Type::WHITESPACE, consumers::Whitespace);
    AddConsumer(Type::STRING, consumers::StringEnclosedIn('"'));
    AddConsumer(Type::NUMBER, consumers::Integer);
    AddConsumer(Type::SYNTAX,
                consumers::TokenFromList({"[", "]", "{", "}", ",", ":"}));
    AddConsumer(Type::CATCH_ALL, consumers::AnyChar);
  }

  void Parse(string_type filename, const_string_type const& document) {
    // Parsing and tokenizing
    Tokenizer::Parse(filename, document);

    // Building an abstract syntax tree
    enum {
      OP_OBJECT = 1,
      OP_ARRAY = 2,
      OP_PROPERTY = 4,
      OP_APPEND = 8,
      OP_STRING = 16,
      OP_NUMBER = 32,
      OP_TRUE = 64,
      OP_FALSE = 128,
      OP_NULL = 256
    };

    using namespace script;
    ASTGroupOperationType T_OBJECT = {OP_OBJECT, ASTProperty::GROUP, 0};
    ASTGroupOperationType T_ARRAY = {OP_ARRAY, ASTProperty::GROUP, 0};
    ASTOperationType T_PROPERTY = {
        OP_PROPERTY, ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT, 1};

    ASTOperationType T_APPEND = {
        OP_APPEND, ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT, 2};
    ASTOperationType T_STRING = {OP_STRING, ASTProperty::TOKEN, 3};
    ASTOperationType T_NUMBER = {OP_NUMBER, ASTProperty::TOKEN, 3};
    ASTOperationType T_TRUE = {OP_TRUE, ASTProperty::TOKEN, 3};
    ASTOperationType T_FALSE = {OP_FALSE, ASTProperty::TOKEN, 3};
    ASTOperationType T_NULL = {OP_NULL, ASTProperty::TOKEN, 3};

    AbstractSyntaxTree tree;

    tree.PushTokenType(T_OBJECT);
    tree.PushTokenType(T_ARRAY);
    tree.PushTokenType(T_PROPERTY);

    tree.PushTokenType(T_APPEND);
    tree.PushTokenType(T_STRING);
    tree.PushTokenType(T_NUMBER);
    tree.PushTokenType(T_TRUE);
    tree.PushTokenType(T_FALSE);
    tree.PushTokenType(T_NULL);

    for (auto &t : *this) {
      switch (t.type()) {
        case Type::SYNTAX:
          if (t == "[")
            tree.PushToken({T_ARRAY.open(), t});
          else if (t == "]")
            tree.PushToken({T_ARRAY.close(), t});
          else if (t == "{")
            tree.PushToken({T_OBJECT.open(), t});
          else if (t == "}")
            tree.PushToken({T_OBJECT.close(), t});
          else if (t == ":")
            tree.PushToken({T_PROPERTY, t});
          else if (t == ",")
            tree.PushToken({T_APPEND, t});
          break;

        case Type::STRING:
          tree.PushToken({T_STRING, t});
          break;

        case Type::NUMBER:
          tree.PushToken({T_NUMBER, t});
          break;

        case Type::TOKEN:
          if (t == "true")
            tree.PushToken({T_TRUE, t});
          else if (t == "false")
            tree.PushToken({T_FALSE, t});
          else if (t == "null")
            tree.PushToken({T_NULL, t});
          break;
        case Type::CATCH_ALL:
          throw UnrecognisedJSONSymbolException(t);
          break;
      }
    }

    tree.Build();

    // Creating variant;
    root_ = std::make_shared< variant_type >();
    // TODO:
  }

  variant_type &root() { return *root_; }
  variant_type const &root() const { return *root_; }

 private:
  std::shared_ptr<variant_type> root_;
  //  variant_type *root_ = nullptr;
};
};
};

#endif
