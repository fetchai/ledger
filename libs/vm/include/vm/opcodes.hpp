#ifndef OPCODES__HPP
#define OPCODES__HPP
#include <cstdint>


namespace fetch {
namespace vm {


enum class Opcode: uint16_t
{
	Unknown,
	Jump,
	JumpIfFalse,
	PushString,
	PushConstant,
	PushVariable,
	VarDeclare,
	VarDeclareAssign,
	Assign,
	Discard,
	Destruct,
	InvokeUserFunction,
	ForRangeInit,
	ForRangeIterate,
	ForRangeTerminate,
	Break,
	Continue,
	Return,
	ReturnValue,
	ToInt8,
	ToByte,
	ToInt16,
	ToUInt16,
	ToInt32,
	ToUInt32,
	ToInt64,
	ToUInt64,
	ToFloat32,
	ToFloat64,
	EqualOp,
	NotEqualOp,
	LessThanOp,
	LessThanOrEqualOp,
	GreaterThanOp,
	GreaterThanOrEqualOp,
	AndOp,
	OrOp,
	NotOp,
	AddOp,
	SubtractOp,
	MultiplyOp,
	DivideOp,
	UnaryMinusOp,
	AddAssignOp,
	SubtractAssignOp,
	MultiplyAssignOp,
	DivideAssignOp,
	PrefixIncOp,
	PrefixDecOp,
	PostfixIncOp,
	PostfixDecOp,
	IndexedAssign,
	IndexOp,
	IndexedAddAssignOp,
	IndexedSubtractAssignOp,
	IndexedMultiplyAssignOp,
	IndexedDivideAssignOp,
	IndexedPrefixIncOp,
	IndexedPrefixDecOp,
	IndexedPostfixIncOp,
	IndexedPostfixDecOp,
	CreateMatrix,
	CreateArray,

	// Custom opcodes
	PrintInt32,
	PrintStr,	
	CreateIntPair,
	IntPairFirst,	
	IntPairSecond,
	Fib
};


} // namespace vm
} // namespace fetch


#endif
