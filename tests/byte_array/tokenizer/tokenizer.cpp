#include<iostream>
#include<byte_array/tokenizer/tokenizer.hpp>
#include<byte_array/consumers.hpp>
#include <string>
#include <fstream>
#include <streambuf>

using namespace fetch::byte_array;

int main(int argc, char **argv) 
{
  Tokenizer test;
  enum {
    E_INTEGER = 0,
    E_FLOATING_POINT = 1,
    E_STRING = 2,
    E_KEYWORD = 3,     
    E_TOKEN = 4,
    E_WHITESPACE = 5
  };


  int number_consumer = test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) {
      uint64_t oldpos = pos;
      uint64_t N = pos + 1;
      if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) &&
        (str[N] <= '9'))
        pos += 2;
      
      while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
      if(pos != oldpos) {
        int ret = int(E_INTEGER); 
        
        if((pos < str.size()) && (str[pos] == '.')) {
          ++pos;
          ret = int(E_FLOATING_POINT);
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
          ret = int(E_FLOATING_POINT);          
        }
        
        return ret;
      }
      
      return -1; 
    });

  
   int string_consumer = test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int {
       if(str[pos] != '"') return -1;
       ++pos;
       if(pos >= str.size()) return -1;

       while( (pos < str.size()) && (str[pos] != '"') ) {
         pos += 1 + (str[pos] == '\\');         
       }

       if( pos >= str.size() )
         return -1;
       ++pos;
       return E_STRING;
     });

   int keyword_consumer = test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int{
       static std::vector< ConstByteArray > keywords = {
         "null", "true", "false"
       };

       for(auto const &k: keywords) {
         if(str.Match(k, pos)) {
           pos += k.size();           
           return int(E_KEYWORD);
         }
       }
       
       return -1; 
     });      

  int token_consumer = test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int {      
      switch(str[pos]) {
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':        
        ++pos;
        return E_TOKEN;      
      };

      
      return -1; 
    });
  
   int white_space_consumer = test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int {
      uint64_t oldpos = pos;
      while ((pos < str.size()) && ((str[pos] == ' ') || (str[pos] == '\n') ||
          (str[pos] == '\r') || (str[pos] == '\t')))
        ++pos;
      
      if(pos == oldpos) return -1;           
      return int(E_WHITESPACE);      
    });


   test.SetConsumerIndexer([number_consumer, string_consumer, white_space_consumer, keyword_consumer, token_consumer](ConstByteArray const&str, uint64_t const&pos, int const& index) {
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



   ByteArray doc_content = R"({
  "a": 3,
  "x": { 
    "y": [1,2,3],
    "z": null,
    "q": [],
    "hello world": {}
  }
}
)" ;
   
   test.Parse(doc_content);
   
   for(auto &t : test) {
     std::cout <<  "Line " << t.line() << ", char " << t.character() << std::endl;
     std::cout << t.type() << " " << t.size() << " " << t << std::endl;
   } 
   return 0;
  
}

/*
void test1() {
  Tokenizer test;
  enum {
    E_TOKEN = 0,    
    E_BYTE_ARRAY = 1,
    E_OPERATOR = 2,
    E_WHITESPACE = 3,
    E_CATCH_ALL = 4     
  };

  test.AddConsumer(E_TOKEN, consumers::AlphaNumericLetterFirst);
  test.AddConsumer(E_WHITESPACE, consumers::Whitespace);  
  test.AddConsumer(E_BYTE_ARRAY, consumers::StringEnclosedIn('"'));
  test.AddConsumer(E_OPERATOR, consumers::TokenFromList({
        "==", "!=", "<=" ,">=", "+=", "-=", "=", "+", "-", "/", "*"
          }));  
  test.AddConsumer(E_CATCH_ALL, consumers::AnyChar);
    
  test.Parse("test.file", R"({
  "x": function helloworld() { var x = 2 * 2; x+=1; }
}
)" ); 

  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
}


int main() {
  Tokenizer test;
  enum {
    A_TOKEN = 0,    
    A_BYTE_ARRAY = 1,
    A_OPERATOR = 2,
    A_WHITESPACE = 3,
    A_CATCH_ALL = 5,
    
    B_TOKEN = 10,
    B_BYTE_ARRAY = 11,
    B_OPERATOR = 12,
    B_WHITESPACE = 13,
    B_CONTEXT_START = 14,
    B_CONTEXT_END = 15,    
    B_CATCH_ALL = 16    
  };


  test.CreateSubspace("function", B_CONTEXT_START, "{", B_CONTEXT_END, "}");
  
  test.AddConsumer(A_TOKEN, consumers::AlphaNumericLetterFirst);
  test.AddConsumer(A_WHITESPACE, consumers::Whitespace);  
  test.AddConsumer(A_BYTE_ARRAY, consumers::StringEnclosedIn('"'));
  test.AddConsumer(A_CATCH_ALL, consumers::AnyChar);


  test.AddConsumer(B_TOKEN, consumers::AlphaNumericLetterFirst, "function");
  test.AddConsumer(B_WHITESPACE, consumers::Whitespace, "function");  
  test.AddConsumer(B_BYTE_ARRAY, consumers::StringEnclosedIn('"'), "function");
  test.AddConsumer(B_OPERATOR, consumers::TokenFromList({
        "==", "!=", "<=" ,">=", "+=", "-=", "=", "+", "-", "/", "*"
          }), "function");  
  test.AddConsumer(B_CATCH_ALL, consumers::AnyChar, "function");


  
  test.Parse("test.file", R"(
Noun+ Verb { a + {b} } Noun + Verb
)" );
  

  for(auto &t : test) 
    std::cout << t << " : " << t.type() << std::endl;
  
  return 0;
}
*/
