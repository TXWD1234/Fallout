// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "esp_heap_caps.h"
#include "impl/grow_array.hpp"
#include "impl/circular_queue.hpp"
#include "impl/ring_buffer.hpp"
#include <span>

namespace tx::esp {

template <class T>
inline T* allocate(u32 size) {
	if (size == 0) return nullptr;
	return static_cast<T*>(heap_caps_malloc(
	    size * sizeof(T), MALLOC_CAP_8BIT));
}
template <class T>
inline void free(T* ptr) {
	if (!ptr) return;
	heap_caps_free(ptr);
}


template <class T>
struct Buffer {
public:
	Buffer(u32 capacity)
	    : m_data(allocate<T>(capacity)),
	      m_size(capacity) {}
	~Buffer() {
		free(m_data);
	}

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	u32 size() const { return m_size; }

	std::span<T> span() { return std::span<T>(m_data, m_size); }
	std::span<const T> span() const { return std::span<const T>(m_data, m_size); }

private:
	T* m_data;
	u32 m_size;
};

template <class T>
class Buffer_GrowArray : public GrowArrayOverlay<T> {
public:
	Buffer_GrowArray(u32 capacity)
	    : GrowArrayOverlay<T>(
	          allocate<T>(capacity), capacity) {}
	~Buffer_GrowArray() {
		if (!this->isNull_impl()) {
			this->destruct_impl(); // destroy live elements before freeing
			free(this->data());
		}
	}

	Buffer_GrowArray(const Buffer_GrowArray<T>& other)
	    : GrowArrayOverlay<T>(
	          allocate<T>(other.m_capacity), other.m_capacity) {
		copy_impl(other);
	}
	Buffer_GrowArray(Buffer_GrowArray<T>&& other) : GrowArrayOverlay<T>(other) {
		// just use the copy constructor of GrowArrayOverlay - shallow copy
		other.null_impl();
	}
	Buffer_GrowArray& operator=(Buffer_GrowArray<T> other) {
		this->swap_impl(other);
		return *this;
	}

private:
	// cannot be swap_impl because ambiguity with base's swap_impl
	// this function exists for potential future expansion
	void swap(Buffer_GrowArray<T>& other) {
		swap_impl(other);
	}
};

// exist just because i don't want to change the original CircularQueue in
// TXLib, but TXCompute require some feature that it don't support.
// I cannot just add it because that don't fit the architecture of this data
// structure
template <class T>
class TempCircularQueueOverlay {
public:
	using value_type = T;

public:
	/**
	 * @param ptr the data pointer to a piece of memory that have at least size of `capacity`.
	 * @param capacity the capacity of this container object. It cannot resize.
	 */
	TempCircularQueueOverlay(T* ptr, u32 size)
	    : m_data(ptr), m_size(size) {}
	/**
	 * @param buffer the provided storage memory buffer
	 */
	TempCircularQueueOverlay(std::span<T> buffer)
	    : m_data(buffer.data()), m_size(buffer.size()) {}
	~TempCircularQueueOverlay() {}

	TempCircularQueueOverlay(const TempCircularQueueOverlay&) = default;
	TempCircularQueueOverlay& operator=(const TempCircularQueueOverlay&) = default;
	TempCircularQueueOverlay(TempCircularQueueOverlay&& other) = default;
	TempCircularQueueOverlay& operator=(TempCircularQueueOverlay&& other) = default;

	// basic operations

	void push(const T& val) {
		assert_impl([&]() { return !full(); },
		            "tx::TempCircularQueueOverlay::push(): called on full buffer");
		std::construct_at(m_data + m_end, val);
		push_impl();
	}
	void push(T&& val) {
		assert_impl([&]() { return !full(); },
		            "tx::TempCircularQueueOverlay::push(): called on full buffer");
		std::construct_at(m_data + m_end, std::move(val));
		push_impl();
	}
	template <class... Args>
	void emplace(Args&&... args) {
		assert_impl([&]() { return !full(); },
		            "tx::TempCircularQueueOverlay::emplace(): called on full buffer");
		std::construct_at(m_data + m_end, std::forward<Args>(args)...);
		push_impl();
	}

	void pop() {
		std::destroy_at(m_data + m_begin);
		pop_impl();
	}

	void clear() {
		if (m_wrap) {
			std::destroy(
			    m_data + m_begin,
			    m_data + m_size);
			std::destroy(
			    m_data,
			    m_data + m_end);
		} else {
			std::destroy(
			    m_data + m_begin,
			    m_data + m_end);
		}

		m_begin = 0;
		m_end = 0;
		m_wrap = false;
	}

	// basic getter

	bool full() const { return m_begin == m_end && m_wrap; }
	bool empty() const { return m_begin == m_end && !m_wrap; }
	u32 size() const {
		return (m_wrap ?
		            m_size - m_begin + m_end :
		            m_end - m_begin);
	}
	u32 capacity() const { return m_size; }

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	// data getters

	T& front() {
		assert_impl([&]() { return !empty(); },
		            "tx::TempCircularQueueOverlay::front(): called on empty buffer");
		return *(m_data + m_begin);
	}
	const T& front() const {
		assert_impl([&]() { return !empty(); },
		            "tx::TempCircularQueueOverlay::front(): called on empty buffer");
		return *(m_data + m_begin);
	}
	T& back() {
		assert_impl([&]() { return !empty(); },
		            "tx::TempCircularQueueOverlay::back(): called on empty buffer");
		return *(m_data + m_end - 1);
	}
	const T& back() const {
		assert_impl([&]() { return !empty(); },
		            "tx::TempCircularQueueOverlay::back(): called on empty buffer");
		return *(m_data + m_end - 1);
	}

	// injected API for specialized TXCompute usage

	// T& atReversed(u32 index) {
	// 	if (index >= m_end) {
	// 		return *(m_data + m_size - (index - m_end) - 1);
	// 	} else {
	// 		return *(m_data + m_end - index - 1);
	// 	}
	// }
	// const T& atReversed(u32 index) const {
	// 	if (index >= m_end) {
	// 		return *(m_data + m_size - index - 1);
	// 	} else {
	// 		return *(m_data + m_end - index - 1);
	// 	}
	// }


private:
	T* m_data;
	u32 m_size;
	u32 m_begin = 0, m_end = 0;
	bool m_wrap = false;

	T& at_impl(u32 index) {
		return *(m_data + index);
	}

	void push_impl() {
		m_end++;
		if (m_end == m_size) {
			m_end = 0; // wrapping logic
			m_wrap = true;
		}
	}
	void pop_impl() {
		m_begin++;
		if (m_begin == m_size) {
			m_begin = 0; // wrapping logic
			m_wrap = false;
		}
	}

	template <std::invocable<T&> Func>
	void foreach_impl(Func&& f) {
		if (m_wrap) {
			for (u32 i = m_begin; i < m_size; i++) {
				f(at_impl(i));
			}
			for (u32 i = 0; i < m_end; i++) {
				f(at_impl(i));
			}
		} else {
			for (u32 i = m_begin; i < m_end; i++) {
				f(at_impl(i));
			}
		}
	}

protected:
	void null_impl() {
		m_data = nullptr;
		m_size = 0;
		m_begin = 0;
		m_end = 0;
		m_wrap = false;
	}
	void swap_impl(TempCircularQueueOverlay<T>& other) {
		std::swap(m_data, other.m_data);
		std::swap(m_size, other.m_size);
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
		std::swap(m_wrap, other.m_wrap);
	}
	// copy the data and state of another object after construction
	// to fully sync with the other object
	void copy_impl(const TempCircularQueueOverlay<T>& other) {
		if (other.m_wrap) {
			std::uninitialized_copy(
			    other.m_data + other.m_begin,
			    other.m_data + other.m_size,
			    m_data + other.m_begin);
			std::uninitialized_copy(
			    other.m_data,
			    other.m_data + other.m_end,
			    m_data);
		} else {
			std::uninitialized_copy(
			    other.m_data + other.m_begin,
			    other.m_data + other.m_end,
			    m_data);
		}
		m_begin = other.m_begin;
		m_end = other.m_end;
		m_wrap = other.m_wrap;
	}
	// query if object is valid
	// used for defend moved-from object
	bool isNull_impl() const {
		return !m_data;
	}
	// called at destruction to clean up data
	void destruct_impl() {
		clear();
	}
};

template <class T>
class Buffer_CircularQueue : public TempCircularQueueOverlay<T> {
public:
	Buffer_CircularQueue(u32 capacity)
	    : TempCircularQueueOverlay<T>(
	          allocate<T>(capacity), capacity) {}
	~Buffer_CircularQueue() {
		if (!this->isNull_impl()) {
			this->destruct_impl(); // destroy live elements before freeing
			free(this->data());
		}
	}

	Buffer_CircularQueue(const Buffer_CircularQueue<T>& other)
	    : TempCircularQueueOverlay<T>(
	          allocate<T>(other.capacity()), other.capacity()) {
		copy_impl(other);
	}
	Buffer_CircularQueue(Buffer_CircularQueue<T>&& other) : TempCircularQueueOverlay<T>(other) {
		// just use the copy constructor of TempCircularQueueOverlay - shallow copy
		other.null_impl();
	}
	Buffer_CircularQueue& operator=(Buffer_CircularQueue<T> other) {
		this->swap_impl(other);
		return *this;
	}

private:
	// cannot be swap_impl because ambiguity with base's swap_impl
	// this function exists for potential future expansion
	void swap(Buffer_CircularQueue<T>& other) {
		swap_impl(other);
	}
};

template <class T>
class Buffer_RingBuffer : public RingBufferOverlay<T> {
public:
	Buffer_RingBuffer(u32 capacity)
	    : RingBufferOverlay<T>(
	          allocate<T>(capacity), capacity) {}
	~Buffer_RingBuffer() {
		if (!this->isNull_impl()) {
			this->destruct_impl(); // destroy live elements before freeing
			free(this->data());
		}
	}

	Buffer_RingBuffer(const Buffer_RingBuffer<T>& other)
	    : RingBufferOverlay<T>(
	          allocate<T>(other.capacity()), other.capacity()) {
		copy_impl(other);
	}
	Buffer_RingBuffer(Buffer_RingBuffer<T>&& other) : RingBufferOverlay<T>(other) {
		// just use the copy constructor of RingBufferOverlay - shallow copy
		other.null_impl();
	}
	Buffer_RingBuffer<T>& operator=(Buffer_RingBuffer<T> other) {
		this->swap_impl(other);
		return *this;
	}

private:
	// cannot be swap_impl because ambiguity with base's swap_impl
	// this function exists for potential future expansion
	void swap(Buffer_RingBuffer<T>& other) {
		swap_impl(other);
	}
};
} // namespace tx::esp