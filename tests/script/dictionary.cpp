#include<iostream>
#include"script/dictionary.hpp"
using namespace fetch::script;

int main() {
  Dictionary< int > dict;
  dict["Hello world"] = 9;
  dict["Hi"] = 2;
  std::cout << dict["Hello world"] << std::endl;
  return 0;
}
