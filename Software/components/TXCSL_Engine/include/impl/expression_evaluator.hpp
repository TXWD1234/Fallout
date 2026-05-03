// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"
#include "tx/data.h"
#include "tx/addons.h"
#include "tx/bit_trick.hpp"
#include <span>
#include <string>
#include <string_view>
#include <charconv>


/**
 * This evaluator don't manage memory, it requires you to provide memory for it.
 * If the memory buffer you provided is not enough, evaluation will be interrupted (DevNote: add error handling) 
 * 
 * Specification:
 * Max size of ConstantBuffer and VariableBuffer: 64
 * Optimal size of CommandBuffer: 256
 */
class ExpressionEvaluator {
	/**
	 * Dev Note
	 * 
	 * Terminology:
	 * - `Major Operation`: multiply or divide
	 * - `Minor Operation`: add or subtract
	 * 
	 * Idea:
	 * - compile(): make operation queue / command buffer
	 * - evaluate(): calculate / execute the composed command buffer
	 * 
	 * ## Command buffer
	 * The register
	 * Opaque struct `Operation`: set a register as the result of an `Expression`
	 */
private:
	enum class OperationType_impl : tx::u8;

public:
	using num = float;

	// @note size == 4 byte
	struct Command {
		tx::u8 m_dest; // index of the destination register
		OperationType_impl m_operation; // index of the operation
		tx::u8 m_vala; // identifier of the value to perform the operation
		tx::u8 m_valb; // identifier of the value to perform the operation
		/**
		 * Note of m_vala / m_valb:
		 * max value: 64
		 * The first 2 bits are boolean flags
		 * First bit: isFromBuffer; if is 0, then the value is from registers
		 * Second bit: isVariableBuffer; if is 0, then the value is from constant buffer
		 */
	};
	struct Expression {
		Expression(
		    std::span<Command> in_commandBuffer,
		    std::span<num> in_constantBuffer,
		    std::span<num> in_variableBuffer)
		    : commandBuffer(in_commandBuffer),
		      constantBuffer(in_constantBuffer),
		      variableBuffer(in_variableBuffer) {}
		std::span<Command> commandBuffer;
		std::span<num> constantBuffer;
		std::span<num> variableBuffer;
		tx::u32 registerCount; // the required registers when evaluating
	};

	struct CompileResult {
		tx::u32 commnadCount;
		tx::u32 constantCount;
		tx::u32 variableCount;
		std::vector<std::string_view> varibaleNames; // for the caller of compile() to fill in the variabe buffer
	};

public:
	static CompileResult compile(std::string_view source, Expression& result) {
		compile_impl(source, result);
		return CompileResult{};
	}
	// static num evaluate(std::string_view source) {

	// 	Expression bin;
	// 	compile(source, bin);
	// 	return evaluate_impl(bin);
	// }

private:
	// operation table
	static void add_impl(num& dest, num a, num b) { dest = a + b; }
	static void sub_impl(num& dest, num a, num b) { dest = a - b; }
	static void mul_impl(num& dest, num a, num b) { dest = a * b; }
	static void div_impl(num& dest, num a, num b) { dest = a / b; }

	static void assign_impl(num& dest, num val) { dest = val; }

	enum class OperationType_impl : tx::u8 {
		Add = 1,
		Subtract = 2,
		Multiply = 3,
		Divide = 4,
		Assign = 5,
	};
	// clang-format off

	static void performOperation_impl(OperationType_impl operation, num& dest, num a, num b) {
		switch (operation) {
		case OperationType_impl::Add:      add_impl(dest, a, b); break;
		case OperationType_impl::Subtract: sub_impl(dest, a, b); break;
		case OperationType_impl::Multiply: mul_impl(dest, a, b); break;
		case OperationType_impl::Divide:   div_impl(dest, a, b); break;
		case OperationType_impl::Assign:   assign_impl(dest, a); break;
		}
	}
	// clang-format on


private:
	// The Compilation logic
	/**
	 * The design pattern of the compile stage:
	 * Each stage will have it's own class.
	 * That class will be a temporary object, that will outputs a certain result of the stage
	 * The purpose of the class is to maintain it's own internal state (because they are all state machin design)
	 */

	/**
	 * The compilation pipeline flow is:
	 * 1. create bracket structure
	 * 2. tokenlize (only the top layer)
	 * 3. codegen
	 * 		- when encountered a token of type `Expression`, goto setp 2
	 * 
	 * The process of nested expressions will be lazy,
	 * meaning that it will not be processed until necessary
	 */

	struct Range_impl {
		tx::u32 offset, size;
	};

	struct Bracket_impl {
		Range_impl range; // include the "()"
		tx::u16 childCount;
		tx::u16 index;
	};
	/**
	 * @note
	 * stack size == 8, therefore the nested parentheses limit is 8
	 */
	class BracketParser_impl {
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
	public:
		BracketParser_impl(std::string_view source, std::vector<Bracket_impl>& buffer)
		    : m_source(source), m_buffer(buffer) {}

		/**
		 * @return success
		 * 
		 * Performance:
		 * one O(N) scan
		 */
		bool run() {
			for (tx::u32 i = 0; i < m_source.size(); i++) {
				if (m_source[i] == '(') { // enter new bracket range
					if (!stateEnterBracket_impl(i)) return false;
				} else if (m_source[i] == ')') { // exit current bracket
					if (!stateExitBracket_impl(i)) return false;
				}
			}
			return true;
		}

	private:
		inline static constexpr const tx::u8 stackCapacity = 8;

	private:
		tx::u32 m_stack[stackCapacity]; // stores the index of entry in `m_buffer`
		std::string_view m_source;
		std::vector<Bracket_impl>& m_buffer;
		tx::u8 m_stackPtr = 0;

		// logic
		/**
		 * Convension for logic clearity and correctness:
		 * stack operations (stack push and stack pop) must happen in the function strictly according to their role
		 * - stack push must at the begining of the function (after integrity check of course)
		 * - stack pop must at the end of the function
		 */

		bool stateEnterBracket_impl(tx::u32 index) {
			if (!stackCheckOverflow_impl()) return false; // stack overflow
			stackPush_impl(m_buffer.size());
			if (m_stackPtr > 1) currentParent_impl().childCount++;
			m_buffer.push_back(Bracket_impl{
			    Range_impl{ index, tx::u32{ 0 } },
			    tx::u16{ 0 }, static_cast<tx::u16>(m_buffer.size()) });
			return true;
		}
		bool stateExitBracket_impl(tx::u32 index) {
			if (m_stackPtr == 0) return false; // syntax error
			current_impl().range.size = index - current_impl().range.offset + 1; // plus one for the extra ")" at the end
			stackPop_impl();
			return true;
		}

		// stack operations

		// call this when going to push stack
		// returned boolean means "is the stack capable of pushing a new entry?"
		bool stackCheckOverflow_impl() const { return m_stackPtr < stackCapacity; }
		void stackPush_impl(tx::u32 val) {
			m_stack[m_stackPtr] = val;
			m_stackPtr++;
		}
		void stackPop_impl() {
			m_stackPtr--;
		}

		tx::u32 stackTop() const { return m_stack[m_stackPtr - 1]; }

		Bracket_impl& currentParent_impl() { return m_buffer.operator[](m_stack[m_stackPtr - 2]); }
		Bracket_impl& current_impl() { return m_buffer.operator[](stackTop()); }
	};


	enum class TokenType_impl : tx::u8 {
		Constant,
		Variable,
		Operator,
		Expression,
		Register
	};
	struct Token_impl {
		TokenType_impl type;
		tx::u8 index; // index at the corrisponding token object array of the type
		// if type is Operation, then index is just the enum value
		OperationType_impl getOperatorType() const { return static_cast<OperationType_impl>(index); }
		void setOperatorType(OperationType_impl type) { index = static_cast<tx::u16>(type); }
		bool isOperator() const { return type == TokenType_impl::Operator; }
		bool isConstant() const { return type == TokenType_impl::Constant; }
		bool isVaraible() const { return type == TokenType_impl::Variable; }
		bool isExpression() const { return type == TokenType_impl::Expression; }
		bool isRegister() const { return type == TokenType_impl::Register; }
	};
	using Constant_impl = num;
	using Variable_impl = std::string_view;
	struct Expression_impl {
		tx::u32 bracketIndex; // global index
	};

	class Tokenlizer_impl {
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

	public:
		Tokenlizer_impl(
		    std::string_view source, tx::u32 sourceOffset, std::span<const Bracket_impl> bracketBuffer, // read only data
		    std::span<Constant_impl> constantBuffer, tx::u32 constantBufferOffset,
		    std::vector<Variable_impl>& variableBuffer,
		    std::vector<Expression_impl>& expressionBuffer,
		    std::vector<Token_impl>& tokenBuffer)
		    : m_source(source), m_bracketBuffer(bracketBuffer), // read only data
		      m_constantBuffer(constantBuffer), // appending from the back
		      m_variableBuffer(variableBuffer),
		      m_expressionBuffer(expressionBuffer),
		      m_tokenBuffer(tokenBuffer),
		      m_sourceOffset(sourceOffset),

		      m_index(0), m_nextBracketIndex(0),
		      m_constantBufferSize(constantBufferOffset) {}

		struct Result {
			tx::u32 constantOffset;
		};
		Result run() {
			parse_impl();
			return Result{
				m_constantBufferSize
			};
		}

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

		void parse_impl() {
			while (m_index < m_source.size()) {
				skipWhiteSpace_impl(); // early return if no white space existing
				if (m_index >= m_source.size()) break;
				parseToken_impl(); // accounting for brackets
			}
		}

		// @return break, because reached bracket
		void parseToken_impl() {
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



		void parseExpression_impl() {
			m_tokenBuffer.push_back(Token_impl{
			    .type = TokenType_impl::Expression,
			    .index = static_cast<tx::u8>(m_expressionBuffer.size()) });
			m_expressionBuffer.push_back(Expression_impl{
			    nextBracket_impl().index });

			m_index += nextBracket_impl().range.size;
			m_nextBracketIndex++;
		}
		void parseOperation_impl() { // DevNote: add multi character operator
			m_tokenBuffer.push_back(Token_impl{
			    .type = TokenType_impl::Operator,
			    .index = static_cast<tx::u8>(getOperationType_impl(m_source[m_index])) });
			m_index++;
		}
		void parseVariable_impl() {
			tx::u32 begin = m_index;
			while (m_index < m_source.size() && isVariableNameBody_impl(m_source[m_index])) m_index++; // resulted one index after the variable

			m_tokenBuffer.push_back(Token_impl{
			    .type = TokenType_impl::Variable,
			    .index = static_cast<tx::u8>(m_variableBuffer.size()) });
			m_variableBuffer.push_back(m_source.substr(
			    begin, m_index - begin));
		}
		void parseNumber_impl() {
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

		// helpers

		Bracket_impl nextBracket_impl() const { return m_bracketBuffer[m_nextBracketIndex]; }
		tx::u32 nextBracketBegin_impl() const { return nextBracket_impl().range.offset - m_sourceOffset; }
		bool isBracket_impl() const { return nextBracketBegin_impl() == m_index; }
		bool hasIncomingBrackets() const { return m_nextBracketIndex < m_bracketBuffer.size(); }

		static bool isWhiteSpace_impl(char c) { return tx::CharWhiteSpaceGroup::contains(c); }
		void skipWhiteSpace_impl() {
			while (m_index < m_source.size() && isWhiteSpace_impl(m_source[m_index])) { m_index++; }
		}

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
		static OperationType_impl getOperationType_impl(char c) {
			switch (c) {
			case '+': return OperationType_impl::Add;
			case '-': return OperationType_impl::Subtract;
			case '*': return OperationType_impl::Multiply;
			case '/': return OperationType_impl::Divide;
			};
		}

		static bool isNumber_impl(char c) {
			return tx::inRange(c, '0', '9') || c == '-';
		}
		static bool isNumberBody_impl(char c) {
			return tx::inRange(c, '0', '9') || c == '.';
		}

		tx::u32 m_constantBufferSize;
		void bufferConstantPushBack_impl(Constant_impl val) {
			if (m_constantBufferSize >= m_constantBuffer.size()) {
				// DevNote: error
				return;
			}
			m_constantBuffer[m_constantBufferSize] = val;
			m_constantBufferSize++;
		}
	};



	class Compiler_impl {
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
		 * all heap allocation here will be temporary. they will be freed after compilation
		 */

		/**
		 * DevNote:
		 * Add reserve
		 * Calculate allocated size after reserve
		 * Assert if current heap capacity is not enough
		 */

		/**
		 * The order of evaluation in implmentation is from right to left
		 * (which is from the back of the array poping back)
		 * All the buffer will reversed. And the new expressions will be appended
		 * after the current expression section, just like stack.
		 */

		/**
		 * Buffers
		 * 
		 * BracketBuffer
		 * Bracket structure generated by BracketParser
		 * Forms a stack like tree structure
		 * Accessability: composed by BracketParser, read only throughout
		 * Lifetime: vector created during compilation, one time init, will be destroied after compilation
		 * Type: compilation data (meta)
		 * 
		 * ConstantBuffer
		 * Store all constant literals / values
		 * Accessability: composed by Tokenlizer, will be write by Compiler for constant pre-evaluatioon
		 * Lifetime: managed by user, exist as span during compilation
		 * Type: compile result data (evaluator usage)
		 * (inside `m_bin`)
		 * 
		 * VariableBuffer
		 * Store the name (std::string_view, source is in `m_source`) of the variables
		 * Accessability: composed by Tokenlizer, forwarding in Compiler
		 * Lifetime: vector created during compilation, ownership will be taken by the user after compilation (in CompileResult)
		 * Type: compile result data (user usage)
		 * DevNote: maybe make my own string_view specificly for this structure to save memory?
		 * 
		 * ExpressionBuffer
		 * Store the expressions (brackets) in a given expression. brackets are represented by their global index in compilation
		 * Acts like stack, and will be constantly appending and deleting.
		 * Accessability: composed by Tokenlizer, read and transfer in Compiler
		 * Lifetime: vector created during compilation, will be destoried after compilation
		 * Type: compilation data (meta)
		 * (reversed)
		 * 
		 * TokenBuffer
		 * Store the tokens generated by Tokenlizer.
		 * Acts like stack, and will be constantly appending and deleting.
		 * Accessability: composed by Tokenlizer, read and transfer in Compiler
		 * Lifetime: vector created during compilation, will be destoried after compilation
		 * Type: compilation data
		 * (reversed)
		 * 
		 * 
		 * Note:
		 * ConstantBuffer and VariableBuffer will accumulate during compilation. 
		 * They will result a structure that have the same sequence as the BracketBuffer.
		 * 
		 * ExpressionBuffer and TokenBuffer will be constantly generating and deleting.
		 * They acts like stack, where the base data will stay, and their child object (expression / bracket) will expand after them.
		 * The processed data will be deleted.
		 * Because the stack like design require them to operate from the back, so in order to
		 * follow normal order of operation (left to right), we need to reverse the data for every section (every expression)
		 */

		/**
		 * Pipeline
		 * 
		 * 1. Raw string
		 * 2. BracketParser - compose bracket structure
		 * 3. Tokenlizer    - tokenlize raw string. identify and convert value
		 * 4. Compiler      - transform tokens into instructions, flatten the operation tree
		 */
	public:
		Compiler_impl(
		    std::string_view source,
		    Expression& bin) : m_bin(bin), m_source(source) {}

		// top layer function to be called
		void run() {
			compile_impl();
		}

	private:
		Expression m_bin;
		std::string_view m_source;

		// buffers

		std::vector<Bracket_impl> m_bracketBuffer;
		std::vector<Variable_impl> m_variableBuffer;
		std::vector<Expression_impl> m_expressionBuffer; // this acts like the stack
		std::vector<Token_impl> m_tokenBuffer; // this acts like stack

		tx::u32 m_constantBufferSize = 0;

		// Sub systems

		// register

		tx::HandleSystem<tx::u8> hs;
		tx::u8 registerNew() { return hs.addHandle(); }
		void registerFree(tx::u8 id) { hs.deleteHandle(id); }


		// this is basically the main function of compilation
		void compile_impl() {
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

		// stages

		void stageInit_impl() {
			// reserve buffers
			m_bracketBuffer.reserve(64);
			m_variableBuffer.reserve(64);
			m_expressionBuffer.reserve(64);
			m_tokenBuffer.reserve(256);

			// reserve handle system
			hs.reserve(64);
			hs.reserveDeletion(64);
		}

		void stageBracket_impl() {
			BracketParser_impl bracketParser(m_source, m_bracketBuffer);
			if (!bracketParser.run()) { // DevNote: if stack overflow - error
				return;
			}
		}

		// compilation
		struct StackObject_impl {
			tx::u32
			    expressionIndex,
			    tokenIndex;
			struct Context {
				tx::u32 resultRegister;
				// // if there's no nested expression, these 2 variables will never be used
				// tx::u32 index = tx::InvalidU32; // for the returned expression to resume the previous process
				// tx::u8 expandedExpressionRegister;
			} context;
		};
		inline static constexpr const tx::u8 stackCapacity = tx::u8{ 8 };
		std::array<StackObject_impl, stackCapacity> m_stack; // stores the index of the stack top (end of section) in token
		tx::u32 m_stackPtr = 0;

		/**
		 * all compile functions will output to the end of m_bin.commandQueue
		 * 
		 * all operations can only apply regarding the current stack object / the stack top
		 * 
		 * ALL INDICES ARE REVERSED within expression. traversal is also reversed
		 * 
		 * The expanding logic:
		 * When a new expression is found in the parent expression, the compilation of the parent expression will be interrupted.
		 * When expanding in the process of compile, it have to make sure that all the context is prepared for the next `compileExpression_impl` call
		 * Additional, it need to save it's own internal state, so that when the expanded expression is done compiling,
		 * it can continue with the value of the expanded expression.
		 * 
		 * Detail:
		 * When encountered a expression:
		 * 1. pop the already processed tokens, including the expression one. but save the operation token before the expression token
		 * 2. push 3 new token: [expressionRegister][operation][majorExpressionRegister]
		 * 		- expressionRegister: the register assigned to the expression
		 * 		- operation: the saved operation before expression
		 * 		- majorExpressionregister: the register of that major expression
		 *			 (major section, the stride of contiuous major operations that was processing by the `compileMajorExpression_impl`
		 * 3. update stack
		 * 3. expand the expression
		 * 
		 * Note: there will not be explicit state saying that it's a resumed expression.
		 * When the `compileMajorExpression_impl` function sees a Register Token, it will just merge it with the current register and then free the token one
		 * Notice that the original major expression's result register will become a temporary value, and get merged with the new result register.
		 * (because it's the first token the resumed function sees, therefore it's value will just be directly assigned to the new result register)
		 * 
		 * Note: though the expression buffer is indeed dynamic, it will not be modified, until it's corresponding parent expression is finished compiling, when the back of it will be deleted
		 * Note: whenever consumed a register token, it must be freed immediatly after
		 * Note: the compilation of each expression should not manage their result register. instead, the parent scope of that expression will have the ownership of that register, and the responsibility of freeing it
		 */

		/**
		 * The major / minor idea
		 * Major operation (multiply and divide), which are evaluate before minor operation
		 * Minor operation (add and subtract), which are evaluate after major operation
		 */

		/**
		 * The rule:
		 * a valid expression must have an odd number of token 
		 * (it stays true in this particular compiler, since:
		 * - barckets are resolved
		 * - expression is folded
		 * - and there's currently no unary operators)
		 */


		// resumable
		// compiles the stack top expression
		void compileExpression_impl() {
			const tx::i32 end = m_stackPtr == 1 ? -1 : stackParent_impl().tokenIndex;
			const tx::i32 begin = stackTop_impl().tokenIndex;

			if (!((begin - end) % 2)) { // token count not odd: error
				return; // DevNote: error handling
			}

			tx::u8 resultReg = stackTop_impl().context.resultRegister;
			tx::u8 majorExprReg = registerNew();

			// compilation loop
			tx::i32 current = begin;
			while (current > end) {
				// find the next minor operation token
				tx::i32 minorPos = findNextMinorOperator_impl(current, end);

				bool breakFlag = compileMajorExpression_impl(current, minorPos, majorExprReg);
				if (breakFlag) { // interrupt - expand nested expression
					registerFree(majorExprReg);
					return;
				}
				pushCommand_impl( // merge result of major expression to resultReg
				    resultReg,
				    m_tokenBuffer[minorPos].getOperatorType(),
				    makeValueID_impl(resultReg, false),
				    makeValueID_impl(majorExprReg, false));
				if (minorPos > end) m_tokenBuffer.pop_back(); // delete the minor operator token

				current = minorPos - 1;
			}

			// clean up
			registerFree(majorExprReg);

			const tx::u32 exprBufferEnd =
			    m_stackPtr == 1 ? 0 : stackParent_impl().tokenIndex + 1; // inclusive for erase
			m_expressionBuffer.erase(
			    m_expressionBuffer.begin() + exprBufferEnd,
			    m_expressionBuffer.end());

			stackPop_impl();
		}
		tx::i32 findNextMinorOperator_impl(tx::i32 current, tx::i32 end) {
			while (current > end) {
				if (isMinorOperationToken_impl(m_tokenBuffer[current])) {
					return current;
				}
				current--;
			}
			return end;
		}
		static bool isMinorOperationToken_impl(Token_impl token) {
			return token.isOperator() &&
			       isMinorOperation_impl(token.getOperatorType());
		}
		static bool isMinorOperation_impl(OperationType_impl operation) {
			return operation == OperationType_impl::Add ||
			       operation == OperationType_impl::Subtract;
		}

		// basically compile a section(which is separated by minor operation)
		// all the compilation happens here
		// after this function returns, the tokens it processed should be deleted
		// @return (boolean) break flag, if true then return
		// @param begin/end [inclusive-exclusive)
		bool compileMajorExpression_impl(tx::i32 begin, tx::i32 end, tx::u8 resultReg) {
			tx::i32 current = begin;

			Token_impl currentToken = m_tokenBuffer[current];

			// first token (must be operand)
			if (currentToken.isOperator()) {
				// DevNote: error
				return;
			}
			if (currentToken.isExpression()) { // it is an expression -> expand it
				handleExpandingExpressionBegin_impl(
				    currentToken,
				    resultReg);
				return 1;
			} else { // assign it directly to resultReg
				pushCommand_impl(
				    resultReg,
				    OperationType_impl::Assign,
				    makeValueID_impl(
				        currentToken.index,
				        currentToken.isRegister(),
				        currentToken.isVaraible()),
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
					handleExpandingExpression_impl(
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
					        currentToken.isRegister(),
					        currentToken.isVaraible()));
					if (currentToken.isRegister())
						registerFree(currentToken.index);
				}
				current--;
			}

			// clean up
			m_tokenBuffer.erase(
			    m_tokenBuffer.begin() + end + 1,
			    m_tokenBuffer.end());

			return 0;
		}
		// handle expression that's at the front of the parent expression
		void handleExpandingExpressionBegin_impl(
		    Token_impl currentToken,
		    tx::u8 resultReg) {
			Expression_impl expr = m_expressionBuffer[currentToken.index];

			// just reuse the resultReg register, since it's just the first token, it was fresh before
			m_tokenBuffer.pop_back();
			m_tokenBuffer.push_back(Token_impl{
			    .type = TokenType_impl::Register,
			    .index = resultReg });

			// expanding (tokenlize) the expression
			expandExpression_impl(expr, resultReg);
		}
		void handleExpandingExpression_impl(
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

			// expanding (tokenlize) the expression
			expandExpression_impl(expr, expressionResultRegister);
		}
		// expand an expression from current context
		// handles stack push and tokenlization
		void expandExpression_impl(Expression_impl expr, tx::u8 resultReg) {
			// tokenlize
			tokenlize_impl(m_bracketBuffer[expr.bracketIndex]);

			// push stack
			stackPush_impl(
			    m_expressionBuffer.size() - 1,
			    m_tokenBuffer.size() - 1,
			    resultReg);
		}

		// compilation helpers

		static Token_impl makeOperatorToken_impl(OperationType_impl type) {
			return Token_impl{
				.type = TokenType_impl::Operator,
				.index = static_cast<tx::u8>(type)
			};
		}


		// * stack helpers

		bool stackPush_impl(tx::u32 expressionIndex, tx::u32 tokenIndex, tx::u32 resultRegister) {
			if (m_stackPtr >= stackCapacity) return false; // stack overflow
			m_stack[m_stackPtr] = StackObject_impl{
				.expressionIndex = expressionIndex,
				.tokenIndex = tokenIndex,
				.context = StackObject_impl::Context{ .resultRegister = resultRegister }
			};
			m_stackPtr++;
			return true;
		}
		void stackPop_impl() {
			m_stackPtr--;
		}

		// the current stack object
		StackObject_impl& stackTop_impl() { return m_stack[m_stackPtr - 1]; }
		StackObject_impl& stackParent_impl() { return m_stack[m_stackPtr - 2]; }

		tx::u32 stackGetExpressionCount_impl() {
			return stackTop_impl().expressionIndex - stackParent_impl().expressionIndex;
		}
		tx::u32 stackGetExpressionBegin_impl() {
			return stackTop_impl().expressionIndex;
		}


		// helpers

		// * tokenlizer parameter

		std::string_view findBracketSource_impl(Bracket_impl bracket) const {
			return m_source.substr(
			    bracket.range.offset + 1,
			    bracket.range.size - 2);
		}
		tx::u32 findBracketSourceOffset_impl(Bracket_impl bracket) const {
			return bracket.range.offset + 1;
		}
		std::span<const Bracket_impl> findBracketChilds_impl(Bracket_impl bracket) const {
			return std::span<const Bracket_impl>{
				m_bracketBuffer.begin() + (bracket.index + 1),
				m_bracketBuffer.begin() + (bracket.index + 1 + bracket.childCount)
			};
		}

		void tokenlize_impl(
		    std::string_view source,
		    tx::u32 sourceOffset,
		    std::span<const Bracket_impl> bracketBuffer) {

			tx::u32 oldExpressionBufferSize = m_expressionBuffer.size(),
			        oldTokenBufferSize = m_tokenBuffer.size();

			Tokenlizer_impl tokenlizer(
			    source, sourceOffset, bracketBuffer,
			    m_bin.constantBuffer, m_constantBufferSize,
			    m_variableBuffer,
			    m_expressionBuffer,
			    m_tokenBuffer);
			m_constantBufferSize = tokenlizer.run().constantOffset;

			if (oldExpressionBufferSize < m_expressionBuffer.size())
				std::reverse(
				    m_expressionBuffer.begin() + oldExpressionBufferSize,
				    m_expressionBuffer.end());
			if (oldTokenBufferSize < m_tokenBuffer.size())
				std::reverse(
				    m_tokenBuffer.begin() + oldTokenBufferSize,
				    m_tokenBuffer.end());
		}
		// somewhat high level helper function
		void tokenlize_impl(Bracket_impl bracket) {
			tokenlize_impl(
			    findBracketSource_impl(bracket),
			    findBracketSourceOffset_impl(bracket),
			    findBracketChilds_impl(bracket));
		}

		// code gen

		tx::u32 m_commandBufferSize = 0;
		void pushCommand_impl(
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
		tx::u8 makeValueID_impl(tx::u8 index, bool isFromBuffer, bool isVariableBuffer = false) {
			return tx::bit::setted(
			    tx::bit::setted(index, tx::u8{ 0x80 }, isFromBuffer),
			    tx::u8{ 0x40 }, isVariableBuffer);
		}
	};


	static void compile_impl(std::string_view source, Expression& bin) {
		Compiler_impl compiler(source, bin);
		compiler.run();
	}

	static num evaluate_impl(Expression& bin) {
		return num{};
	}

	// helper functions
};

/**
 * Things to add:
 * 1. Variable write back - the assign operator
 * 2. constant merge / constant pre-compute
 */