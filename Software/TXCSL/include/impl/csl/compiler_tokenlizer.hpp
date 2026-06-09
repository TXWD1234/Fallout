// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/csl/compiler.hpp"

namespace tx::csl {
class Compiler::Tokenlizer_impl {
	/**
	 * This entire class object is responsible for only one operation
	 * *you should not call `run()` multiple times.*
	 * The correct pipeline flow is:
	 * 1. create buffer
	 * 2. create an object
	 * 3. call run()
	 * 4. use buffer
	 * 
	 * The entire class is a state machine. There will be a lot "global" function call within the scope of the class object
	 */

	/**
	 * The order of tokenlization is according to the level of nesting brackets.
	 * It's "lazy", meaning that it will not also tokenlize the content inside the bracket.
	 * When in the stage of CodeGen, any untokenlized expressions (token with type `Expression`)
	 * will be then be tokenlized.
	 */

	// *"State Machine Soup"*
	//                     -- Said TX_Jerry

	// this class is more likely a Lexer instead of a tokenlizer

public:
	Tokenlizer_impl(
	    std::string_view source, tx::u32 sourceOffset, std::span<const Bracket_impl> bracketBuffer, // read only data
	    std::span<Constant_impl> constantBuffer, tx::u32 constantBufferOffset,
	    std::vector<Variable_impl>& variableBuffer,
	    std::vector<Expression_impl>& expressionBuffer,
	    std::vector<Token_impl>& tokenBuffer);

	struct Result {
		tx::u32 constantOffset;
	};
	Result run();

private:
	std::string_view m_source;
	std::span<const Bracket_impl> m_bracketBuffer;
	std::span<Constant_impl> m_constantBuffer;
	std::vector<Variable_impl>& m_variableBuffer;
	std::vector<Expression_impl>& m_expressionBuffer;
	std::vector<Token_impl>& m_tokenBuffer;
	tx::u32 m_sourceOffset;

	// state
	tx::u32 m_index = 0;
	tx::u32 m_nextBracketIndex = 0;

	void parse_impl();
	void parseToken_impl();
	void parseExpression_impl();
	void parseOperation_impl();
	void parseVariable_impl();
	void parseNumber_impl();

	// helpers

	Bracket_impl nextBracket_impl() const { return m_bracketBuffer[m_nextBracketIndex]; }
	tx::u32 nextBracketBegin_impl() const { return nextBracket_impl().range.offset - m_sourceOffset; }
	bool isBracket_impl() const { return nextBracketBegin_impl() == m_index; }
	bool hasIncomingBrackets() const { return m_nextBracketIndex < m_bracketBuffer.size(); }

	static bool isWhiteSpace_impl(char c) { return tx::CharWhiteSpaceGroup::contains(c); }
	void skipWhiteSpace_impl();

	static bool isAlphabet_impl(char c) { return tx::inRange(c, 'a', 'z') || tx::inRange(c, 'A', 'Z'); }
	static bool isVariableNameBegin_impl(char c) { return isAlphabet_impl(c) || c == '_'; }
	static bool isVariableNameBody_impl(char c) { return isVariableNameBegin_impl(c) || tx::inRange(c, '0', '9'); }

	using CharOperationGroup = tx::ValueGroup<
	    char,
	    '+',
	    '-',
	    '*',
	    '/'>;
	static bool isOperation_impl(char c) {
		return CharOperationGroup::contains(c);
	}
	static OperationType_impl getOperationType_impl(char c);

	static bool isNumber_impl(char c) {
		return tx::inRange(c, '0', '9');
	}
	static bool isNumberBody_impl(char c) {
		return tx::inRange(c, '0', '9') || c == '.';
	}

	tx::u32 m_constantBufferSize;
	void bufferConstantPushBack_impl(Constant_impl val);
};
} // namespace tx::csl