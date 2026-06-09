// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/csl/compiler.hpp"

namespace tx::csl {
class Compiler::BracketParser_impl {
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
} // namespace tx::csl