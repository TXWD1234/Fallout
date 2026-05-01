// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"
#include "tx/data.h"
#include <span>
#include <string>
#include <string_view>
#include <charconv>

/**
 * This evaluator don't manage memory, it requires you to provide memory for it.
 * If the memory buffer you provided is not enough, evaluation will be interrupted (DevNote: add error handling) 
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
	enum class OperationType : tx::u8;

public:
	using num = float;

	// @note size == 4 byte
	struct Command {
		tx::u8 m_dest; // index of the destination register
		OperationType m_operation; // index of the operation
		tx::i8 m_vala; // index variable / constant value to be processed (negative is variable, positive is constant)
		tx::i8 m_valb; // index variable / constant value to be processed (negative is variable, positive is constant)
	};
	struct Expression {
		std::span<Command> commandQueue;
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
	static num evaluate(std::string_view source) {

		Expression bin;
		compile(source, bin);
		return evaluate_impl(bin);
	}

private:
	// operation table
	static void add_impl(num& dest, num a, num b) { dest = a + b; }
	static void sub_impl(num& dest, num a, num b) { dest = a - b; }
	static void mul_impl(num& dest, num a, num b) { dest = a * b; }
	static void div_impl(num& dest, num a, num b) { dest = a / b; }

	enum class OperationType_impl : tx::u8 {
		Add = 1,
		Subtract = 2,
		Multiply = 3,
		Divide = 4,
	};
	// clang-format off

	static void performOperation_impl(OperationType_impl operation, num& dest, num a, num b) {
		switch (operation) {
		case OperationType_impl::Add:      add_impl(dest, a, b); break;
		case OperationType_impl::Subtract: sub_impl(dest, a, b); break;
		case OperationType_impl::Multiply: mul_impl(dest, a, b); break;
		case OperationType_impl::Divide:   div_impl(dest, a, b); break;
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
	 */

	struct Bracket_impl {
		struct Range_impl {
			tx::u32 offset, size;
		} range; // include the "()"
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
		    : m_source(source), m_buffer(&buffer) {}

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
		std::vector<Bracket_impl>* m_buffer;
		tx::u8 m_stackPtr;

		// logic
		/**
		 * Convension for logic clearity and correctness:
		 * stack operations (stack push and stack pop) must happen in the function strictly according to their role
		 * - stack push must at the begining of the function (after integrity check of course)
		 * - stack pop must at the end of the function
		 */

		bool stateEnterBracket_impl(tx::u32 index) {
			if (!stackCheckOverflow_impl()) return false; // stack overflow
			stackPush_impl(m_buffer->size());
			if (m_stackPtr != 0) currentParent_impl().childCount++;
			m_buffer->push_back(Bracket_impl{
			    Bracket_impl::Range_impl{ index, tx::u32{ 0 } },
			    tx::u16{ 0 }, static_cast<tx::u16>(m_buffer->size()) });
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

		Bracket_impl& currentParent_impl() { return m_buffer->operator[](stackTop() - 1); }
		Bracket_impl& current_impl() { return m_buffer->operator[](stackTop()); }
	};


	enum class TokenType_impl : tx::u16 {
		Constant,
		Variable,
		Operation,
		Expression
	};
	struct Token_impl {
		tx::u16 index; // index at the corrisponding token object array of the type
		TokenType_impl type;
		// if type is Operation, then index is just the enum value
		OperationType_impl getOperationType() const { return static_cast<OperationType_impl>(index); }
		void setOperationType(OperationType_impl type) { index = static_cast<tx::u16>(type); }
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
		      m_variableBuffer(&variableBuffer),
		      m_expressionBuffer(&expressionBuffer),
		      m_tokenBuffer(&tokenBuffer),
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
		std::vector<Variable_impl>* m_variableBuffer;
		std::vector<Expression_impl>* m_expressionBuffer;
		std::vector<Token_impl>* m_tokenBuffer;
		tx::u32 m_sourceOffset;

		// state
		tx::u32 m_index = 0;
		tx::u32 m_nextBracketIndex = 0;

		void parse_impl() {
			while (m_index < m_source.size()) {
				skipWhiteSpace_impl(); // early return if no white space existing
				if (m_index < m_source.size()) break;
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
			}
		}



		void parseExpression_impl() {
			m_tokenBuffer->push_back(Token_impl{
			    static_cast<tx::u16>(m_expressionBuffer->size()),
			    TokenType_impl::Expression });
			m_expressionBuffer->push_back(Expression_impl{
			    nextBracket_impl().index });

			m_index += nextBracket_impl().range.size;
			m_nextBracketIndex++;
		}
		void parseOperation_impl() { // DevNote: add multi character operator
			m_tokenBuffer->push_back(Token_impl{
			    static_cast<tx::u16>(getOperationType_impl(m_source[m_index])),
			    TokenType_impl::Operation });
			m_index++;
		}
		void parseVariable_impl() {
			tx::u32 begin = m_index;
			while (isVariableNameBody_impl(m_source[m_index])) m_index++; // resulted one index after the variable

			m_tokenBuffer->push_back(Token_impl{
			    static_cast<tx::u16>(m_variableBuffer->size()),
			    TokenType_impl::Variable });
			m_variableBuffer->push_back(m_source.substr(
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
			}

			m_index += static_cast<tx::u32>(ptr - begin);

			m_tokenBuffer->push_back(Token_impl{
			    static_cast<tx::u16>(m_constantBufferSize),
			    TokenType_impl::Constant });
			bufferConstantPushBack_impl(val);
		}

		// helpers

		Bracket_impl nextBracket_impl() const { return m_bracketBuffer[m_nextBracketIndex]; }
		tx::u32 nextBracketBegin_impl() const { return nextBracket_impl().range.offset - m_sourceOffset; }
		bool isBracket_impl() const { return nextBracketBegin_impl() == m_index; }
		bool hasIncomingBrackets() const { return m_nextBracketIndex < m_bracketBuffer.size(); }

		static bool isWhiteSpace_impl(char c) { return tx::CharWhiteSpaceGroup::contains(c); }
		void skipWhiteSpace_impl() {
			while (isWhiteSpace_impl(m_source[m_index])) { m_index++; }
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
		 * 
		 * TokenBuffer
		 * Store the tokens generated by Tokenlizer.
		 * Acts like stack, and will be constantly appending and deleting.
		 * Accessability: composed by Tokenlizer, read and transfer in Compiler
		 * Lifetime: vector created during compilation, will be destoried after compilation
		 * Type: compilation data
		 * 
		 * 
		 * Note:
		 * ConstantBuffer and VariableBuffer will accumulate during compilation. 
		 * They will result a structure that have the same sequence as the BracketBuffer.
		 * 
		 * ExpressionBuffer and TokenBuffer will be constantly generating and deleting.
		 * They acts like stack, where the base data will stay, and their child object (expression / bracket) will expand after them.
		 * The processed data will be deleted.
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
		std::vector<Bracket_impl> m_bracketBuffer;
		std::vector<Variable_impl> m_variableBuffer;
		std::vector<Expression_impl> m_expressionBuffer; // this acts like the stack
		std::vector<Token_impl> m_tokenBuffer; // this acts like stack

		tx::u32 m_constantBufferSize = 0;

		void compile_impl() {
			stageBracket_impl();
			Tokenlizer_impl tokenlizer(
			    m_source,
			    0,
			    m_bracketBuffer,
			    m_bin.constantBuffer, m_constantBufferSize,
			    m_variableBuffer,
			    m_expressionBuffer,
			    m_tokenBuffer);
			tokenlizer.run();

			int a = 0;
		}

		// stages

		void stageBracket_impl() {
			BracketParser_impl bracketParser(m_source, m_bracketBuffer);
			bracketParser.run(); // DevNote: if stack overflow
		}
		void stageTokenlizeExpr_impl(Bracket_impl bracket) {
			Tokenlizer_impl tokenlizer(
			    findBracketSource_impl(bracket),
			    findBracketSourceOffset_impl(bracket),
			    findBracketChilds_impl(bracket),
			    m_bin.constantBuffer, m_constantBufferSize,
			    m_variableBuffer,
			    m_expressionBuffer,
			    m_tokenBuffer);
			tokenlizer.run();
		}


		// helpers

		// tokenlizer parameter

		std::string_view findBracketSource_impl(Bracket_impl bracket) {
			return m_source.substr(
			    bracket.range.offset + 1,
			    bracket.range.size - 2);
		}
		tx::u32 findBracketSourceOffset_impl(Bracket_impl bracket) {
			return bracket.range.offset + 1;
		}
		std::span<const Bracket_impl> findBracketChilds_impl(Bracket_impl bracket) {
			return std::span<const Bracket_impl>{
				m_bracketBuffer.begin() + (bracket.index + 1),
				m_bracketBuffer.begin() + (bracket.index + 1 + bracket.childCount)
			};
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