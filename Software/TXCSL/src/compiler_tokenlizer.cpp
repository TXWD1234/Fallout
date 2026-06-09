// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/csl/compiler_tokenlizer.hpp"

namespace tx::csl {

Compiler::Tokenlizer_impl::Tokenlizer_impl(
    std::string_view source, tx::u32 sourceOffset, std::span<const Bracket_impl> bracketBuffer,
    std::span<Constant_impl> constantBuffer, tx::u32 constantBufferOffset,
    std::vector<Variable_impl>& variableBuffer,
    std::vector<Expression_impl>& expressionBuffer,
    std::vector<Token_impl>& tokenBuffer)
    : m_source(source), m_bracketBuffer(bracketBuffer),
      m_constantBuffer(constantBuffer),
      m_variableBuffer(variableBuffer),
      m_expressionBuffer(expressionBuffer),
      m_tokenBuffer(tokenBuffer),
      m_sourceOffset(sourceOffset),
      m_index(0), m_nextBracketIndex(0),
      m_constantBufferSize(constantBufferOffset) {}

Compiler::Tokenlizer_impl::Result Compiler::Tokenlizer_impl::run() {
	parse_impl();
	return Result{
		m_constantBufferSize
	};
}

void Compiler::Tokenlizer_impl::parse_impl() {
	while (m_index < m_source.size()) {
		skipWhiteSpace_impl(); // early return if no white space existing
		if (m_index >= m_source.size()) break;
		parseToken_impl(); // accounting for brackets
	}
}

void Compiler::Tokenlizer_impl::parseToken_impl() {
	char c = m_source[m_index];
	if (hasIncomingBrackets() && // bound check for the bracket array
	    isBracket_impl()) {
		parseExpression_impl();
	} else if (isOperation_impl(c)) {
		parseOperation_impl();
	} else if (isVariableNameBegin_impl(c)) {
		parseVariable_impl();
	} else if (isNumber_impl(c)) { // it's a number or syntax error
		parseNumber_impl();
	} else {
		// DevNote: error
		return;
	}
}

void Compiler::Tokenlizer_impl::parseExpression_impl() {
	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Expression,
	    .index = static_cast<tx::u8>(m_expressionBuffer.size()) });
	m_expressionBuffer.push_back(Expression_impl{
	    nextBracket_impl().index });

	m_index += nextBracket_impl().range.size;
	m_nextBracketIndex++;
}

void Compiler::Tokenlizer_impl::parseOperation_impl() { // DevNote: add multi character operator
	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Operator,
	    .index = static_cast<tx::u8>(getOperationType_impl(m_source[m_index])) });
	m_index++;
}

void Compiler::Tokenlizer_impl::parseVariable_impl() {
	tx::u32 begin = m_index;
	while (m_index < m_source.size() && isVariableNameBody_impl(m_source[m_index])) m_index++; // resulted one index after the variable

	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Variable,
	    .index = static_cast<tx::u8>(m_variableBuffer.size()) });
	m_variableBuffer.push_back(m_source.substr(
	    begin, m_index - begin));
}

void Compiler::Tokenlizer_impl::parseNumber_impl() {
	Constant_impl val;
	const char* begin = m_source.data() + m_index;
	auto [ptr, ec] = std::from_chars(
	    begin,
	    m_source.data() + m_source.size(),
	    val);
	if (ec != std::errc{}) {
		// DevNote: error
		return;
	}

	m_index += static_cast<tx::u32>(ptr - begin);

	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Constant,
	    .index = static_cast<tx::u8>(m_constantBufferSize) });
	bufferConstantPushBack_impl(val);
}

void Compiler::Tokenlizer_impl::skipWhiteSpace_impl() {
	while (m_index < m_source.size() && isWhiteSpace_impl(m_source[m_index])) { m_index++; }
}

OperationType_impl Compiler::Tokenlizer_impl::getOperationType_impl(char c) {
	switch (c) {
	case '+': return OperationType_impl::Add;
	case '-': return OperationType_impl::Subtract;
	case '*': return OperationType_impl::Multiply;
	case '/': return OperationType_impl::Divide;
	};
	return OperationType_impl::Invalid;
}

void Compiler::Tokenlizer_impl::bufferConstantPushBack_impl(Constant_impl val) {
	if (m_constantBufferSize >= m_constantBuffer.size()) {
		// DevNote: error
		return;
	}
	m_constantBuffer[m_constantBufferSize] = val;
	m_constantBufferSize++;
}

} // namespace tx::csl