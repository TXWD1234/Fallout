// Copyright@TXCompute All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXCSL_Engine
// File: expression_evaluator.hpp

#pragma once
#include "impl/basic_utils.hpp"
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
	 * - compose(): make operation queue / command buffer
	 * - evaluate(): calculate / execute the composed command buffer
	 * 
	 * ## Command buffer
	 * The register
	 * Opaque struct `Operation`: set a register as the result of an `Expression`
	 */
public:
	static void compose(std::string_view source, std::span<std::byte> buffer) { compose_impl(source, buffer); }
	static float evaluate(std::string_view source, std::span<std::byte> buffer) {
		compose(source, buffer);
		return evaluate_impl(buffer);
	}

private:
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


	static void compose_impl(std::string_view source, std::span<std::byte> buffer) {
	}

	static float evaluate_impl(std::span<const std::byte> buffer) {
	}

	// helper functions
};