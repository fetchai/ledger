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

  
class JSONDocument {

  enum Type {
    KEYWORD_TRUE = 0,
    KEYWORD_FALSE = 1,
    KEYWORD_NULL = 2,        
    STRING = 3,
    SYNTAX = 4,
    NUMBER_INT = 5,
    NUMBER_FLOAT = 6,    
    WHITESPACE = 7
  };

  enum {
    PROPERTY = 2,
    ENTRY_ALLOCATOR = 3,
    OBJECT = 10,
    ARRAY = 11
  };  


  /* Consumes an integer from a byte array if found.
   * @param str is a constant byte array.
   * @param pos is a position in the byte array.
   *
   * The implementation follows the details given on JSON.org. The
   * function has support for numbers such as 23, 32.15, -2e0 and
   * -3.2e+3. Numbers are classified either as integers or floating
   * points. 
   */
  static int NumberConsumer(byte_array::ConstByteArray const &str, uint64_t &pos) {
    /* ┌┐                                                                        ┌┐
    ** ││                   ┌──────────────────────┐                             ││
    ** ││                   │                      │                             ││
    ** ││                   │                      │                             ││
    ** ││             .─.   │   .─.    .─────.     │                             ││
    ** │├─┬────────┬▶( 0 )──┼─▶( - )─▶( digit )──┬─┴─┬─────┬────────────┬───────▶││
    ** ││ │        │  `─' ┌─┘   `─'    `─────'   │   │     │            │        ││
    ** ││ │        │      │               ▲      │   ▼     ▼         .─────.     ││
    ** ││ │   .─.  │   .─────.            │      │  .─.   .─.  ┌───▶( digit )──┐ ││
    ** └┘ └─▶( - )─┴─▶( digit )──┐        └──────┘ ( e ) ( E ) │     `─────'   │ └┘
    **        `─'      `─────'   │                  `─'   `─'  │        ▲      │   
    **                    ▲      │                   │     │   │        │      │   
    **                    │      │                   ├──┬──┤   │        └──────┘   
    **                    └──────┘                   ▼  │  ▼   │                   
    **                                              .─. │ .─.  │                   
    **                                             ( + )│( - ) │                   
    **                                              `─' │ `─'  │                   
    **                                               │  │  │   │                   
    **                                               └──┼──┘   │                   
    **                                                  └──────┘                   
    */                                         
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


  /* Consumes string starting and ending with ".
   * @param str is a constant byte array.
   * @param pos is a position in the byte array.
   *
   * The implementation follows the details given on JSON.org.
   * Currently, there is no checking if unicodes are correctly
   * formatted.
   */
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

  /* Consumes either of the three keywords: true, false and null.
   * @param str is a constant byte array.
   * @param pos is a position in the byte array.
   *
   * The implementation follows the details given on JSON.org.
   */  
  static int KeywordConsumer(byte_array::ConstByteArray const &str, uint64_t &pos){
    if(str.Match("null",pos)) {
      pos+=4;
      
      return KEYWORD_NULL;      
    }

    if(str.Match("true",pos)) {
      pos+=4;
      
      return KEYWORD_TRUE;      
    }

    if(str.Match("false",pos)) {
      pos+=5;      
      return KEYWORD_FALSE;
    }
    
    return -1; 
  }


  /* Consumes whitespaces.
   * @param str is a constant byte array.
   * @param pos is a position in the byte array.
   *
   * Accepted whitespaces are unspecified on JSON.org.
   */    
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
//  typedef script::Variant variant_type;

  JSONDocument() {
  }

  JSONDocument(const_string_type const &document) : JSONDocument() {
    Parse(document);
  }
/*
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
*/
  std::vector< uint32_t > counters_;
  std::vector< std::size_t > allocators_;      
  std::vector< byte_array::Token > tokens_;
  
  void Parse(const_string_type const& document) {    
    uint64_t line = 0, character = 0;
    uint64_t pos = 0;

    int max_depth = 0, current_depth = 0, objects = 0, total_allocators = 0;
    
    while( pos < document.size() ) {
      char c = document[pos];
      ++character;
      int type;
      
      switch(c) {
      case '\n':
        character = 0;        
        ++line;
      case '\t':
      case ' ':
      case '\r':
        if(WhiteSpaceConsumer(document, pos) != WHITESPACE) {
          std::cerr << "Could not consume token at pos." << std::endl;
          exit(-1);
        }
        continue;
      case 't':
      case 'f':
      case 'n':
        type = KeywordConsumer(document, pos);

        if(type == -1) {
          std::cerr << "Could not consume token at pos." << std::endl;
          exit(-1);
        }
        
        ++objects;
      case '"':
        if(StringConsumer(document, pos) != STRING) {
          std::cerr << "Could not consume token at pos." << std::endl;
          exit(-1);
        }

        ++objects;        
        break;        
      case '{':
        ++current_depth;        
        ++pos;
        break;
        
      case '}':
        if(current_depth > max_depth) max_depth = current_depth;        
        --current_depth;
        ++objects;
        ++total_allocators;        
        ++pos;
        break;
        
      case '[':
        ++current_depth;
        ++pos;
        break;
        
      case ']':
        if(current_depth > max_depth) max_depth = current_depth;        
        --current_depth;
        ++objects;        
        ++total_allocators;
        ++pos;
        break;
        
      case ':':
        ++pos;
        break;
        
      case ',':
        ++pos;
        break;
        
      default: // If none of the above it must be number:
        type = NumberConsumer(document,pos);
        if(type == -1) {          
          std::cerr << "Could not consume token at pos 3." << std::endl;
          exit(-1);          
        }         
        ++objects;        
        break;
      }      
    }
    
    counters_.reserve(max_depth);
    

    script::VariantList variants;
    
    variants.Resize(objects);
    std::size_t current_index = 0;
    
    
    pos = 0;    
    while( pos < document.size() ) {
      std::size_t oldpos = pos;

      char c = document[pos];
      int type;
      
      switch(c) {
      case '\n':
      case '\t':
      case ' ':
      case '\r':
        WhiteSpaceConsumer(document, pos);
        continue;
      case 't':
      case 'f':
      case 'n':
        type = KeywordConsumer(document, pos);
        
        switch(type) {
        case KEYWORD_TRUE:
          variants[ current_index++ ] = true;
          
          break;
        case KEYWORD_FALSE:
          variants[ current_index++ ] = false;
          
          break;          
        case KEYWORD_NULL:
// TODO          variants[ current_index++ ] = nullptr;
          
          break;
        }
        
      case '"':
        StringConsumer(document, pos);
        variants[ current_index++ ] =  document.SubArray(oldpos+ 1,pos - oldpos -2 ) ;
        break;        
      case '{':
        counters_.emplace_back(0);
        ++pos;
        break;
        
      case '}': 
      {
        
//        allocators_.emplace_back( counters_.back() );
//        variants.emplace_back( object );
        
        counters_.pop_back();
        ++pos;

      }      
        break;
        
      case '[':
        counters_.emplace_back(0);
        ++pos;
        break;
        
      case ']': 
      {
/*        
        int N = counters_.back();
        auto array = script::Variant::Array(N);
        auto it = variants.end() - N;
        for(std::size_t i =0; i < N; ++i) {
          array[i] = *it;
          ++it;          
        }
        
        while(N != 0) {
          variants.pop_back();
          --N;          
        }
*/
//        allocators_.emplace_back( counters_.back() );
//        variants.emplace_back( array );        
        counters_.pop_back();
        ++pos;
      }
      
      break;
        
      case ':':
        ++pos;
        break;
        
      case ',':
        ++pos;
        ++counters_.back();
        break;
        
      default: // If none of the above it must be number:
        type = NumberConsumer(document,pos);
        switch(type) {
        case NUMBER_INT:
          variants[ current_index++ ] = atoi( reinterpret_cast< char const * >( document.pointer() ) + oldpos) ;
          break;
        case NUMBER_FLOAT:
          variants[ current_index++ ] = atof( reinterpret_cast< char const * >( document.pointer() ) + oldpos) ;
          break;                    
        }
        break;
      }      
    }
    
//    std::cout << variants.size() << " " << counters_.size() << std::endl;
    
    if(variants.size()!= 1) {
//      throw JSONParseException("JSON compiled into more than one top level object");
    } else {
//      root_ = variants[0];

    }
  }

//  variant_type &root() { return root_; }
//  variant_type const &root() const { return root_; }

 private:
   
//  variant_type root_ = nullptr;
};
};
};

#endif
