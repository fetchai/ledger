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

       uint8_t const *ptr = str.pointer() + pos;
       uint8_t compare[16] = {'"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"'};
       uint8_t ret[16] = {0}; 
       ret[0] = (compare[0] == ptr[0]);
       ret[1] = (compare[1] == ptr[1]);
       ret[2] = (compare[2] == ptr[2]);
       ret[3] = (compare[3] == ptr[3]);
       ret[4] = (compare[4] == ptr[4]);
       ret[5] = (compare[5] == ptr[5]);
       ret[6] = (compare[6] == ptr[6]);
       ret[7] = (compare[7] == ptr[7]);
       ret[8] = (compare[8] == ptr[8]);
       ret[9] = (compare[9] == ptr[9]);
       ret[10] = (compare[10] == ptr[10]);
       ret[11] = (compare[11] == ptr[11]);
       ret[12] = (compare[12] == ptr[12]);
       ret[13] = (compare[13] == ptr[13]);
       ret[14] = (compare[14] == ptr[14]);
       ret[15] = (compare[15] == ptr[15]);
       uint16_t found = (ret[0] & 1) | (uint16_t(ret[1]&1) << 1 )
         | (uint16_t(ret[2]&1) << 2 ) | (uint16_t(ret[3]&1) << 3 )
         | (uint16_t(ret[4]&1) << 4 ) | (uint16_t(ret[5]&1) << 5 )
         | (uint16_t(ret[6]&1) << 6 ) | (uint16_t(ret[7]&1) << 7 )
         | (uint16_t(ret[8]&1) << 8 ) | (uint16_t(ret[9]&1) << 9 )
         | (uint16_t(ret[10]&1) << 10 ) | (uint16_t(ret[11]&1) << 11 )
         | (uint16_t(ret[12]&1) << 12 ) | (uint16_t(ret[13]&1) << 13 )
         | (uint16_t(ret[14]&1) << 14 ) | (uint16_t(ret[15]&1) << 15 );

       /*       
       bool not_found = (str[pos] != '"')  && (str[pos+1] != '"')  &&
         (str[pos+2] != '"')  && (str[pos+3] != '"') &&
         (str[pos +4] != '"')  && (str[pos+5] != '"')  &&
         (str[pos+6] != '"')  && (str[pos+7] != '"') ;
       */
       //
       // https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=mm_move&expand=3609
       while( (pos < str.size()) && ( !found ) ) {
         pos += 16;
         ptr += 16;
         ret[0] = (compare[0] == ptr[0]);
         ret[1] = (compare[1] == ptr[1]);
         ret[2] = (compare[2] == ptr[2]);
         ret[3] = (compare[3] == ptr[3]);
         ret[4] = (compare[4] == ptr[4]);
         ret[5] = (compare[5] == ptr[5]);
         ret[6] = (compare[6] == ptr[6]);
         ret[7] = (compare[7] == ptr[7]);
         ret[8] = (compare[8] == ptr[8]);
         ret[9] = (compare[9] == ptr[9]);
         ret[10] = (compare[10] == ptr[10]);
         ret[11] = (compare[11] == ptr[11]);
         ret[12] = (compare[12] == ptr[12]);
         ret[13] = (compare[13] == ptr[13]);
         ret[14] = (compare[14] == ptr[14]);
         ret[15] = (compare[15] == ptr[15]);
         found = (ret[0] & 1) | (uint16_t(ret[1]&1) << 1 )
           | (uint16_t(ret[2]&1) << 2 ) | (uint16_t(ret[3]&1) << 3 )
           | (uint16_t(ret[4]&1) << 4 ) | (uint16_t(ret[5]&1) << 5 )
           | (uint16_t(ret[6]&1) << 6 ) | (uint16_t(ret[7]&1) << 7 )
           | (uint16_t(ret[8]&1) << 8 ) | (uint16_t(ret[9]&1) << 9 )
           | (uint16_t(ret[10]&1) << 10 ) | (uint16_t(ret[11]&1) << 11 )
           | (uint16_t(ret[12]&1) << 12 ) | (uint16_t(ret[13]&1) << 13 )
           | (uint16_t(ret[14]&1) << 14 ) | (uint16_t(ret[15]&1) << 15 );
         
       }

       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       pos += ( (pos < str.size()) && (str[pos] != '"') );
       
       
       if( pos >= str.size() )
         return -1;
       ++pos;
       return STRING;
     }


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
  script::VariantList variants_;  
  std::size_t current_index_ ;


  
  
  void Parse(const_string_type const& document) {    
    uint64_t line = 0, character = 0;
    uint64_t pos = 0;

    std::chrono::high_resolution_clock::time_point t1, t2, t3, t4;

    counters_.clear();
    variants_.Resize(1024);
    

    current_index_ = 0;       
    pos = 0;
    char const *ptr = reinterpret_cast< char const *> (document.pointer());

    bool inside_string = false;
    while( pos < document.size() ) {
      uint16_t const *words16 = reinterpret_cast< uint16_t const *> (ptr + pos);

      switch(words16[0]) { // Handling white spaces
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

      uint32_t const *words = reinterpret_cast< uint32_t const *> (ptr + pos);
      
      switch(words[0]) {
      case 0x65757274:  // true
        variants_[ current_index_++ ] = true;        
        pos+=4;
        continue;
      case 0x736C6166:  // fals
        pos+=4;
        ++pos;
        variants_[ current_index_++ ] = false;
        continue;
      case 0x6C6C756E:  // null
        pos+=4;
        continue;   
      }
      
      std::size_t oldpos = pos;
      char const &c = *(ptr+pos);

      int type;
      //`      std::cout << "Char: " << c << std::endl;
      switch(c) {
      case '\n':
        ++line;
      case '\t':
      case ' ':
      case '\r':
        ++pos;
        continue;        
      case '"':

        StringConsumer(document, pos);
        variants_[ current_index_++ ].EmplaceSetString( document, oldpos+ 1,pos - oldpos -2 ) ;
        //        std::cout <<document.SubArray( oldpos+ 1,pos - oldpos -2 ) << std::endl;
        break;        
      case '{':
        CreateObject();
        ++pos;
        break;
        
      case '}': 
      {
        CloseObject();
        ++pos;
      }      
        break;
        
      case '[':
        CreateArray();
        ++pos;
        break;
        
      case ']': 
      {
        CloseArray();

        ++pos;
      }
      
      break;
        
      case ':':
        ++pos;
        CreateProperty();
        break;
        
      case ',':
        ++pos;
        CreateEntry();
        break;
        
      default: // If none of the above it must be number:
        type = NumberConsumer(document,pos);
        switch(type) {
        case NUMBER_INT:
          variants_[ current_index_++ ] = atoi( reinterpret_cast< char const * >( document.pointer() ) + oldpos) ;
          break;
        case NUMBER_FLOAT:
          variants_[ current_index_++ ] = atof( reinterpret_cast< char const * >( document.pointer() ) + oldpos) ;
          break;                    
        }

        break;
      }

    }

    

  }

//  variant_type &root() { return root_; }
//  variant_type const &root() const { return root_; }

 private:
  void CreateArray() {
    counters_.emplace_back(0);
  }
  
  void CreateObject() {
    counters_.emplace_back(0);
  }

  void CreateEntry() {
    ++counters_.back();
  }
  
  void CreateProperty() {
  }

  void CloseArray() {
    int N = counters_.back();
    variants_[current_index_] = script::VariantList(variants_, current_index_-N, N);
    ++current_index_;
    counters_.pop_back();    
  }

  void CloseObject() {
    int N = counters_.back();
    variants_[current_index_].EmplaceSetArray( variants_, current_index_-N, N );
    ++current_index_; 
    
    counters_.pop_back();
  }  
   
//  variant_type root_ = nullptr;
};
};
};

#endif
