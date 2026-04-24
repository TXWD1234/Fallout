// Copyright@TXCompute All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXCSL_Engine
// File: expression_evaluator.hpp

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"
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
		tx::u32 registerCount;
	};

public:
	static void compile(std::string_view source, Expression& result) { compile_impl(source, result); }
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

	enum class OperationType : tx::u8 {
		Add = 1,
		Subtract = 2,
		Multiply = 3,
		Divide = 4,
	};
	// clang-format off

	static void performOperation_impl(OperationType operation, num& dest, num a, num b) {
		switch (operation) {
		case OperationType::Add:      add_impl(dest, a, b); break;
		case OperationType::Subtract: sub_impl(dest, a, b); break;
		case OperationType::Multiply: mul_impl(dest, a, b); break;
		case OperationType::Divide:   div_impl(dest, a, b); break;
		}
	}
	// clang-format on


private:
	// structure construction
	struct Range_impl {
		tx::u32 offset, size;
	};
	struct Bracket_impl {
		Range_impl range;
		tx::u16 childCount;
	};

	static void composeBracket_impl(std::string_view source, std::span<std::byte> buffer) {
		std::vector<tx::u32> stack;
		stack.reserve(8);



		for (tx::u32 i = 0; i < source.size(); i++) {
			if (source[i] == '(') {
			}
		}
	}


	static void compile_impl(std::string_view source, Expression& bin) {
	}

	static num evaluate_impl(Expression& bin) {
	}

	// helper functions
};