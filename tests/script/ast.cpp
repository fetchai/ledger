#include<iomanip>
#include<iostream>
#include"script/ast.hpp"
#include"byte_array/consumers.hpp"
#include"byte_array/tokenizer/tokenizer.hpp"


using namespace fetch::byte_array;
using namespace fetch::script;

class AbstractTreeEvaluator {
public:
  typedef std::shared_ptr<ASTNode> ast_node_ptr;

  AbstractTreeEvaluator(AbstractSyntaxTree const &tree) {
    VisitChildren(tree.root_shared_pointer());
  }
  
  virtual void operator() () = 0;
  std::vector< ast_node_ptr >& program() { return program_; };
  std::vector< ast_node_ptr > const & program() const { return program_; };  
private:
  void VisitChildren(ast_node_ptr node) {
    for(ast_node_ptr c: node->children) 
      VisitChildren(c);

    if( (node->token_class.properties & ASTProperty::GROUP) == 0)
      program_.push_back( node );
  }
    
  std::vector< ast_node_ptr > program_;
};

class StackPrinter : public AbstractTreeEvaluator {
public:
  StackPrinter(AbstractSyntaxTree const &tree) :
    AbstractTreeEvaluator( tree ) { }
  
  void operator() () override {
    for(auto &a: this->program()) 
      std::cout << a->symbol << " " << a->token_class.type << std::endl;
  }
};


int main() {
  //  oldcode();
  //  return 0;

  Tokenizer test;

  enum {
    TOK_TOKEN = 0,    
    TOK_BYTE_ARRAY = 1,
    TOK_OPERATOR = 2,
    TOK_WHITESPACE = 3,
    TOK_CATCH_ALL = 4     
  };

  test.AddConsumer(TOK_TOKEN, consumers::AlphaNumericLetterFirst);
  test.AddConsumer(TOK_WHITESPACE, consumers::Whitespace);
  test.AddConsumer(TOK_BYTE_ARRAY, consumers::StringEnclosedIn('"'));
  test.AddConsumer(TOK_OPERATOR, consumers::TokenFromList({"==", "!=", "<=" ,">=", "+=", "-=", "=", "+", "-", "/", "*", "(", ")"}) );
  test.AddConsumer(TOK_CATCH_ALL, consumers::AnyChar);

  test.Parse("test.file", R"( 
a * ( b + (a+c*g) * d + e) *a
)" );

  
  enum {
    OP_PARAN = 1,
    OP_ADD = 2,
    OP_MUL = 3,
    OP_RES = 4,
    OP_RET = 5
  };

  
  ASTGroupOperationType T_PARAN = {OP_PARAN, ASTProperty::GROUP, 0 } ;
  ASTOperationType T_MUL = {OP_MUL, ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT, 1 } ;
  ASTOperationType T_ADD = {OP_ADD, ASTProperty::OP_LEFT | ASTProperty::OP_RIGHT, 2 } ;

  ASTOperationType T_RES = {OP_RES, ASTProperty::TOKEN, 3 } ;
  ASTOperationType T_RET = {OP_RET, ASTProperty::OP_LEFT, 4} ;

  AbstractSyntaxTree tree;  
  tree.PushTokenType( T_PARAN );
  tree.PushTokenType( T_ADD );
  tree.PushTokenType( T_MUL );

  std::cout << "Creating tree list" << std::endl;
  for(auto &t : test) {

    switch(t.type()) {
    case TOK_TOKEN:
      tree.PushToken( {T_RES, t } );
      break;
    case TOK_OPERATOR:
      if(t == "(") 
        tree.PushToken( {T_PARAN.open(), t } );
      else if(t == ")") 
        tree.PushToken( {T_PARAN.close(), t } );
      else if(t == "*") 
        tree.PushToken( {T_MUL, t } );
      else if(t == "+") 
        tree.PushToken( {T_ADD, t } );
      break;
    case TOK_WHITESPACE:
      break;
    case TOK_BYTE_ARRAY:
    case TOK_CATCH_ALL:
      std::cerr << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
      std::cerr << "Symbol '" << t <<  "' is not supported. " << std::endl;      
      exit(-1);
    }
  }

  std::cout << "Building" << std::endl;
  tree.Build();
  std::cout << "Tree: ";

  StackPrinter eval(tree);

  eval();
  std::cout << std::endl;

  return 0;
}
