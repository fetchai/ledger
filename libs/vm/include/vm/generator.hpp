#ifndef GENERATOR__HPP
#define GENERATOR__HPP
#include "vm/node.hpp"
#include "vm/defs.hpp"


namespace fetch {
namespace vm {


class Generator
{
public:
	Generator() {}
	~Generator() {}
	void Generate(const BlockNodePtr& root, const std::string& name, Script& script);

private:

	struct Scope
	{
		std::vector<Index>	objects;
	};

	struct Loop
	{
		int					scope_number;
		std::vector<Index> 	continue_pcs;
		std::vector<Index> 	break_pcs;
	};

	Script					script_;
	std::vector<Scope>		scopes_;
	std::vector<Loop>		loops_;
	Script::Function*		function_;

	void CreateFunctions(const BlockNodePtr& root);
	void HandleBlock(const BlockNodePtr& node);
	void HandleFunctionDefinitionStatement(const BlockNodePtr& node);
	void HandleWhileStatement(const BlockNodePtr& node);
	void HandleForStatement(const BlockNodePtr& node);
	void HandleIfStatement(const NodePtr& node);
	void HandleVarStatement(const NodePtr& node);
	void HandleReturnStatement(const NodePtr& node);
	void HandleBreakStatement(const NodePtr& node);
	void HandleContinueStatement(const NodePtr& node);
	void HandleAssignmentStatement(const ExpressionNodePtr& node);
	void HandleLHSExpression(const ExpressionNodePtr& lhs,
		const Opcode override_opcode, const ExpressionNodePtr& rhs);
	void HandleExpression(const ExpressionNodePtr& node);
	void HandleIdentifier(const ExpressionNodePtr& node);
	void HandleInteger32(const ExpressionNodePtr& node);
	void HandleUnsignedInteger32(const ExpressionNodePtr& node);
	void HandleInteger64(const ExpressionNodePtr& node);
	void HandleUnsignedInteger64(const ExpressionNodePtr& node);
	void HandleSinglePrecisionNumber(const ExpressionNodePtr& node);
	void HandleDoublePrecisionNumber(const ExpressionNodePtr& node);
	void HandleString(const ExpressionNodePtr& node);
	void HandleTrue(const ExpressionNodePtr& node);
	void HandleFalse(const ExpressionNodePtr& node);
	void HandleNull(const ExpressionNodePtr& node);
	void HandleIncDecOp(const ExpressionNodePtr& node);
	void HandleOp(const ExpressionNodePtr& node);
	TypeId TestArithmeticTypes(const ExpressionNodePtr& node);
	void HandleIndexOp(const ExpressionNodePtr& node);
	void HandleDotOp(const ExpressionNodePtr& node);
	void HandleInvokeOp(const ExpressionNodePtr& node);
	void ScopeEnter();
	void ScopeLeave(BlockNodePtr block_node);
};


} // namespace vm
} // namespace fetch


#endif
