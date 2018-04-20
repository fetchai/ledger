#ifndef BYTE_ARRAY_JSON_DOCUMENT_HPP
#define BYTE_ARRAY_JSON_DOCUMENT_HPP

#include "byte_array/consumers.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "byte_array/consumers.hpp"
#include "script/variant.hpp"
#include "json/exceptions.hpp"

#include <memory>
#include <vector>
#include<stack>

namespace fetch {
namespace json {

  
class JSONDocument {

  enum Type {
    KEYWORD_TRUE = 0,
    KEYWORD_FALSE = 1,
    KEYWORD_NULL = 2,        
    STRING = 3,

    NUMBER_INT = 5,
    NUMBER_FLOAT = 6,

    OPEN_OBJECT = 11,
    CLOSE_OBJECT = 12,
    OPEN_ARRAY = 13,
    CLOSE_ARRAY = 14,

    KEY = 16
  };
    

  enum {
    PROPERTY = 2,
    ENTRY_ALLOCATOR = 3,
    OBJECT = 10,
    ARRAY = 11
  };  





 public:
  typedef byte_array::ByteArray string_type;
  typedef byte_array::ConstByteArray const_string_type;  
//  typedef script::Variant variant_type;

  JSONDocument() {
    counters_.reserve(32);
    variants_.Reserve(1024);
  }

  JSONDocument(const_string_type const &document) : JSONDocument() {
    Parse(document);
  }

  script::Variant& operator[](std::size_t const& i) {
    return root()[i];
  }

  script::Variant const& operator[](std::size_t const& i) const {
    return root()[i]; 
  }

  typename script::Variant::variant_proxy_type operator[](byte_array::BasicByteArray const &key) 
  {
    return root()[key];    
  }

  script::Variant const & operator[](byte_array::BasicByteArray const &key) const 
  {
    return root()[key];
  }

  std::vector< uint16_t > counters_;

  
  void Parse(const_string_type const& document) {    
    Tokenise(document);

    variants_.LazyResize( objects_ + 1);
    counters_.clear();
    
    uint32_t allocation_counter = 1;
    JSONObject current_object;
    
    char const *ptr = reinterpret_cast< char const * >( document.pointer() );

    
    for(auto const &t: tokens_ ) {
      switch(t.type) {
      case KEYWORD_TRUE:
        variants_[current_object.i++] = true;        
        break;        
      case KEYWORD_FALSE:
        variants_[current_object.i++] = false;
        break;
      case KEYWORD_NULL:
        variants_[current_object.i++].MakeNull(); // = nullptr;
        break;        
      case STRING:
        variants_[ current_object.i++ ].EmplaceSetString( document, t.first, t.second - t.first ) ;
        break;

      case NUMBER_INT:
        variants_[ current_object.i++ ] = atoi( ptr + t.first) ;
        break;        
      case NUMBER_FLOAT:
        variants_[ current_object.i++ ] = atof( ptr + t.first) ;        
        break;        
      case OPEN_OBJECT:
        
        object_assembly_.emplace_back(current_object);

        current_object.start = allocation_counter;
        current_object.i = allocation_counter;
        current_object.size= t.second;
        current_object.type = t.type;
        
        allocation_counter += t.second;        
        break;        
      case CLOSE_OBJECT:
        variants_[ object_assembly_.back().i ].SetObject(variants_, current_object.start, current_object.size);        

        current_object = object_assembly_.back();
        object_assembly_.pop_back();
        
        ++current_object.i;

        break;        
      case OPEN_ARRAY:
        object_assembly_.emplace_back(current_object);        

        current_object.start = allocation_counter;
        current_object.i = allocation_counter;
        current_object.size =  t.second;
        current_object.type = t.type;

        allocation_counter += t.second;                

        break;        
      case CLOSE_ARRAY:
        variants_[ object_assembly_.back().i ].SetArray(variants_, current_object.start, current_object.size);
        
        current_object = object_assembly_.back();        
        object_assembly_.pop_back();

        ++current_object.i;
        break;        
      }
    }  
  }

  script::Variant &root() { return variants_[0]; }
  script::Variant const &root() const { return variants_[0]; }

 private:
  void Tokenise(const_string_type const& document) 
  {
    int line = 0;
    uint64_t pos = 0;

    objects_ = 0;


    brace_stack_.reserve(32);
    brace_stack_.clear();
    
    object_stack_.reserve(32);
    object_stack_.clear();    
    counters_.reserve(32);
    counters_.clear();
    tokens_.reserve(1024);
    tokens_.clear();
    
    uint16_t element_counter = 0;
    
    char const *ptr = reinterpret_cast< char const *> (document.pointer());
    while( pos < document.size() ) {
      uint16_t const *words16 = reinterpret_cast< uint16_t const *> (ptr + pos);
      uint32_t const *words = reinterpret_cast< uint32_t const *> (ptr + pos);      
      char const &c = *(ptr+pos);
      if((document.size() - pos) > 2) {
        // Handling white spaces      
        switch(words16[0]) { 
        case 0x2020:  //a
        case 0x200A:  //
        case 0x200D:  //
        case 0x2009:  //
        case 0x0A20:  //
        case 0x0A0A:  //
        case 0x0A0D:  //
        case 0x0A09:  //
        case 0x0D20:  //
        case 0x0D0A:  //
        case 0x0D0D:  //
        case 0x0D09:  //
        case 0x0920:  //
        case 0x090A:  //
        case 0x090D:  //
        case 0x0909:  //
          pos += 2;
          continue;
        }
        
      }

      switch(c) {
      case '\n':
        ++line;
      case '\t':
      case ' ':
      case '\r':
        ++pos;        
        continue;
      }       
      // Handling keywords
      if((document.size() - pos) > 4) {
        switch(words[0]) {
        case 0x65757274:  // true
          ++objects_;
          tokens_.push_back({uint32_t(pos), pos+4, KEYWORD_TRUE});
          pos+=4;
          ++element_counter;        
          continue;
        case 0x736C6166:  // fals
          ++objects_;
          tokens_.push_back({pos, pos+5,  KEYWORD_FALSE});
          pos+=4;
          ++pos;
          ++element_counter;
          continue;
        case 0x6C6C756E:  // null
          ++objects_; // TODO: Move
          tokens_.push_back({pos, pos+4, KEYWORD_NULL});
          pos+=4;
          ++element_counter;        
          continue;   
        }
      }
      uint64_t oldpos = pos;
      uint8_t type;

      switch(c) {
      case '"':
        ++objects_;
        ++element_counter;        
        byte_array::consumers::StringConsumer< STRING >(document, pos);
        tokens_.push_back({oldpos+1, pos -1, STRING});        
        break;        
      case '{':
        brace_stack_.push_back('}');
        counters_.emplace_back(element_counter);
        element_counter = 0;
        tokens_.push_back({pos, 0, OPEN_OBJECT});
        object_stack_.emplace_back( &tokens_.back() );
        
        ++pos;
        break;
      case '}':
        if(brace_stack_.back() != '}') {
          throw JSONParseException("Expected '}', but found ']'");
        }
        brace_stack_.pop_back();
        tokens_.push_back({pos, uint64_t(element_counter ), CLOSE_OBJECT});
        object_stack_.back()->second  = (element_counter );
        
          
        object_stack_.pop_back();        
        element_counter = counters_.back();
        counters_.pop_back();
        ++element_counter;
        ++pos;
        ++objects_;        
        break;
      case '[':
        brace_stack_.push_back(']');
        counters_.emplace_back(element_counter);
        
        element_counter = 0;
        tokens_.push_back({pos, 0, OPEN_ARRAY});
        object_stack_.emplace_back( &tokens_.back() );
        
        ++pos;
        break;
      case ']':
        if(brace_stack_.back() != ']') {
          throw JSONParseException("Expected ']', but found '}'.");
        }
        brace_stack_.pop_back();        
        tokens_.push_back({pos, element_counter, CLOSE_ARRAY});
        object_stack_.back()->second  = element_counter;
        
        object_stack_.pop_back();        
        element_counter = counters_.back();
        ++element_counter;        
        counters_.pop_back();

        ++pos;
        ++objects_;
        
      break;
        
      case ':':
        if(brace_stack_.back() != '}') {
          throw JSONParseException("Cannot set property outside of object context");
        }        
        //        tokens_.back().type = KEY;
        ++pos;
        break;
        
      case ',':
//        tokens_.push_back({pos, 0, NEW_ENTRY});                
        ++pos;
        break;
        
      default: // If none of the above it must be number:

        ++element_counter;        
        type = uint8_t(byte_array::consumers::NumberConsumer< NUMBER_INT, NUMBER_FLOAT >(document,pos));
        if(type == uint8_t(-1)) {
          throw JSONParseException("Unable to parse integer.");
        }
        tokens_.push_back({oldpos, pos - oldpos, type});        
        break;
      }
    }

    if(!brace_stack_.empty()) {
      throw JSONParseException("Object or array indicators are unbalanced.");
    }
  }
  
  struct JSONObject 
  {
    uint64_t start = 0;
    uint64_t size = 1;    
    uint64_t i = 0;
    uint8_t type = 0;
  };
  

  std::vector< JSONObject > object_assembly_;
  
  struct JSONToken
  {
    uint64_t first;
    uint64_t second;
    uint8_t type;      
  };

  std::vector< JSONToken* > object_stack_; 
  std::vector< JSONToken > tokens_;
  script::VariantList variants_;   
  std::size_t objects_;


  std::vector< char > brace_stack_;
  
//  variant_type root_ = nullptr;
};
};
};

#endif
