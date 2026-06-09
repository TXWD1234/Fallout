// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "tx/bit_trick.hpp"

namespace tx::csl {
class Evaluator {
public:
	// APIs

	enum class ErrorCode : u32 {
		Success = 0xFFFFFFFF,
		RegisterOverflow = 0,
	};

	struct EvaluateResult {
		num value;
		ErrorCode err;

		operator bool() { return err == ErrorCode::Success; }
	};

	static EvaluateResult evaluate(Expression expression) {
		num result;
		ErrorCode err = Evaluator_impl(expression).run(&result);
		return EvaluateResult{ result, err };
	}
	static void setRegisterFileMemory(void* ptr, size_t size) {
		registerFile = static_cast<num*>(ptr);
		registerFileSize = size;
	}
	static void setRegisterFileMemory(std::span<num> span) {
		registerFile = static_cast<num*>(span.data());
		registerFileSize = span.size();
	}

private:
	// Implementation

	// ======== Members ========

	inline static num* registerFile = nullptr;
	inline static size_t registerFileSize = 0;

	// ======== Operations ========

	// operation table
	static void add_impl(num& dest, num a, num b) { dest = a + b; }
	static void sub_impl(num& dest, num a, num b) { dest = a - b; }
	static void mul_impl(num& dest, num a, num b) { dest = a * b; }
	static void div_impl(num& dest, num a, num b) { dest = a / b; }

	static void assign_impl(num& dest, num val) { dest = val; }

	// clang-format off

	static void performOperation_impl(OperationType_impl operation, num& dest, num a, num b) {
		switch (operation) {
		case OperationType_impl::Add:      add_impl(dest, a, b); break;
		case OperationType_impl::Subtract: sub_impl(dest, a, b); break;
		case OperationType_impl::Multiply: mul_impl(dest, a, b); break;
		case OperationType_impl::Divide:   div_impl(dest, a, b); break;
		case OperationType_impl::Assign:   assign_impl(dest, a); break;
		case OperationType_impl::Invalid: break;
		}
	}
	// clang-format on


	// ======== Evaluator ========

	class Evaluator_impl {
	public:
		Evaluator_impl(Expression expr)
		    : m_bin(expr) {}

		// @return success
		ErrorCode run(num* val) {
			if (registerFileSize < m_bin.registerCount) return ErrorCode::RegisterOverflow;

			evaluate_impl();

			(*val) = registerFile[0];
			return ErrorCode::Success;
		}

	private:
		Expression m_bin;

		void evaluate_impl() {
			for (tx::u32 i = 0; i < m_bin.commandCount; i++) {
				executeCommand_impl(m_bin.commandBuffer[i]);
			}
		}
		void executeCommand_impl(Command cmd) {
			performOperation_impl(
			    cmd.m_operation,
			    registerFile[cmd.m_dest],
			    valueIdGetValue(cmd.m_vala),
			    valueIdGetValue(cmd.m_valb));
		}

		// helper for decoding the command

		tx::u8 valueIdGetIndex(tx::u8 valueId) {
			return tx::bit::erase_copy(valueId, tx::u8{ 0xC0 }); // 0x80 | 0x40
		}
		bool valueIdIsFromBuffer(tx::u8 valueId) {
			return tx::bit::contains_all(valueId, tx::u8{ 0x80 });
		}
		bool valueIdIsVariableBuffer(tx::u8 valueId) {
			return tx::bit::contains_all(valueId, tx::u8{ 0x40 });
		}
		num valueIdGetValue(tx::u8 valueId) {
			tx::u8 index = valueIdGetIndex(valueId);
			if (valueIdIsFromBuffer(valueId)) {
				return valueIdIsVariableBuffer(valueId) ?
				           m_bin.variableBuffer[index] :
				           m_bin.constantBuffer[index];
			} else {
				return registerFile[index];
			}
		}
	};
};
}