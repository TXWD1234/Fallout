// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/basic_utils.hpp"
#include <vector>
#include <concepts>

namespace tx {
// be aware of the dangling reference of the deleted handles
template <std::integral T>
class HandleSystemBase {
public:
	using value_type = T;

	T addHandle() {
		if (!m_availableHandleBuffer.empty()) {
			T handle = pop_back();
			m_active[handle] = 1;
			return handle;
		}
		m_active.push_back(1);
		return m_handleMax++;
	}
	void deleteHandle(T handle) {
		if (handle < m_handleMax && m_active[handle]) {
			m_active[handle] = 0;
			m_availableHandleBuffer.push_back(handle);
		}
	}

	bool valid(T handle) const { return handle < m_handleMax && m_active[handle]; }

	tx::u32 count() { return m_handleMax - static_cast<tx::u32>(m_availableHandleBuffer.size()); }
	tx::u32 countMax() { return m_handleMax; }

	void reserve(tx::u32 count) { m_active.reserve(count); }
	void reserveDeletion(tx::u32 count) { m_availableHandleBuffer.reserve(count); }

private:
	std::vector<T> m_availableHandleBuffer;
	std::vector<tx::u8> m_active;
	T m_handleMax = 0; // aka the next handle

	T pop_back() {
		T back = m_availableHandleBuffer.back();
		m_availableHandleBuffer.pop_back();
		return back;
	}
};
} // namespace tx