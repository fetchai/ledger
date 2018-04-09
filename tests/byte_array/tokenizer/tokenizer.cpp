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
    E_TOKEN = 0,    
    E_BYTE_ARRAY = 1,
    E_OPERATOR = 2,
    E_WHITESPACE = 3,
    E_INTEGER = 4,
    E_FLOATING_POINT = 5,
    E_KEYWORD = 6,        
    E_CATCH_ALL = 7
  };

/*
[](ConstByteArray const &str, uint64_t &pos) {
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
        
        if((pos < str.size()) && ((str[pos] == 'e') || (str[pos] == 'f') || (str[pos] == 'E') || (str[pos] == 'F') ) ) {
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
      }*/  

   /*
   test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int {
       uint64_t oldpos = pos;
       
       while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
       
       if(pos != oldpos) {
         int ret =E_INTEGER; 
         
         if((pos < str.size()) && (str[pos] == '.')) {
          ++pos;
          ret = int(E_FLOATING_POINT);
          while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;          
         }        
         return ret;
       }
       
       return -1; 
     });
   */
   test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int {
       switch( str[pos] ) {
       case '*':
       case '/':         
       case '-':
       case '+':
       case ':':
       case '=':         
         ++pos;
         return E_OPERATOR;
       }
       
                    
       return -1; 
     });
   
   

   

   test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int{
       static std::vector< ConstByteArray > keywords = {
         "if", "then", "begin", "end", "procedure", "function", "program", 
       };

       for(auto const &k: keywords) {
         if(str.Match(k, pos)) {
           pos += k.size();           
           return int(E_KEYWORD);
         }
       }
       
       return -1; 
     });   


   test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) -> int{
       uint64_t oldpos = pos;       
       while ((('a' <= str[pos]) && (str[pos] <= 'z')) ||
         (('A' <= str[pos]) && (str[pos] <= 'Z')) || (str[pos] == '\''))
         ++pos;
       if(oldpos == pos) return -1;
       return E_TOKEN;
     });

  test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) {
      uint64_t oldpos = pos;
      while ((pos < str.size()) && ((str[pos] == ' ') || (str[pos] == '\n') ||
          (str[pos] == '\r') || (str[pos] == '\t')))
        ++pos;
      
      if(pos == oldpos) return -1;           
      return int(E_WHITESPACE);      
    });

  /*
  test.AddConsumer([](ConstByteArray const &str, uint64_t &pos) {
      ++pos;      
      return int(E_CATCH_ALL);      
    });
  */

  
//  test.Parse("test.file", R"(232    332e- 32.15e 32.3e-2.)" );
  FILE * pFile;
  long lSize;
  size_t result;

  for(std::size_t i = 0; i <  1; ++i) {
    test.clear();

    ByteArray data;
    pFile = fopen ( argv[1] , "r" );    
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
    
    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);    
    data.Resize(lSize);
    result = fread (data.pointer(),1,lSize,pFile);
    fclose(pFile);
    
    
    
    test.Parse(data);
  }
  
  std::cout << argv[1] << std::endl;
  
  std::cout << test.size() << std::endl;
  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  } 
  */ 
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
