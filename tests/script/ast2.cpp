#include<iostream>
#include<iomanip>
#include<chrono>
#include"byte_array/referenced_byte_array.hpp"
#include"script/variant.hpp"
#include"script/function.hpp"
using namespace fetch;

class Language {
public:
  typedef byte_array::ReferencedByteArray byte_array_type;
  typedef script::Variant variant_type;
  
  Language() {
    script::Function::ConfigureTokenizer(tokenizer_);
    script::Function::ConfigureAST(tree_);
  }

  void Parse(byte_array_type filename,
             byte_array_type document ) {
    tokenizer_.Parse( filename, document );    
    tree_.Clear();
    std::size_t n = 0;
    script::Function::BuildFunctionTree(tokenizer_, tree_, n);
    tree_.Build();
    node_list_.clear();

    VisitChildren( tree_.root_shared_pointer() );
    
  }
private:
  typedef std::shared_ptr<script::ASTNode> ast_node_ptr;
  std::vector< ast_node_ptr > node_list_;
  byte_array::Tokenizer tokenizer_;
  script::AbstractSyntaxTree tree_;
  script::Function function_;

  void VisitChildren(ast_node_ptr node) {
    for(ast_node_ptr c: node->children) 
      VisitChildren(c);

    node_list_.push_back( node );
  }  
};

int main() {
  Language intepreter;
  intepreter.Parse("test.file", R"(
(2 + 2) - 1
)");
  return 0;
}
