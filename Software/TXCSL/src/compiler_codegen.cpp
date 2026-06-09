// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/csl/compiler_codegen.hpp"

namespace tx::csl {

CompileResult Compiler::CodeGen_impl::run() {
	compile_impl();
	m_bin.registerCount = hs.countMax();
	m_bin.commandCount = m_commandBufferSize;
	return CompileResult{
		.commandCount = m_commandBufferSize,
		.constantCount = m_constantBufferSize,
		.variableCount = static_cast<tx::u32>(m_variableBuffer.size()),
		.variableNames = std::move(m_variableBuffer)
	};
}

void Compiler::CodeGen_impl::compile_impl() {
	stageInit_impl();

	stageBracket_impl(); // compose bracket structure

	tokenlize_impl(
	    m_source,
	    0,
	    m_bracketBuffer); // tokenlize outer most expression
	stackPush_impl(
	    m_expressionBuffer.size() - 1,
	    m_tokenBuffer.size() - 1,
	    registerNew());

	while (m_stackPtr > 0) {
		compileExpression_impl();
	}
}

void Compiler::CodeGen_impl::stageInit_impl() {
	// reserve buffers
	m_bracketBuffer.reserve(64);
	m_variableBuffer.reserve(64);
	m_expressionBuffer.reserve(64);
	m_tokenBuffer.reserve(256);

	// reserve handle system
	hs.reserve(64);
	hs.reserveDeletion(64);
}

void Compiler::CodeGen_impl::stageBracket_impl() {
	BracketParser_impl bracketParser(m_source, m_bracketBuffer);
	if (!bracketParser.run()) { // DevNote: if stack overflow - error
		return;
	}
}

void Compiler::CodeGen_impl::compileExpression_impl() {
	const tx::i32 end = m_stackPtr == 1 ? -1 : stackParent_impl().tokenIndex;
	const tx::i32 begin = stackTop_impl().tokenIndex;

	if (!((begin - end) % 2)) { // token count not odd: error
		return; // DevNote: error handling: unary operators
	}

	tx::u8 resultReg = stackTop_impl().context.resultRegister;
	m_context.resultReg = resultReg;
	tx::u8 majorExprReg = registerNew();

	// compilation loop
	tx::i32 current = begin;
	// find the next minor operation token
	tx::i32 minorPos = findNextMinorOperator_impl(current, end);

	// first major expression - assign directly to resultReg
	m_context.isFirstMajorExpr = 1;
	bool breakFlag = compileMajorExpression_impl(current, minorPos, majorExprReg);
	if (breakFlag) { // interrupt - expand nested expression
		// since it's the first major expression, there's nothing to do
		return;
	}

	pushCommand_impl( // merge result of major expression to resultReg
	    resultReg,
	    OperationType_impl::Assign,
	    makeValueID_impl(majorExprReg, false),
	    tx::InvalidU8);

	// no need to add merge command, because the compile major expression above directly uses the resultReg
	if (minorPos == end) { // then the entire expression only have one major expression - return
		registerFree(majorExprReg);
		exitExpression_impl();
		return;
	}
	current = minorPos - 1;

	// the operation with the next major expression
	OperationType_impl operation = m_tokenBuffer[minorPos].getOperatorType();
	m_context.operation = operation;
	m_tokenBuffer.pop_back(); // delete the minor operator token
	stackTop_impl().tokenIndex--;

	m_context.isFirstMajorExpr = 0;
	while (current > end) {
		minorPos = findNextMinorOperator_impl(current, end);

		breakFlag = compileMajorExpression_impl(current, minorPos, majorExprReg);
		if (breakFlag) { // interrupt - expand nested expression
			//handleExpressionExpansion_impl(resultReg, operation);
			// this function will be called in `handleExpressionExpansionMajor_impl` instead
			// see "The state problem - handle expression expansion order problem"
			return;
		}

		pushCommand_impl( // merge result of major expression to resultReg
		    resultReg,
		    operation,
		    makeValueID_impl(resultReg, false),
		    makeValueID_impl(majorExprReg, false));
		if (minorPos > end) { // if not at the end
			operation = m_tokenBuffer[minorPos].getOperatorType();
			m_context.operation = operation;
			m_tokenBuffer.pop_back(); // delete the minor operator token
			stackTop_impl().tokenIndex--;
		}

		current = minorPos - 1;
	}

	// clean up
	registerFree(majorExprReg);
	exitExpression_impl();
}

tx::i32 Compiler::CodeGen_impl::findNextMinorOperator_impl(tx::i32 current, tx::i32 end) {
	while (current > end) {
		if (isMinorOperationToken_impl(m_tokenBuffer[current])) {
			return current;
		}
		current--;
	}
	return end;
}

void Compiler::CodeGen_impl::exitExpression_impl() {
	const tx::u32 exprBufferEnd =
	    m_stackPtr == 1 ? 0 : stackParent_impl().tokenIndex + 1; // inclusive for erase
	m_expressionBuffer.erase(
	    m_expressionBuffer.begin() + exprBufferEnd,
	    m_expressionBuffer.end());

	stackPop_impl();
}

void Compiler::CodeGen_impl::handleExpressionExpansion_impl(tx::u8 resultReg, OperationType_impl operation) {
	m_tokenBuffer.push_back(
	    makeOperatorToken_impl(operation));

	// make a new register, assign the value of resultReg to it.
	// this new register will be a temporary register, just to store the old value of resultReg
	tx::u8 tempReg = registerNew();
	pushCommand_impl( // assign the value of resultReg to tempReg
	    tempReg,
	    OperationType_impl::Assign,
	    makeValueID_impl(
	        resultReg, false),
	    tx::InvalidU8);

	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Register,
	    .index = tempReg });
	// update stack
	stackTop_impl().tokenIndex += 2; // this is stack top because this will be called by the `handleExpressionExpensionMajor_impl` before expanding the new expression
	// see "The state problem - handle expression expansion order problem"
}

bool Compiler::CodeGen_impl::compileMajorExpression_impl(tx::i32 begin, tx::i32 end, tx::u8 resultReg) {
	tx::i32 current = begin;

	Token_impl currentToken = m_tokenBuffer[current];

	// first token (must be operand)
	if (currentToken.isOperator()) {
		// DevNote: error
		return 1;
	}
	if (currentToken.isExpression()) { // it is an expression -> expand it
		handleExpressionExpansionMajorBegin_impl(
		    currentToken,
		    resultReg);
		return 1;
	} else { // assign it directly to resultReg
		pushCommand_impl(
		    resultReg,
		    OperationType_impl::Assign,
		    makeValueID_impl(
		        currentToken.index,
		        currentToken.isBuffer(),
		        currentToken.isVariable()),
		    tx::InvalidU8);
		if (currentToken.isRegister())
			registerFree(currentToken.index);
	}
	current--;

	// looping the rest
	// each iteration consumes 2 token: operator token and operand token
	// The entire thing will be separated like this:
	// 0         +         1         -         2         *         3
	// [operand] [operator][operand] [operator][operand] [operator][operand] ...
	// first     iteration 1         iteration 2         iteration 3         ...
	while (current > end) {
		// operator
		currentToken = m_tokenBuffer[current];
		if (!currentToken.isOperator()) {
			// DevNote: error
			break;
		}
		OperationType_impl operation = currentToken.getOperatorType();

		current--;
		if (!(current > end)) {
			// DevNote: error
			break;
		}

		// the next operand
		currentToken = m_tokenBuffer[current];
		if (currentToken.isOperator()) {
			// DevNote: error
			break;
		}
		if (currentToken.isExpression()) { // it is an expression -> expand it
			handleExpressionExpansionMajor_impl(
			    currentToken,
			    current,
			    operation,
			    resultReg);
			return 1;
		} else {
			pushCommand_impl(
			    resultReg,
			    operation,
			    makeValueID_impl(resultReg, false),
			    makeValueID_impl(
			        currentToken.index,
			        currentToken.isBuffer(),
			        currentToken.isVariable()));
			if (currentToken.isRegister())
				registerFree(currentToken.index);
		}
		current--;
	}

	// clean up
	m_tokenBuffer.erase(
	    m_tokenBuffer.begin() + end + 1,
	    m_tokenBuffer.end());
	stackTop_impl().tokenIndex = m_tokenBuffer.size() - 1;

	return 0;
}

void Compiler::CodeGen_impl::handleExpressionExpansionMajorBegin_impl(
    Token_impl currentToken,
    tx::u8 resultReg) {
	Expression_impl expr = m_expressionBuffer[currentToken.index];

	// just reuse the resultReg register, since it's just the first token, it was fresh before
	m_tokenBuffer.pop_back();
	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Register,
	    .index = resultReg });

	// handle expression expansion in the `compileExpression` layer - see "The state problem - handle expression expansion order problem"
	if (!m_context.isFirstMajorExpr)
		handleExpressionExpansion_impl(m_context.resultReg, m_context.operation);

	// expanding (tokenlize) the expression
	expandExpression_impl(expr, resultReg);
}

void Compiler::CodeGen_impl::handleExpressionExpansionMajor_impl(
    Token_impl currentToken,
    tx::i32 current,
    OperationType_impl operation,
    tx::u8 resultReg) {
	Expression_impl expr = m_expressionBuffer[currentToken.index];

	// delete the processed tokens
	m_tokenBuffer.erase(m_tokenBuffer.begin() + current, m_tokenBuffer.end());

	// append the 3 new tokens
	tx::u8 expressionResultRegister = registerNew();
	m_tokenBuffer.push_back(Token_impl{ // the register for the expression
	                                    .type = TokenType_impl::Register,
	                                    .index = expressionResultRegister });
	m_tokenBuffer.push_back(makeOperatorToken_impl(operation)); // the operator between them
	m_tokenBuffer.push_back(Token_impl{
	    .type = TokenType_impl::Register,
	    .index = resultReg });

	// update stack
	stackTop_impl().tokenIndex = m_tokenBuffer.size() - 1;

	// handle expression expansion in the `compileExpression` layer - see "The state problem - handle expression expansion order problem"
	if (!m_context.isFirstMajorExpr)
		handleExpressionExpansion_impl(m_context.resultReg, m_context.operation);

	// expanding (tokenlize) the expression
	expandExpression_impl(expr, expressionResultRegister);
}

void Compiler::CodeGen_impl::expandExpression_impl(Expression_impl expr, tx::u8 resultReg) {
	// tokenlize
	tokenlize_impl(m_bracketBuffer[expr.bracketIndex]);

	// parse


	// push stack
	stackPush_impl(
	    m_expressionBuffer.size() - 1,
	    m_tokenBuffer.size() - 1,
	    resultReg);
}

void Compiler::CodeGen_impl::tokenlize_impl(
    std::string_view source,
    tx::u32 sourceOffset,
    std::span<const Bracket_impl> bracketBuffer) {

	//tx::u32 oldExpressionBufferSize = m_expressionBuffer.size(),
	tx::u32 oldTokenBufferSize = m_tokenBuffer.size();

	Tokenlizer_impl tokenlizer(
	    source, sourceOffset, bracketBuffer,
	    m_bin.constantBuffer, m_constantBufferSize,
	    m_variableBuffer,
	    m_expressionBuffer,
	    m_tokenBuffer);
	m_constantBufferSize = tokenlizer.run().constantOffset;

	// if (oldExpressionBufferSize < m_expressionBuffer.size())
	// 	std::reverse(
	// 	    m_expressionBuffer.begin() + oldExpressionBufferSize,
	// 	    m_expressionBuffer.end());
	if (oldTokenBufferSize < m_tokenBuffer.size())
		std::reverse(
		    m_tokenBuffer.begin() + oldTokenBufferSize,
		    m_tokenBuffer.end());
}

void Compiler::CodeGen_impl::tokenlize_impl(Bracket_impl bracket) {
	tokenlize_impl(
	    findBracketSource_impl(bracket),
	    findBracketSourceOffset_impl(bracket),
	    findBracketChilds_impl(bracket));
}

void Compiler::CodeGen_impl::pushCommand_impl(
    tx::u8 dest,
    OperationType_impl operation,
    tx::u8 vala,
    tx::u8 valb) {
	m_bin.commandBuffer[m_commandBufferSize] = Command{
		.m_dest = dest,
		.m_operation = operation,
		.m_vala = vala,
		.m_valb = valb
	};
	m_commandBufferSize++;
}

} // namespace tx::csl