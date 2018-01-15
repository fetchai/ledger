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
  
  virtual void Evaluate() = 0;
  std::vector< ast_node_ptr >& program() { return program_; };
  std::vector< ast_node_ptr > const & program() const { return program_; };
  
private:
  void VisitChildren(ast_node_ptr node) {
    for(ast_node_ptr c: node->children) 
      VisitChildren(c);

    // FIXME: Generalise
    if( (node->token_class.properties & ASTProperty::GROUP) == 0)
      program_.push_back( node );
  }
    
  std::vector< ast_node_ptr > program_;
};

class Calculator : public AbstractTreeEvaluator {
public:
  enum {
    OP_PARAN,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQEQ,
    OP_NEQ,
    OP_LE,
    OP_GE,
    OP_LTE,
    OP_GTE,
    OP_SIGN,    
    TYPE_INT
  };


  Calculator(AbstractSyntaxTree const &tree) :
    AbstractTreeEvaluator( tree ) { }

  int operator() () {
    Evaluate();
    return result();
  }
  
  void Evaluate() override {
    int a, b;
    stack_.clear();
    
    for(auto &node: this->program()) {
      if(verbose_) {
        for(auto &s: stack_ ) 
          std::cout << s << " ";
        std::cout << node->symbol << std::endl;
      }
      
      switch(node->token_class.type) {
      case TYPE_INT:
        stack_.push_back( node->symbol.AsInt() );
        break;
      case OP_ADD:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a + b);
        break;
      case OP_SIGN:
        a = TopPop();        
        stack_.push_back( -a);
        break;        
      case OP_SUB:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a - b);
        break;
      case OP_MUL:       
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a * b);
        break;
      case OP_DIV:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a / b);
        break;
      case OP_EQEQ:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a == b);
        break;
      case OP_NEQ:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a != b);
        break;
      case OP_LE:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a < b);
        break;
      case OP_GE:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a > b);
        break;
      case OP_LTE:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a <= b);
        break;
      case OP_GTE:
        b = TopPop();
        a = TopPop();        
        stack_.push_back( a >= b);
        break;                
      default:        
        std::cout << "Other: " <<  node->symbol << std::endl;
        break;
      }
    }   
  }

  int result() const {
    if( stack_.size() != 1 ) return 0;
    return stack_[0];
  }
private:
  int TopPop() {
    int n = stack_.back();
    stack_.pop_back();
    return n;
  }
  
  std::vector< int > stack_;
  bool verbose_ = false;
};


int main(int argc, char **argv) {
  if(argc == 1) {
    std::cerr << "usage: ./" << argv[0] << " [expression]" << std::endl;
    exit(-1);
  }
  
  ReferencedByteArray input;
  for(std::size_t i =1 ; i < argc; ++i)
    input = input + argv[i];
  
  Tokenizer test;

  enum {
    TOK_TOKEN = 1,
    TOK_OPERATOR = 2,
    TOK_WHITESPACE = 3,
    TOK_CATCH_ALL = 4     
  };

  test.AddConsumer(TOK_TOKEN, consumers::Integer);
  test.AddConsumer(TOK_WHITESPACE, consumers::Whitespace);
  test.AddConsumer(TOK_OPERATOR, consumers::TokenFromList({"==", "!=", "<=" ,">=", ">", "<","+", "-", "/", "*", "(", ")"}) );
  test.AddConsumer(TOK_CATCH_ALL, consumers::AnyChar);

  test.Parse("test.file", input);

  AbstractSyntaxTree tree;
  
  ASTGroupOperationType T_PARAN = tree.AddGroup( Calculator::OP_PARAN );
  ASTOperationType T_SIGN = tree.AddRight(Calculator::OP_SIGN);
  ASTOperationType T_EQEQ = tree.AddLeftRight( Calculator::OP_EQEQ);
  ASTOperationType T_NEQ = tree.AddLeftRight(Calculator::OP_NEQ);
  ASTOperationType T_LE = tree.AddLeftRight(Calculator::OP_LE);
  ASTOperationType T_GE = tree.AddLeftRight(Calculator::OP_GE);
  ASTOperationType T_LTE = tree.AddLeftRight( Calculator::OP_LTE);
  ASTOperationType T_GTE = tree.AddLeftRight( Calculator::OP_GTE);
  ASTOperationType T_MUL = tree.AddLeftRight( Calculator::OP_MUL);
  ASTOperationType T_DIV = tree.AddLeftRight( Calculator::OP_DIV);
  ASTOperationType T_ADD = tree.AddLeftRight( Calculator::OP_ADD);
  ASTOperationType T_SUB = tree.AddLeftRight( Calculator::OP_SUB);
  ASTOperationType T_INT = tree.AddToken(Calculator::TYPE_INT);

  bool last_token = false, next_token = false;

  for(auto &t : test) {
    next_token = false;
    switch(t.type()) {
    case TOK_TOKEN:
      tree.PushToken( {T_INT, t } );
      next_token = true;
    case TOK_OPERATOR:
      if(t == "(") 
        tree.PushToken( {T_PARAN.open(), t } );
      else if(t == ")") 
        tree.PushToken( {T_PARAN.close(), t } );
      else if(t == "*") 
        tree.PushToken( {T_MUL, t } );
      else if(t == "+") 
        tree.PushToken( {T_ADD, t } );
      else if(t == "/") 
        tree.PushToken( {T_DIV, t } );
      else if(t == "-") {
        if(last_token)
          tree.PushToken( {T_SUB, t } );
        else
          tree.PushToken( {T_SIGN, t } );          
      } else if(t == "==") 
        tree.PushToken( {T_EQEQ, t } );
      else if(t == "!=") 
        tree.PushToken( {T_NEQ, t } );
      else if(t == "<=") 
        tree.PushToken( {T_LTE, t } );
      else if(t == ">=") 
        tree.PushToken( {T_GTE, t } );
      else if(t == "<") 
        tree.PushToken( {T_LE, t } );
      else if(t == ">") 
        tree.PushToken( {T_GE, t } );
      break;
    case TOK_WHITESPACE:
      break;
    case TOK_CATCH_ALL:
      std::cerr << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
      std::cerr << "Symbol '" << t <<  "' is not supported. " << std::endl;      
      exit(-1);
    }
    last_token = next_token;
  }

  tree.Build();

  Calculator eval(tree);

  std::cout << "Result = " << eval() << std::endl;
  std::cout << std::endl;

  return 0;
}
