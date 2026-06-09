// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include <vector>
#include <span>
#include <string_view>

namespace tx::csl {

using num = float;

enum class OperationType_impl : tx::u8 {
	Invalid = 0,
	Add = 1,
	Subtract = 2,
	Multiply = 3,
	Divide = 4,
	Assign = 5,
};

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
	tx::u32 commandCount;
};

struct CompileResult {
	tx::u32 commandCount;
	tx::u32 constantCount;
	tx::u32 variableCount;
	std::vector<std::string_view> variableNames; // for the caller of compile() to fill in the variabe buffer
};
} // namespace tx::csl