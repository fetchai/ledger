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
    root_ = std::make_shared<variant_type>();

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

  
  void Parse(const_string_type const& document) {    
    // Parsing and tokenizing
    byte_array::Tokenizer::clear();    
    byte_array::Tokenizer::Parse(document);

    compilation_stack_.clear();
    operator_stack_ = std::stack< uint64_t >();
    
    for(auto &t: *this) {
      std::shared_ptr<variant_type> obj;
      
      switch(t.type()) {
      case KEYWORD:
        switch(t[0]) {
        case 't': // true;
          obj = std::make_shared< Variant >( true );
          break;                    
        case 'f': // false;
          obj = std::make_shared< Variant >( false );
          break;          
        case 'n': // null
          obj = std::make_shared< Variant >( nullptr );
          break;
          
        }
        
      case STRING:
        obj = std::make_shared< Variant >( t.SubArray(1,t.size() -2 ) );        
      case SYNTAX:
        switch(t[0]) {
        case '{':
          OpenGroup(OP_OBJECT);
          break;
        case '}':
          CloseGroup(OP_OBJECT);
          break;
        case '[':
          OpenGroup(OP_ARRAY);
          break;
        case ']':
          CloseGroup(OP_ARRAY);
          break;          
        case ':':
          SetProperty();
          break;
        case ',':
          SeparateEntries();
          break;
        }                      
      case NUMBER_INT:
        obj = std::make_shared< Variant >( t.AsInt() );
        break;
        
      case NUMBER_FLOAT:
        obj = std::make_shared< Variant >( t.AsFloat() );
        break;

      case WHITESPACE:
        continue;
      }
    }        
    
  }

  variant_type &root() { return *root_; }
  variant_type const &root() const { return *root_; }

 private:
  void OpenGroup(std::size_t const &id) 
  {
    operators_.push_back(id);
  }

  void CloseGroup(std::size_t const &id) 
  {

  }

  void SetProperty() 
  {

  }

  void SeparateEntries() 
  {

  }
  
  
  std::vector< std::shared_ptr<variant_type> > compilation_stack_;
  std::stack< uint64_t > operator_stack_;  
  std::shared_ptr<variant_type> root_;
  //  variant_type *root_ = nullptr;
};
};
};

#endif
