#ifndef ANALYSER__HPP
#define ANALYSER__HPP
#include "node.hpp"


namespace fetch {
namespace vm {


class Analyser
{
public:
	Analyser();
	~Analyser() {}
	bool Analyse(const BlockNodePtr& root, std::vector<std::string>& errors);

private:

	void AddError(const Token& token, const std::string& message);
	void BuildBlock(const BlockNodePtr& block_node);
	void BuildFunctionDefinition(const BlockNodePtr& parent_block_node,
		const BlockNodePtr& function_definition_node);
	void BuildWhileStatement(const BlockNodePtr& parent_block_node,
		const BlockNodePtr& while_statement_node);
	void BuildForStatement(const BlockNodePtr& parent_block_node,
		const BlockNodePtr& for_statement_node);
	void BuildIfStatement(const BlockNodePtr& parent_block_node,
		const NodePtr& if_statement_node);
	void AnnotateBlock(const BlockNodePtr& block_node);
	void AnnotateFunctionDefinitionStatement(const BlockNodePtr& function_definition_node);
	bool TestBlock(const BlockNodePtr& block_node);
	void AnnotateWhileStatement(const BlockNodePtr& while_statement_node);
	void AnnotateForStatement(const BlockNodePtr& for_statement_node);
	void AnnotateIfStatement(const NodePtr& if_statement_node);
	void AnnotateVarStatement(const BlockNodePtr& parent_block_node,
		const NodePtr& var_statement_node);
	void AnnotateReturnStatement(const NodePtr& return_statement_node);
	void AnnotateConditionalBlock(const BlockNodePtr& conditional_node);
	bool AnnotateTypeExpression(const ExpressionNodePtr& node);
	bool AnnotateAssignOp(const ExpressionNodePtr& node);
	bool AnnotateAddSubtractAssignOp(const ExpressionNodePtr& node);
	bool AnnotateMultiplyAssignOp(const ExpressionNodePtr& node);
	bool AnnotateDivideAssignOp(const ExpressionNodePtr& node);
	bool IsWriteable(const ExpressionNodePtr& lhs);
	TypePtr IsAddSubtractCompatible(const ExpressionNodePtr& node,
		const ExpressionNodePtr& lhs, const ExpressionNodePtr& rhs);
	TypePtr IsMultiplyCompatible(const ExpressionNodePtr& node,
		const ExpressionNodePtr& lhs, const ExpressionNodePtr& rhs);
	TypePtr IsDivideCompatible(const ExpressionNodePtr& node,
		const ExpressionNodePtr& lhs, const ExpressionNodePtr& rhs);
	bool AnnotateExpression(const ExpressionNodePtr& node);
	bool AnnotateAddSubtractOp(const ExpressionNodePtr& node);
	bool AnnotateMultiplyOp(const ExpressionNodePtr& node);
	bool AnnotateDivideOp(const ExpressionNodePtr& node);
	bool AnnotateUnaryMinusOp(const ExpressionNodePtr& node);
	bool AnnotateEqualityOp(const ExpressionNodePtr& node);
	bool AnnotateRelationalOp(const ExpressionNodePtr& node);
	bool AnnotateBinaryLogicalOp(const ExpressionNodePtr& node);
	bool AnnotateUnaryLogicalOp(const ExpressionNodePtr& node);
	bool AnnotateIncDecOp(const ExpressionNodePtr& node);
	bool AnnotateIndexOp(const ExpressionNodePtr& node);
	bool AnnotateDotOp(const ExpressionNodePtr& node);
	bool AnnotateInvokeOp(const ExpressionNodePtr& node);

	FunctionPtr FindMatchingFunction(const FunctionGroupPtr& fg, const TypePtr& type,
		const std::vector<TypePtr>& input_types,
		std::vector<TypePtr>& output_types);
	TypePtr ConvertType(TypePtr type, TypePtr instantiated_template_type);
	TypePtr FindType(const ExpressionNodePtr& node);
	SymbolPtr FindSymbol(const ExpressionNodePtr& node);
	SymbolPtr SearchSymbolTables(const std::string& name);
	void SetVariable(const ExpressionNodePtr& node, const VariablePtr& variable);
	void SetLV(const ExpressionNodePtr& node, const TypePtr& type);
	void SetRV(const ExpressionNodePtr& node, const TypePtr& type);
	void SetType(const ExpressionNodePtr& node, const TypePtr& type);
	void SetFunction(const ExpressionNodePtr& node, const FunctionGroupPtr& fg,
		const TypePtr& fg_owner, const bool function_invoked_on_instance);
	TypePtr CreatePrimitiveType(const std::string& name, const TypeId id);
	TypePtr CreateTemplateType(const std::string& name, const TypeId id);
	TypePtr CreateTemplateInstantiationType(const std::string& name,
		const TypeId id, const TypePtr& template_type,
		const std::vector<TypePtr>& template_parameter_types);
	TypePtr CreateClassType(const std::string& name, const TypeId id);
	FunctionPtr CreateUserFunction(const std::string& name,
		const std::vector<TypePtr>& parameter_types,
		const std::vector<VariablePtr>& parameter_variables,
		const TypePtr& return_type);
	FunctionPtr CreateOpcodeFunction(const std::string& name,
		const Function::Kind kind, const std::vector<TypePtr>& parameter_types,
		const TypePtr& return_type, const Opcode opcode);
	void AddFunction(const SymbolTablePtr& symbols, const FunctionPtr& function);
	void CreateOpcodeFunction(const std::string& name,
		const std::vector<TypePtr>& parameter_types,
		const TypePtr& return_type,
		const Opcode opcode);
	void CreateOpcodeTypeFunction(const TypePtr& type,
		const std::string& name,
		const std::vector<TypePtr>& parameter_types,
		const TypePtr& return_type,
		const Opcode opcode);
	void CreateOpcodeInstanceFunction(const TypePtr& type,
		const std::string& name,
		const std::vector<TypePtr>& parameter_types,
		const TypePtr& return_type,
		const Opcode opcode);

	static std::string CONSTRUCTOR;

	SymbolTablePtr				symbols_;

	TypePtr						template_instantiation_type_;
	TypePtr						template_parameter_type1_;
	TypePtr						template_parameter_type2_;
	TypePtr						numeric_type_;

	TypePtr						matrix_template_type_;
	TypePtr						array_template_type_;

	TypePtr						void_type_;
	TypePtr						null_type_;
	TypePtr						bool_type_;
	TypePtr						int8_type_;
	TypePtr						byte_type_;
	TypePtr						int16_type_;
	TypePtr						uint16_type_;
	TypePtr						int32_type_;
	TypePtr						uint32_type_;
	TypePtr						int64_type_;
	TypePtr						uint64_type_;
	TypePtr						float32_type_;
	TypePtr						float64_type_;
	TypePtr						matrix_float32_type_;
	TypePtr						matrix_float64_type_;
	TypePtr						string_type_;
	TypePtr 					array_bool_type_;
	TypePtr 					array_int8_type_;
	TypePtr 					array_byte_type_;
	TypePtr 					array_int16_type_;
	TypePtr 					array_uint16_type_;
	TypePtr 					array_int32_type_;
	TypePtr 					array_uint32_type_;
	TypePtr 					array_int64_type_;
	TypePtr 					array_uint64_type_;
	TypePtr 					array_float32_type_;
	TypePtr 					array_float64_type_;
	TypePtr 					array_matrix_float32_type_;
	TypePtr 					array_matrix_float64_type_;
	TypePtr 					array_string_type_;

	BlockNodePtr				root_;
	std::vector<BlockNodePtr>	blocks_;
	std::vector<BlockNodePtr>	loops_;
	FunctionPtr					function_;
	std::vector<std::string>	errors_;
};


} // namespace vm
} // namespace fetch


#endif
