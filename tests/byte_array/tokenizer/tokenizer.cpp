#include<iostream>
#include<byte_array/tokenizer/tokenizer.hpp>
#include<byte_array/consumers.hpp>
using namespace fetch::byte_array;
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

  /*
  test.AddConsumer(B_CONTEXT_START, consumers::Keyword("{"));
  test.AddConsumer(B_CONTEXT_START, consumers::Keyword("{"), "function");
  test.AddConsumer(B_CONTEXT_END, consumers::Keyword("}"), "function");    
  test.CreateTokenSpace("function", B_CONTEXT_START, B_CONTEXT_START, B_CONTEXT_END);
  */
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
