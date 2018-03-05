#ifndef BYTE_ARRAY_JSON_DOCUMENT_HPP
#define BYTE_ARRAY_JSON_DOCUMENT_HPP

#include "byte_array/consumers.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "byte_array/tokenizer/tokenizer.hpp"
#include "script/ast.hpp"
#include "script/variant.hpp"
#include "json/exceptions.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace json {

class JSONDocument : private byte_array::Tokenizer {
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


  
 public:
  typedef byte_array::ByteArray string_type;
  typedef byte_array::ConstByteArray const_string_type;  
  typedef script::Variant variant_type;

  enum Type {
    TOKEN = 0,
    STRING = 1,
    SYNTAX = 2,
    NUMBER = 3,
    WHITESPACE = 5,
    CATCH_ALL = 6
  };

  JSONDocument(script::Variant var) : JSONDocument() {
    *root_ = var;
  }

  JSONDocument() {
    root_ = std::make_shared<variant_type>(script::Variant::Object());

    AddConsumer(Type::TOKEN, byte_array::consumers::AlphaNumericLetterFirst);
    AddConsumer(Type::WHITESPACE, byte_array::consumers::Whitespace);
    AddConsumer(Type::STRING, byte_array::consumers::StringEnclosedIn('"'));
    AddConsumer(Type::NUMBER, byte_array::consumers::Integer);
    AddConsumer(Type::SYNTAX,
                byte_array::consumers::TokenFromList({"[", "]", "{", "}", ",", ":"}));
    AddConsumer(Type::CATCH_ALL, byte_array::consumers::AnyChar);
  }

  JSONDocument(string_type filename, const_string_type const &document) : JSONDocument() {
    Parse(filename, document);
  }

  script::Variant& operator[](std::size_t const& i) {
    return (*root_)[i];
  }

  script::Variant const& operator[](std::size_t const& i) const {
    return (*root_)[i];    
  }

  script::Variant & operator[](byte_array::BasicByteArray const &key) 
  {
    return (*root_)[key];    
  }

  script::Variant const & operator[](byte_array::BasicByteArray const &key) const 
  {
    return (*root_)[key];
  }

  
  void Parse(string_type filename, const_string_type const& document) {    
    // Parsing and tokenizing
    byte_array::Tokenizer::clear();    
    byte_array::Tokenizer::Parse(filename, document);

    // Building an abstract syntax tree
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
    VisitASTNodes( tree.root_shared_pointer(), *root_ );
  }

  variant_type &root() { return *root_; }
  variant_type const &root() const { return *root_; }

 private:
  typedef std::shared_ptr<script::ASTNode> ast_node_ptr;

  
  std::size_t VisitArrayElements(ast_node_ptr node, std::vector< ast_node_ptr > &array_contents)
  {
    if(node->token_class.type == OP_APPEND) {
      std::size_t ret = 0;      
      for(ast_node_ptr c: node->children)
      {
        ret += VisitArrayElements(c, array_contents);
      }
      return ret;      
    }
    array_contents.push_back(node);    
    return 1;    
  }


  std::size_t VisitObjectElements(ast_node_ptr node, std::vector< ast_node_ptr > &keys, std::vector< ast_node_ptr > &values)
  {
    if(node->token_class.type == OP_APPEND)
    {
      std::size_t ret = 0;
      
      for(ast_node_ptr c: node->children)
      {
        ret += VisitObjectElements(c, keys, values);
      }
        
      return ret;
    }
    

    if(node->token_class.type != OP_PROPERTY)
    {
      TODO_FAIL("Expected property");      
    }
              
    keys.push_back( node->children[0] );
    values.push_back( node->children[1] );
    return 1;
  }
  
  
  void VisitASTNodes( ast_node_ptr node, variant_type &variant) {


    switch(node->token_class.type) {
    case OP_APPEND: {
      TODO_FAIL("unexpected append");
    } break;
      
    case OP_ARRAY: {
      std::vector< ast_node_ptr > array_contents;
        
      std::size_t n = 0;
      for(auto &c: node->children)
      {
        n += VisitArrayElements( c, array_contents );
      }
            
      if(variant.type() != script::VariantType::UNDEFINED)
      {
        TODO_FAIL("Cannot alter type from", variant.type() ," to array");
      }
      
      variant = variant_type::Array( n );
      std::size_t i=0;      
      for(ast_node_ptr c: array_contents)
      {
        VisitASTNodes(c, variant[i++]);
      }
      
    } break;
    case OP_OBJECT: {
      std::vector< ast_node_ptr > keys, values;

      std::size_t n = 0;
      for(auto &c: node->children)
      {
        n += VisitObjectElements( c, keys, values);
      }
      
      if(variant.type() != script::VariantType::UNDEFINED)
      {
        TODO_FAIL("Cannot alter type from", variant.type() ," to object");
      }
      
      variant = variant_type::Object();
      
      
      for(std::size_t i=0; i < n; ++i )
      {
        ast_node_ptr key_tree = keys[i];
        ast_node_ptr value_tree = values[i];

        if(key_tree->children.size() != 0 )
        {
          TODO_FAIL("Key cannot have children");        
        }
        
        byte_array::ByteArray key = key_tree->symbol;
        if( (key.size() < 2) || (key[0] != '"') || (key[key.size()-1]!='"' ))
        {
          TODO_FAIL("Key must have quotes");        
        }
        
        key = key.SubArray(1, key.size() - 2);
        variant_type var;
        VisitASTNodes(value_tree, var);
        
        variant[key] = var;
        
      }
      
    } break;
    case OP_PROPERTY: {
      if(variant.type() != script::VariantType::DICTIONARY)
      {
        TODO_FAIL("Can only set properties of an object");
      }
      
      if(node->children.size() != 2)
      {
        TODO_FAIL("Expected a key and a value");
      }
      
      ast_node_ptr key_tree = node->children[0];
      ast_node_ptr value_tree = node->children[1];
      if(key_tree->children.size() != 0 )
      {
        TODO_FAIL("Key cannot have children");        
      }
      
      byte_array::ByteArray key = key_tree->symbol;
      if( (key.size() < 2) || (key[0] != '"') || (key[key.size()-1]!='"' ))
      {
        TODO_FAIL("Key must have quotes");        
      }
      
      key = key.SubArray(1, key.size() - 2);
      variant_type value;      
      
      VisitASTNodes(value_tree, value);
      variant[key] = value;
      
    } break;
    case OP_STRING:
      variant = node->symbol.SubArray(1, node->symbol.size() - 2); 
      break;
    case OP_NUMBER:
      variant = node->symbol.AsFloat();      
      break;      
    case OP_TRUE:
      variant = true; 
      break;
    case OP_FALSE:
      variant = false;
      break;
    case OP_NULL:
      variant = false;      
      break;      
    default:
 
      TODO_FAIL("Unknown type");
      break;
      
    }
        


  }
  
  std::shared_ptr<variant_type> root_;
  //  variant_type *root_ = nullptr;
};
};
};

#endif
