// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "tx/csl.h"
#include "tx/terminal.h"
#include "impl/font.h"

#include "tx/type_traits.hpp"
#include <string>


namespace tx::compute {

terminal::Font FontObject{
	.width = u32{ 10 },
	.height = u32{ 20 },
	.bitmapData = std::span<u8>(FontData, FontDataSize)
};

static constexpr u32 stringHash_impl(std::string_view str) {
	u32 hash = 2166136261u;
	for (char c : str) hash = (hash ^ c) * 16777619u;
	return hash;
}

class MainClass {
public:
	MainClass()
	    : m_terminal(FontObject, Coord{ 48, 16 }) {
		startup_impl();
	}


	// must be called immediately at startup
	static void init() {
		esp::PowerManager::init(PowerPin);
		esp::PowerManager::power_on();
	}
	// must be called no earlier then the very end of the main function
	static void terminate() {
		esp::PowerManager::power_off();
	}


	void run() {
		while (!m_state.shouldTerminate) {
			m_terminal.poll();
			if (m_state.inProcess_userInput) {
				handleInput_impl();
				m_terminal.startInputSession();
			}
		}
	}

private:
	// =============================================
	// **************** Static Data ****************
	// =============================================

	inline static constexpr const char* UserInputHeaderStr = "TXCompute> ";

	struct Log {

		inline static constexpr const char* EvaluateFailure[] = {
			"Evaluation failed: Register Overflow."
		};
	};

	inline static constexpr const gpio_num_t PowerPin = GPIO_NUM_13;

private:
	terminal::TerminalEngine m_terminal;
	struct Buffers_impl {
		esp::Buffer<csl::Command> commandBuffer{ 256 };
		esp::Buffer<csl::num> constantBuffer{ 64 };
		esp::Buffer<csl::num> variableBuffer{ 64 };
		esp::Buffer<csl::num> registerFile{ 64 };
	} m_buffers;

	struct State_impl {
		bool shouldTerminate = false;

		bool inProcess_userInput = false;
		std::string_view userInputStr;
	} m_state;

	void startup_impl() {
		init_impl();
	}

	void init_impl() {
		tx::csl::Evaluator::setRegisterFileMemory(m_buffers.registerFile.span());

		m_terminal.setInputCallback([&](std::string_view str) {
			terminalInputCallback_impl(str);
		});
		m_terminal.setUserInputHeader(UserInputHeaderStr);
		m_terminal.startInputSession();
	}
	// there is not an uninit function because there is nothing no uninit


	// ============================================
	// **************** Core Logic ****************
	// ============================================

	void terminalInputCallback_impl(std::string_view str) {
		m_state.inProcess_userInput = true;
		m_state.userInputStr = str;
	}

	void handleInput_impl() {
		std::string_view cmdStr = m_state.userInputStr.substr(
		    0, m_state.userInputStr.find(' '));

		switch (stringHash_impl(cmdStr)) {
		case stringHash_impl("exit"):
			handleCommand_exit();
			break;
		default:
			handleCSLExpression();
		};
	}

	void handleCSLExpression() {
		csl::Expression expr{
			m_buffers.commandBuffer.span(),
			m_buffers.constantBuffer.span(),
			m_buffers.variableBuffer.span()
		};

		csl::Compiler::compile(
		    m_state.userInputStr, expr);


		auto result = csl::Evaluator::evaluate(expr);
		if (result) {
			m_terminal.print(std::to_string(result.value));
			return;
		}

		m_terminal.printStatic(
		    Log::EvaluateFailure[enumval(result.err)]);
	}

	void handleCommand_exit() {
		m_state.shouldTerminate = true;
	}
};
} // namespace tx::compute

/**
 * Things to do:
 * 1. CSL Compile Error handling
 */