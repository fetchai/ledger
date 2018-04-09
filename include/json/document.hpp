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
#include<stack>
namespace fetch {
namespace json {

class JSONDocument : private byte_array::Tokenizer {

  enum Type {
    KEYWORD = 0,
    STRING = 1,
    SYNTAX = 2,
    NUMBER_INT = 3,
    NUMBER_FLOAT = 4,    
    WHITESPACE = 5
  };

  static int NumberConsumer(byte_array::ConstByteArray const &str, uint64_t &pos) {
      uint64_t oldpos = pos;
      uint64_t N = pos + 1;
      if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) &&
        (str[N] <= '9'))
        pos += 2;
      
      while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
      if(pos != oldpos) {
        int ret = int(NUMBER_INT); 
        
        if((pos < str.size()) && (str[pos] == '.')) {
          ++pos;
          ret = int(NUMBER_FLOAT);
          while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;          
        }
        
        if((pos < str.size()) && ((str[pos] == 'e') || (str[pos] == 'E')  ) ) {
          uint64_t rev = 1;          
          ++pos;
          
          if((pos < str.size()) && ((str[pos] == '-') || (str[pos] == '+' ) ) ) {
            ++pos;
            ++rev;
          }
          
          oldpos = pos;
          while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
          if( oldpos == pos ) {
            pos -= rev;
          }       
          ret = int(NUMBER_FLOAT);
        }
        
        return ret;
      }
      
      return -1; 
  }

  static int StringConsumer(byte_array::ConstByteArray const &str, uint64_t &pos)  {
       if(str[pos] != '"') return -1;
       ++pos;
       if(pos >= str.size()) return -1;

       while( (pos < str.size()) && (str[pos] != '"') ) {
         pos += 1 + (str[pos] == '\\');         
       }

       if( pos >= str.size() )
         return -1;
       ++pos;
       return STRING;
     }

  static int TokenConsumer(byte_array::ConstByteArray const &str, uint64_t &pos)  {      
      switch(str[pos]) {
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':        
        ++pos;
        return SYNTAX;
      };

      
      return -1; 
  }
  
  static int KeywordConsumer(byte_array::ConstByteArray const &str, uint64_t &pos){
       static std::vector< byte_array::ConstByteArray > keywords = {
         "null", "true", "false"
       };

       for(auto const &k: keywords) {
         if(str.Match(k, pos)) {
           pos += k.size();           
           return int(KEYWORD);
         }
       }
       
       return -1; 
  }

  
  static int WhiteSpaceConsumer(byte_array::ConstByteArray const &str, uint64_t &pos)  {
      uint64_t oldpos = pos;
      while ((pos < str.size()) && ((str[pos] == ' ') || (str[pos] == '\n') ||
          (str[pos] == '\r') || (str[pos] == '\t')))
        ++pos;
      
      if(pos == oldpos) return -1;           
      return int(WHITESPACE);      
    }
  
 public:
  typedef byte_array::ByteArray string_type;
  typedef byte_array::ConstByteArray const_string_type;  
  typedef script::Variant variant_type;

  JSONDocument() {

    int number_consumer = AddConsumer(NumberConsumer);
    int string_consumer = AddConsumer(StringConsumer);
    int token_consumer = AddConsumer(TokenConsumer);
    int keyword_consumer =  AddConsumer(KeywordConsumer);
    int white_space_consumer = AddConsumer(WhiteSpaceConsumer);
    
    SetConsumerIndexer([number_consumer, string_consumer, white_space_consumer, keyword_consumer, token_consumer](byte_array::ConstByteArray const&str, uint64_t const&pos, int const& index) {
        char c = str[pos];
        switch(c) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
          return white_space_consumer;
        case 't':
        case 'f':
        case 'n':
          return keyword_consumer;
        case '{':
        case '}':
        case '[':
        case ']':
        case ':':
        case ',':
          return  token_consumer;
        case '"':
          return string_consumer;        
        }

        return number_consumer;

    });
  }

  JSONDocument(const_string_type const &document) : JSONDocument() {
    Parse(document);
  }

  script::Variant& operator[](std::size_t const& i) {
    return root_[i];
  }

  script::Variant const& operator[](std::size_t const& i) const {
    return root_[i];    
  }

  script::Variant & operator[](byte_array::BasicByteArray const &key) 
  {
    return root_[key];    
  }

  script::Variant const & operator[](byte_array::BasicByteArray const &key) const 
  {
    return root_[key];
  }

  enum {
    PROPERTY = 2,
    ENTRY_ALLOCATOR = 3,
    OBJECT = 10,
    ARRAY = 11
  };
  void Parse(const_string_type const& document) {    
    // Parsing and tokenizing
    byte_array::Tokenizer::clear();    
    byte_array::Tokenizer::Parse(document);

    compilation_stack_.clear();
    operator_stack1_ = std::stack< uint64_t >();
    operator_stack2_ = std::stack< uint64_t >();    
    
    for(std::size_t i=0; i < this->size(); ++i) {
      auto const &t = this->at(i);
      
      switch(t.type()) {
      case KEYWORD:
        switch(t[0]) {
        case 't': // true;
          compilation_stack_.emplace_back( true );
          break;                    
        case 'f': // false;
          compilation_stack_.emplace_back( false );
          break;          
        case 'n': // null
          compilation_stack_.emplace_back( nullptr );
          break;
          
        }
        
      case STRING:
        compilation_stack_.emplace_back( t.SubArray(1,t.size() -2 ) );        
      case SYNTAX:
        switch(t[0]) {
        case '{':
          PushToStack(OBJECT);
          if ((i+1) < this->size()) {
            auto const &t2 = this->at(i+1);
            if(t2[0] != '}')  {
              PushToStack(ENTRY_ALLOCATOR);
            }
          }
          break;
        case '}':
          CloseGroup(OBJECT);
          break;
        case '[':
          PushToStack(ARRAY);
          if ((i+1) < this->size()) {
            auto const &t2 = this->at(i+1);
            if(t2[0] != ']')  {
              PushToStack(ENTRY_ALLOCATOR);
            }
          }          

          break;
        case ']':
          CloseGroup(ARRAY);
          break; 
        case ':':
          PushToStack(PROPERTY);
          break;
        case ',':
          PushToStack(ENTRY_ALLOCATOR);
          break;
        }
        break;
      case NUMBER_INT:
        compilation_stack_.emplace_back( t.AsInt() );
        break;
        
      case NUMBER_FLOAT:
        compilation_stack_.emplace_back( t.AsFloat() );
        break;

      case WHITESPACE:
        continue;
      }
    } 

    if(compilation_stack_.size()!= 1) {
      std::cerr << "Something went wrong" << std::endl;
    } else {
      root_ = compilation_stack_[0];
      compilation_stack_.pop_back();
    }
  }

  variant_type &root() { return root_; }
  variant_type const &root() const { return root_; }

 private:
  void PushToStack(std::size_t const &id)    
  {
    if( (id != ARRAY) && (id != OBJECT)) {
    while((!operator_stack2_.empty()) && (operator_stack2_.top() < id)) {
      operator_stack1_.push(operator_stack2_.top());
      operator_stack2_.pop();
    }
    }
    operator_stack2_.push(id);
  }

  void CloseGroup(std::size_t const &id) 
  {
    std::size_t n = 0;
    while((!operator_stack2_.empty()) && (id != operator_stack2_.top())) {
      auto const &t = operator_stack2_.top();
      switch(t) {
      case  PROPERTY:
        operator_stack1_.push(t);
        break;
      case OBJECT:
      case ARRAY:
        std::cerr << "unmatched group" << std::endl;
        return;
        break;
      default:
        ++n;
      }
      operator_stack2_.pop();
    }
    
    if(operator_stack2_.empty()) {
      std::cerr << "Could not find group: " << id << std::endl;
    }
    
    if(id == OBJECT) {
      CreateObject(n);
    } else {
      CreateArray(n);
    }
    operator_stack2_.pop();
  }

  
  void CreateObject(std::size_t const &n) 
  {
    variant_type obj = variant_type::Object();

    if( (2*n) > compilation_stack_.size() ) {
      std::cerr << " expected value" << std::endl;
      return;
    }
    
    for(std::size_t i = 0; i < n; ++i) {
      if(operator_stack1_.empty()) {
        std::cerr << " error, expected colon" << std::endl;
        return;
      }
      
      operator_stack1_.pop();
      variant_type const &value = compilation_stack_.back();
      compilation_stack_.pop_back();

      variant_type const &key = compilation_stack_.back();
      compilation_stack_.pop_back();
      obj[key.as_byte_array()] = value;
    }

    compilation_stack_.push_back(obj);
  }

  
  void CreateArray(std::size_t const &n) 
  {
    variant_type arr = variant_type::Array(n);
    for(std::size_t i=n; i !=0; ) {
      --i;

      arr[i] = compilation_stack_.back();
      compilation_stack_.pop_back();
    }

    compilation_stack_.push_back(arr);
  }
  
  std::vector< variant_type > compilation_stack_;
  std::stack< uint64_t > operator_stack2_;    
  std::stack< uint64_t > operator_stack1_;  
  variant_type root_;
};
};
};

#endif
