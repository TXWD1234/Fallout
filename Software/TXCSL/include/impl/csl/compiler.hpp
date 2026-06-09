// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/value_group.hpp"
#include "tx/handle_system.hpp"
#include "tx/bit_trick.hpp"
#include <span>
#include <string_view>
#include <charconv>



namespace tx::csl {
/**
 * This compiler don't manage memory. Only temporary memory will be created by it.
 * The long term memory to store the output requires you to provide.
 * If the memory buffer you provided is not enough, evaluation will be interrupted (DevNote: add error handling) 
 * 
 * Specification:
 * Optimal size of CommandBuffer: 256
 * Max size of ConstantBuffer: 64
 * Max size of VariableBuffer: 64
 */
class Compiler {
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
	 * 2. tokenlize (only the top layer) (Lexer)
	 * 3. parse (handle unary operators, functions)
	 * 4. codegen
	 * 		- when encountered a token of type `Expression`, goto setp 2
	 * 
	 * The process of nested expressions will be lazy,
	 * meaning that it will not be processed until necessary
	 */

	/**
	 * Pipeline
	 * 
	 * 1. Raw string
	 * 2. BracketParser - compose bracket structure
	 * 3. Tokenlizer    - tokenlize raw string. identify and convert value
	 * 4. Parser        - parse the tokens to handle token combinations such as unary operators and functions
	 * 5. CodeGen       - transform tokens into instructions, flatten the operation tree
	 */

public:
	// ======================================
	// **************** APIs ****************
	// ======================================

	static CompileResult compile(std::string_view source, Expression& result);

private:
	// ================================================
	// **************** Implementation ****************
	// ================================================

	// -----------------------------------------------
	// ++++++++++++++++ BracketParser ++++++++++++++++
	// -----------------------------------------------

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
	class BracketParser_impl;


	// --------------------------------------------
	// ++++++++++++++++ Tokenlizer ++++++++++++++++
	// --------------------------------------------

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
		bool isVariable() const { return type == TokenType_impl::Variable; }
		bool isExpression() const { return type == TokenType_impl::Expression; }
		bool isRegister() const { return type == TokenType_impl::Register; }
		bool isBuffer() const { return isConstant() || isVariable(); }
	};
	using Constant_impl = num;
	using Variable_impl = std::string_view;
	struct Expression_impl {
		tx::u32 bracketIndex; // global index
	};

	class Tokenlizer_impl;

	// ----------------------------------------
	// ++++++++++++++++ Parser ++++++++++++++++
	// ----------------------------------------

	class Parser_impl {
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
		 * This class will process the token buffer inplace, after everytime the tokenlizer is called
		 * It will handle token combinations such as unary operator and functions
		 */

		/**
		 * Main Logic Design:
		 * Following the STL 2 pointer approach: dest and cur
		 */
	public:
		// need vector here for possible expansion
		Parser_impl(std::vector<Token_impl>& tokenBuffer, tx::u32 begin)
		    : m_tokenBuffer(tokenBuffer), m_begin(begin) {
		}



	private:
		std::vector<Token_impl>& m_tokenBuffer;
		tx::u32 m_begin;
	};

	// -----------------------------------------
	// ++++++++++++++++ CodeGen ++++++++++++++++
	// -----------------------------------------

	class CodeGen_impl;
};
} // namespace tx::csl