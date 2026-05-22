// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "tx/math.h"
#include <algorithm>

/**
 * @note
 * this class will allocate memory:
 * - double text buffer (left and right buffer) of the max buffer size provided
 * 
 * This class is designed under the KISS rule:
 * Keep it Simple, Stupid
 */
class InputLine {
public:
	InputLine(tx::u32 maxBufferSize)
	    : m_data(maxBufferSize) {}

	// user operations

	void input(char val) {
		m_data.left.push(val);
	}
	// backspace
	void deleteFront() {
		m_data.left.pop();
	}
	// delete
	void deleteBack() {
		m_data.right.pop();
	}
	void cursorMoveLeft(tx::u32 distance = 1) {
		for (tx::u32 i = 0; i < distance && m_data.left.size(); i++) {
			m_data.right.push(m_data.left.top());
			m_data.left.pop();
		}
	}
	void cursorMoveRight(tx::u32 distance = 1) {
		for (tx::u32 i = 0; i < distance && m_data.right.size(); i++) {
			m_data.left.push(m_data.right.top());
			m_data.right.pop();
		}
	}

	// program operations

	// @return write successful
	bool output(std::span<char> buffer) {
		// size check
		if (buffer.size() < m_data.left.size() + m_data.right.size()) return false;
		std::reverse_copy(m_data.right.begin(), m_data.right.end(),
		                  std::copy(m_data.left.begin(), m_data.left.end(), buffer.begin()));
		return true;
	}

	void clear() {
		m_data.left.m_size = 0;
		m_data.right.m_size = 0;
	}


private:
	struct Buffer_impl {
		Buffer_impl(tx::u32 in_size)
		    : m_data(std::span<char>(
		          static_cast<char*>(heap_caps_malloc(
		              in_size, MALLOC_CAP_8BIT)),
		          (size_t)in_size)),
		      m_size(0), m_capacity(in_size) {}
		~Buffer_impl() {
			heap_caps_free(m_data.data());
		}

		using It_t = std::span<char>::iterator;
		using ConstIt_t = std::span<char>::const_iterator;

		std::span<char> m_data;
		tx::u32 m_size,
		    m_capacity;

		void push(char val) {
			m_data[m_size] = val;
			m_size = std::min(m_size + 1, m_capacity - 1);
		}
		void pop() {
			m_size = std::max(m_size, tx::u32{ 1 }) - 1;
		}
		char top() const {
			return m_data[m_size - 1];
		}
		tx::u32 size() const {
			return m_size;
		}

		It_t begin() { return m_data.begin(); }
		ConstIt_t begin() const { return m_data.begin(); }
		It_t end() { return m_data.begin() + m_size; }
		ConstIt_t end() const { return m_data.begin() + m_size; }
	};

	struct Data_impl {
		Data_impl(tx::u32 size)
		    : left(size),
		      right(size) {}

		Buffer_impl left;
		Buffer_impl right;
	} m_data;
};














struct Font {
	tx::u32 width;
	tx::u32 height;
	std::span<tx::u8> bitmapData;
};
inline TextRenderer makeTextRenderer(Font font) {
	return TextRenderer{
		font.width,
		font.height,
		font.bitmapData
	};
}



/**
 * @note
 * this class will allocate memory:
 * - TextRenderer: double render buffer, each `width * height * 2` (u16)
 * - String buffer: 
 */
class TerminalEngine {
	/**
	 * This Terminal Engine is highly optimized for memory usage, but not performance.
	 * Both the update and render logic is designed with tight memory constraint in mind
	 */
public:
	// type def

	using InputCallback_t = std::function<void(std::string_view)>;


public:
	TerminalEngine(Font font) : m_rr(makeTextRenderer(font)) {}

	// init data setter

	void setInputCallback(InputCallback_t cb) {
		m_inputCb = cb;
		m_inputCbInited = true;
	}
	template <std::invocable<std::string_view> Func>
	void setInputCallback(Func&& f) {
		m_inputCb = std::forward<Func>(f);
		m_inputCbInited = true;
	}

private:
	// call back and source data (init data)

	InputCallback_t m_inputCb;
	bool m_inputCbInited = false;

	std::span<std::string_view> m_stringPresets;

private:
	// runtime data

	TextRenderer m_rr;
};