#ifndef PARSER__HPP
#define PARSER__HPP
#include "vm/node.hpp"


namespace fetch {
namespace vm {


class Parser
{
public:
	Parser() {}
	~Parser() {}
	BlockNodePtr Parse(const std::string& source, std::vector<std::string>& errors);

private:

	enum class State
	{
		// [group opener, prefix op or operand] is required
		PreOperand,
		// [postfix op, binary op, group separator or group closer] is optional
		PostOperand
	};

	enum class Association
	{
		Left,
		Right
	};

	struct OpInfo
	{
		OpInfo() {}
		OpInfo(int precedence__, Association association__, int arity__)
		{
			precedence = precedence__;
			association = association__;
			arity = arity__;
		}
		int 		precedence;
		Association association;
		int 		arity;
	};

	struct Expr
	{
		bool				is_operator;
		ExpressionNodePtr	node;
		OpInfo				op_info;
		int					count;
	};

	std::vector<Token>			tokens_;
	int							index_;
	Token*						token_;
	std::vector<std::string>	errors_;
	std::vector<Node::Kind>		blocks_;
	State						state_;
	bool						found_expression_terminator_;
	std::vector<int>			groups_;
	std::vector<Expr>			operators_;
	std::vector<Expr>			rpn_;
	std::vector<Expr>			infix_stack_;

	void Tokenise(const std::string& source);
	bool ParseBlock(BlockNode& node);
	BlockNodePtr ParseFunctionDefinition();
	BlockNodePtr ParseWhileStatement();
	BlockNodePtr ParseForStatement();
	NodePtr ParseIfStatement();
	NodePtr ParseVarStatement();
	NodePtr ParseReturnStatement();
	NodePtr ParseBreakStatement();
	NodePtr ParseContinueStatement();
	ExpressionNodePtr ParseExpressionStatement();
	void GoToNextStatement();
	void SkipFunctionDefinition();
	ExpressionNodePtr ParseType();
	ExpressionNodePtr ParseConditionalExpression();
	ExpressionNodePtr ParseExpression(const bool is_conditional_expression = false);
	bool HandleIdentifier();
	bool ParseExpressionIdentifier(std::string& name);
	bool HandleLiteral(const Node::Kind kind);
	void HandlePlus();
	void HandleMinus();
	bool HandleBinaryOp(const Node::Kind kind, const OpInfo& op_info);
	void HandleNot();
	void HandleIncDec(const Node::Kind prefix_kind, const OpInfo& prefix_op_info,
		const Node::Kind postfix_kind, const OpInfo& postfix_op_info);
	bool HandleDot();
	void HandleOpener(const Node::Kind prefix_kind, const Node::Kind postfix_kind);
	bool HandleCloser(const bool is_conditional_expression);
	bool HandleComma();
	void HandleOp(const Node::Kind kind, const OpInfo& op_info);
	void AddGroup(const Node::Kind kind, const int initial_arity);
	void AddOp(const Node::Kind kind, const OpInfo& op_info);
	void AddOperand(const Node::Kind kind);
	void AddError(const std::string& message);

	void IncrementNodeCount()
	{
		if (groups_.size())
		{
                  Expr& group = operators_[std::size_t(groups_.back())];
			++(group.count);
		}
	}

	void Next()
	{
		if (index_ < (int)tokens_.size() - 1)
                  token_ = &tokens_[std::size_t(++index_)];
	}

	void Undo()
	{
          if (--index_ >= 0)
            token_ = &tokens_[std::size_t(index_)];
          else
            token_ = nullptr;
	}
};


} // namespace vm
} // namespace fetch


#endif
