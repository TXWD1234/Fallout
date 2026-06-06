// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "tx/math.h"
#include "tx/data.h"
#include "tx/esp.h"
#include <algorithm>
#include <bit>


/**
 * The terminal engine is the very end of the TXESP infrastructure, combining
 * keyboard input and screen / display output
 */



namespace tx::terminal {

/**
 * Architecture Design Pattern:
 * Logics are separated into different classes, each are friend with whoever
 * going to use it. It will contain a pointer to the object class that it
 * belongs to, as it's parent, and modify data there.
 *
 * The terminal engine is designed under the KISS rule:
 * Keep it Simple, Stupid
 */






/**
 * @note
 * this class will allocate memory:
 * - string buffer - size of provided capacity
 * - metadata buffer - size of provided max string count
 *
 * manages the shrink logic when buffer is full (eviction)
 */
class StringPool {
	/**
	 * Terminology:
	 * - Evict:
	 *   The operation of deleting old data to make space for incoming new data,
	 *   when buffer is full
	 * - Object / Meta / Range:
	 *   These terms refers to an entry in m_meta, which is corresponding to a
	 *   range of data in m_data. Conceptually simillar to a partition
	 *
	 */
public:
	StringPool(u32 characterCapacity, u32 maxStringCount)
	    : m_data(characterCapacity), m_meta(maxStringCount) {
		m_data.resize_no_zero_init(m_data.capacity());
	}

	u32 add(std::string_view str) {
		// DevNote: add assert for string too big
		u32 index = addObject_impl(str.size());
		std::copy(str.begin(), str.end(), m_data.begin() + m_meta[index].offset);
		return index;
	}
	u32 add(u32 size) {
		return addObject_impl(size);
	}

	std::string_view get(u32 index) const {
		// DevNote: add assert for index out of bound
		Range_impl meta = m_meta[index];
		return std::string_view(
		    m_data.begin() + meta.offset,
		    m_data.begin() + meta.offset + meta.size);
	}
	std::span<char> getSpan(u32 index) {
		Range_impl meta = m_meta[index];
		return std::span<char>(
		    m_data.begin() + meta.offset,
		    m_data.begin() + meta.offset + meta.size);
	}

	u32 getSize(u32 index) const { return m_meta[index].size; }


	// delete the oldest string object
	// this function will still invoke the delete callback
	u32 deleteOldest() {
		return makeMetaVacancyAdjacent_impl();
	}
	u32 getOldestIndex() {
		return m_metastate.backBegin;
	}




private:
	// basically std::string_view
	struct Range_impl {
		u32 offset, size;
		u32 end() const { return offset + size; }
	};

public:
	// Callback

	// this callback will be called before every string object that is deleted
	// due to eviction
	// since it's called before deletion, the actual meta object and the data
	// it represents will still exist, and be accessable
	// @param u32 the index / handle of the deleting object
	using ObjectDeleteCallback_t = std::function<void(u32)>;

	ObjectDeleteCallback_t m_deleteCb;
	bool m_deleteCbSetted = false;

	void setDeleteCallback(ObjectDeleteCallback_t& cb) {
		m_deleteCb = cb;
		m_deleteCbSetted = true;
	}
	template <std::invocable<u32> Func>
	void setDeleteCallback(Func&& cb) {
		m_deleteCb = ObjectDeleteCallback_t(std::forward<Func>(cb));
		m_deleteCbSetted = true;
	}

private:
	esp::Buffer_GrowArray<char> m_data;
	esp::Buffer_GrowArray<Range_impl> m_meta;
	/**
	 * The m_meta describes the objects / ranges that is stored in m_data
	 * The order of meta objects in m_meta must reflect the actual data order
	 * of them in m_data.
	 */


	// meta buffer house keeping
	/**
	 * The eviction will delete objects from the begin of buffer, and insert
	 * new data at the empty space cleared out
	 *
	 * The MetaState_impl marks the availability of buffer m_meta.
	 * And the memory availability can be calculated by the metadata stored in
	 * m_meta.
	 *
	 *
	 *
	 * Memory Layout for m_meta: (worse case)
	 * [-- occupied --][-- empty --][-- occupied --][-- empty --]
	 *
	 * - The first part is the new data that replaced the old, evicted data. it
	 *   always start at begin of buffer, and its end is marked by `frontEnd`,
	 *   which stands for "end of front"
	 * - The second part is the extra meta objects deleted, in the case of the
	 *   memory of one object cannot hold all the data of the incoming string.
	 *   Its end is marked by `backBegin`, which stands for "begin of back"
	 * - The third part is the old data that remained from previous. The first
	 *   entry of it is the oldest string object in the pool. Its begin is
	 *   marked by `backBegin`, which is also the next element to be deleted if
	 *   eviction is requested.
	 * - The fourth part is the uninitialized memory of the buffer, which is
	 *   expected to be small, since because of it cannot fit the incoming
	 *   string, eviction was invoked, created part 1 and 2.
	 *
	 * [new data | deleted extras | old data | uninitialized]
	 *  ^        ^                ^          ^              ^
	 *  0     frontEnd      backBegin   m_meta.size()  m_meta.capacity()
	 *
	 * When frontEnd == 0 and backBegin == 0, it indicates that the buffer is
	 * linear, meaning that there was no eviction happend, or all previous
	 * generation data are all evicted - the eviction is resolved
	 */
	struct MetaState_impl {
		u32 frontEnd = 0, backBegin = 0;
	} m_metastate;

	// entry point of the entire evicting logic
	// called by add()
	// @param size the requested size of new data
	// @return the index of the meta object in m_meta. That meta object
	//         represents the free space for the new data to occupy in
	u32 addObject_impl(u32 size) {
		if (size > m_data.capacity()) {
			// DevNote: error - single string overflows the entire buffer
			return InvalidU32;
		}
		/**
		 * Edge case: when everything is empty
		 */
		if (m_meta.empty()) {
			m_meta.push_back(Range_impl{ 0, size });
			m_metastate.frontEnd = 0;
			m_metastate.backBegin = 0;
			return 0;
		}

		u32 index;
		Range_impl object = makeMemoryVacancy_impl(size, index);
		if (index == InvalidU32) { // require makeMetaVacancy_impl
			bool bufferLayoutChanged = false;
			index = makeMetaVacancy_impl(bufferLayoutChanged);
			if (bufferLayoutChanged) {
				// now memory place need to be determind again, since memory
				// layout of m_meta have to match m_data

				// clearing meta object data after deletion for meta integrity
				// preventing the curruption of frontEndIndex calculation in
				// the second makeMemoryVacancy_impl call.
				m_meta[index] = {};

				u32 temp; // we don't care about it's potential index
				// because the index is already determinded
				object = makeMemoryVacancy_impl(size, temp);
			}
		}
		m_meta[index] = object;
		return index;
	}


	// delete a meta object - handles all the memory wise delete operations
	void deleteObject_impl(u32 index) {
		// this function don't actually run any deletion logic, because there
		// is no destructor to call - everything is POD
		if (m_deleteCbSetted)
			m_deleteCb(index);
	}

	// eviction
	/**
	 * There are 2 types of overflows, which both cause a eviction:
	 * (only matters in implementation)
	 * - memory overflow: when incoming string size overflows memory capacity
	 * - meta overflow: when metadata buffer is full
	 *
	 * Memory eviction is always resolved first, since as if a deletion of
	 * string object is made by memory eviction, the meta overflows always
	 * resolves itself.
	 */



	// eviction resolution
	// - deicde whether invoke a eviction or not


	// @param size requested size of the incoming data
	// @param index the result index of the new data's meta object; if is
	//              InvalidU32 then it need to be resolved by
	//              makeMetaVacancy_impl
	// @return the resolved meta object (a range) of the incoming data
	Range_impl makeMemoryVacancy_impl(u32 size, u32& index) {
		/**
		 * There are 4 cases to this function:
		 * case 1: Linear     - eviction never happend / is fully resolved,
		 *                      everything is linear. But could invoke eviction
		 *                      when reaches the end of capacity
		 * case 2: Interspace - no eviction is invoked, the space lefted by
		 *                      previous eviction is enough for this incoming
		 *                      object
		 * case 3: Pushing    - some objects from m_metastate.backBegin until
		 *                      the end of buffer need to be evicted in order
		 *                      to fit the incoming data
		 * case 4: Wrapping   - the space from m_metastate.frontEnd until the
		 *                      end of buffer is not enough no fit the incoming
		 *                      data; start from the very beginning of the
		 *                      buffer to find enough space for the new data;
		 *                      Everything from the original m_metastate.backEnd
		 *                      to the end of buffer and from the begin of
		 *                      buffer to the new m_metastate.backBegin will all
		 *                      be deleted.
		 */

		// error case: frontEnd > backBegin
		if (m_metastate.frontEnd > m_metastate.backBegin) {
			// DevNote: error
			return Range_impl{ InvalidU32, InvalidU32 };
		}

		// resolve backBegin == end: all previous object deleted; become linear
		if (m_metastate.backBegin >= m_meta.size()) {
			// remove any potential garbage object in interspace
			if (m_metastate.frontEnd != m_metastate.backBegin)
				m_meta.erase(m_meta.begin() + m_metastate.frontEnd, m_meta.end());

			m_metastate.frontEnd = 0;
			m_metastate.backBegin = 0;
		}

		// case 1
		if (m_metastate.frontEnd == 0) {
			u32 currentBufferSize = m_meta.empty() ? 0 : m_meta.back().end();
			if (size > m_data.capacity() - currentBufferSize) {
				// requested size overflows remaining capacity - evict
				index = 0;
				return makeMemoryVacancyBegin_impl(size);
			}
			index = InvalidU32;
			return Range_impl{ currentBufferSize, size };
		}

		u32 frontEndIndex = m_meta[m_metastate.frontEnd - 1].end();
		u32 freeCapacity = m_meta[m_metastate.backBegin].offset -
		                   frontEndIndex;
		// case 2
		if (size <= freeCapacity) {
			// nothing to do
			index = InvalidU32;
			return Range_impl{
				frontEndIndex, size
			};
		}

		// case 3
		if (size <= m_data.capacity() - frontEndIndex) {
			u32 furtherRequiredSize = size - freeCapacity;
			m_metastate.backBegin = makeMemoryVacancyEvict_impl(
			    furtherRequiredSize, m_metastate.backBegin);
			index = InvalidU32;
			return Range_impl{
				frontEndIndex, size
			};
		}

		// case 4
		index = InvalidU32;
		return makeMemoryVacancyBegin_impl(size);
	}
	// this is not a function to solve a case; it's a generic helper
	// After this function returns, there is possibility of the size is not
	// satisfied, but instead had reached the end of m_meta. Depndending on the
	// context, this should be accounted
	// @return the new backBegin / the index of the next object of the last
	//         object deleted
	// @param size the size that is requested
	// @param first the first object to be evicted
	u32 makeMemoryVacancyEvict_impl(u32 size, u32 first) {
		u32 currentSize = 0;
		u32 i = first;
		for (; i < m_meta.size(); i++) {
			deleteObject_impl(i);
			currentSize += m_meta[i].size;
			if (currentSize >= size) {
				i++;
				break;
			}
		}
		return i;
	}
	// find a free space from the beginning
	Range_impl makeMemoryVacancyBegin_impl(u32 size) {
		m_metastate.backBegin = makeMemoryVacancyEvict_impl(size, 0);
		if (m_metastate.backBegin >= m_meta.size()) {
			// absolute everything got deleted
			m_metastate.frontEnd = 0;
			m_metastate.backBegin = 0;
			m_meta.clear();
			m_meta.push_back({});
		} else {
			m_metastate.frontEnd = 1; // there is one object at the very start
		}

		return Range_impl{ 0, size };
	}

	/**
	 * After makeMetaVacancy_impl returns there are 3 possibilities for the
	 * value of it's return value: index of the meta object
	 * 1. Linear, normal push, index = m_meta.size() - 1;
	 * 2. Evicted, normal push, index = old m_metastate.frontEnd
	 * 3. Linear, push but overflows capacity . evict invoked, index = 0
	 * Notice that in case 3 the memory place need to change, and
	 * makeMemoryVacancy_impl need to be called one more time, since memory
	 * layout of m_meta have to match m_data
	 */

	// context: a new object is inserting
	// call occasion: after memory eviction resolution is over
	// design: lazy, meaning that things don't check the state after they are
	//         done - the next operation checks it.
	// the entry point of meta eviction resolution
	// @param bufferLayoutChanged set to true when buffer order had changed by
	//                            reorder of m_meta; will be used to invoke the
	//                            second makeMemoryVacancy_impl call
	// @return index in m_meta of the object in vacancy
	u32 makeMetaVacancy_impl(bool& bufferLayoutChanged) {
		/**
		 * There are 4 cases (excluding error cases) to this function:
		 * case 1: Linear     - eviction never happend / is fully resolved,
		 *                      everything is linear. But could invoke eviction
		 *                      when reaches the end of capacity
		 * case 2: Full       - frontEnd reached the end of buffer; all
		 *                      previous generation of data is deleted;
		 *                      push_back required; everything resolve back to
		 *                      linear (case 1)
		 * case 3: Interspace - there are more empty slots of meta object
		 *                      remained from previous eviction - just use them
		 * case 4: Adjacent   - the frontEnd had catched up to backBegin there
		 *                      is no more empty slots - meta eviction invokes
		 */

		bufferLayoutChanged = false;

		// error case: frontEnd > backBegin
		if (m_metastate.frontEnd > m_metastate.backBegin) {
			// DevNote: error
			return InvalidU32;
		}

		// resolve backBegin == end: all previous object deleted; become linear
		if (m_metastate.backBegin >= m_meta.size()) {
			// remove any potential garbage object in interspace
			m_meta.erase(m_meta.begin() + m_metastate.frontEnd, m_meta.end());

			m_metastate.frontEnd = 0;
			m_metastate.backBegin = 0;
		}

		// case 1
		if (m_metastate.frontEnd == 0) {
			return makeMetaVacancyLinear_impl(bufferLayoutChanged);
		}

		// case 2
		// unreachable because resolve backBegin == end above already solved it
		// if (m_metastate.frontEnd >= m_meta.size()) {
		// 	m_metastate.frontEnd = 0;
		// 	m_metastate.backBegin = 0;
		// 	return makeMetaVacancyLinear_impl();
		// }

		// case 3
		if (m_metastate.frontEnd < m_metastate.backBegin) {
			m_metastate.frontEnd++;
			return m_metastate.frontEnd - 1;
		}

		// case 4
		if (m_metastate.frontEnd == m_metastate.backBegin) {
			return makeMetaVacancyAdjacent_impl();
		}
		// DevNote: error case - frontEnd bigger then backBegin
		return InvalidU32;
	}
	u32 makeMetaVacancyAdjacent_impl() {
		// DevNote: assertion
		// assert(m_metastate.backBegin < m_meta.size());
		deleteObject_impl(m_metastate.backBegin);
		m_metastate.backBegin++;
		m_metastate.frontEnd++;
		return m_metastate.frontEnd - 1;
	}
	u32 makeMetaVacancyLinear_impl(bool& bufferLayoutChanged) {
		if (m_meta.size() == m_meta.capacity()) {
			// meta overflow - meta eviction invokes; then switch to adjacent
			// - directly use adjacent here because adjacent already included
			//   eviction.
			bufferLayoutChanged = true; // declare buffer layout change, since
			// m_meta wrapping back to start means that memory place need to be
			// re-determinded from the start as well
			// only set here but not makeMetaVacancyAdjacent_impl because in
			// the general makeMetaVacancyAdjacent_impl case the memory layout
			// is not garanteed to change
			return makeMetaVacancyAdjacent_impl();
		}
		m_meta.push_back({});
		return m_meta.size() - 1;
	}
};


/**
 * @note
 * this class will allocate memory:
 * - double text buffer (left and right buffer) of the max buffer size provided
 *
 */
class InputLine {
public:
	InputLine(tx::u32 maxBufferSize)
	    : m_data(maxBufferSize), m_maxLineSize(maxBufferSize) {}

	// user operations
	// @return the operations is valid (false for invalid)

	bool input(char val) {
		if (size() >= m_maxLineSize) return false;
		m_data.left.push_back(val);
		return true;
	}
	// backspace
	bool deleteFront() {
		if (m_data.left.empty()) return false;
		m_data.left.pop_back();
		return true;
	}
	// delete
	bool deleteBack() {
		if (m_data.right.empty()) return false;
		m_data.right.pop_back();
		return true;
	}
	bool cursorMoveLeft() {
		if (m_data.left.empty()) return false;
		m_data.right.push_back(m_data.left.back());
		m_data.left.pop_back();
		return true;
	}
	bool cursorMoveRight() {
		if (m_data.right.empty()) return false;
		m_data.left.push_back(m_data.right.back());
		m_data.right.pop_back();
		return true;
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
	u32 size() {
		return m_data.left.size() + m_data.right.size();
	}

	void clear() {
		m_data.left.clear();
		m_data.right.clear();
	}

	// return the character that is `offset` character left of the cursor
	// 0 is the closest character to the cursor
	// out of bound is UB
	char leftChar(u32 offset) {
		return m_data.left[m_data.left.size() - 1 - offset];
	}
	// return the character that is `offset` character right of the cursor
	// 0 is the closest character to the cursor
	// out of bound is UB
	char rightChar(u32 offset) {
		return m_data.right[m_data.right.size() - 1 - offset];
	}

	u32 leftSize() const { return m_data.left.size(); }
	u32 rightSize() const { return m_data.right.size(); }

private:
	struct Data_impl {
		Data_impl(tx::u32 size)
		    : left(size),
		      right(size) {}

		esp::Buffer_GrowArray<char> left;
		esp::Buffer_GrowArray<char> right;
	} m_data;
	u32 m_maxLineSize;
};



struct InputEvent {
	esp::USBKeyboardInputHandler::Key key;
	esp::USBKeyboardInputHandler::Mod mod;
	esp::USBKeyboardInputHandler::Action action;
};

/**
 * Flatten the call back based input into a buffer
 *
 * This class will be pushing input struct to a circular queue, which is set by
 * the higher level end of this class.
 * This class is a static class, as required by the callback of
 * esp::USBKeyboardInputHandler
 */
class InputHandler {
	/**
	 * How the concurrenct data flow works:
	 * The callback of esp::USBKeyboardInputHandler will push data into the buffer
	 * It will only push, and if befroe it push full() returned true, it's
	 * exception
	 * 
	 */
public:
	static void init() {
		if (m_inited) return;
		m_inited = true;
		esp::USBKeyboardInputHandler::init();
		esp::USBKeyboardInputHandler::setCallback(keyboardCallback);
	}
	static void uninit() { esp::USBKeyboardInputHandler::uninit(); }
	static void setBuffer(esp::TempCircularQueueOverlay<InputEvent>& buffer, std::mutex& lock) {
		m_buffer = &buffer;
		m_lock = &lock;
	}

	static void stale() {
		m_running = false;
	}
	static void unstale() {
		m_running = true;
	}

	static bool valid() { return m_valid && m_inited && m_buffer != nullptr; }

private:
	inline static esp::TempCircularQueueOverlay<InputEvent>* m_buffer = nullptr;
	inline static std::mutex* m_lock = nullptr;
	inline static bool m_valid = true;
	inline static bool m_inited = false;

	inline static std::atomic_bool m_running = true;

	static void keyboardCallback(
	    esp::USBKeyboardInputHandler::Key key,
	    esp::USBKeyboardInputHandler::Action action,
	    esp::USBKeyboardInputHandler::Mod mod) {
		if (!m_running) return;
		std::lock_guard<std::mutex> lock(*m_lock);
		if (m_buffer->full()) {
			m_valid = false;
			return;
		}
		m_buffer->push(InputEvent{ key, mod, action });
	}
};










struct Font {
	u32 width;
	u32 height;
	std::span<u8> bitmapData;
};
inline esp::TextRenderer makeTextRenderer(Font font) {
	return esp::TextRenderer{
		font.width,
		font.height,
		font.bitmapData
	};
}


class TerminalEngine {
	/**
	 * This Terminal Engine is highly optimized for memory usage, but not performance.
	 * Both the update and render logic is designed with tight memory constraint in mind
	 */
public:
	// type def

	using InputCallback_t = std::function<void(std::string_view)>;

private:
	using Key = esp::USBKeyboardInputHandler::Key;
	using Mod = esp::USBKeyboardInputHandler::Mod;
	using Action = esp::USBKeyboardInputHandler::Action;

public:
	TerminalEngine(Font font, Coord textGridDimension)
	    : m_inputBuffer(InputBufferSize),
	      m_inputLocalBuffer(InputBufferSize),
	      m_inputLine(InputLineBufferSize),
	      m_stringPool(StringPoolCharSize, StringPoolStrSize),
	      m_lineBuffer(LineBufferSize),
	      m_rr(makeTextRenderer(font)),
	      m_gridSpanDimension(textGridDimension),
	      m_gridUnitDimension(m_screenDimension / textGridDimension),
	      m_lhCache(
	          std::bit_ceil(static_cast<u32>(textGridDimension.y))) {
		init_impl();
	}

	// invoke async update
	// return boolean: success
	bool poll() {
		if (!InputHandler::valid()) {
			return false;
		}

		// pull data from input buffer
		{
			std::lock_guard<std::mutex> lock(m_inputLock);
			// since there is lock, there will not be any input at this point
			while (!m_inputBuffer.empty()) {
				m_inputLocalBuffer.push_back(m_inputBuffer.front());
				m_inputBuffer.pop();
			}
		}
		for (const InputEvent& i : m_inputLocalBuffer) {
			handleInputEvent_impl(i);
		}
		m_inputLocalBuffer.clear();
		return true;
	}

	// print a dynamic string
	// the value of input `str` will be copied into TemrinalEngine's internal
	// string buffer, and the source of `str` can be safely overwritten or
	// freed after
	void print(std::string_view str) {
		// add to string pool
		u32 id = m_stringPool.add(str);

		// add to line buffer
		lineBufferPush_impl(Line_impl{ id });
		renderEvent_output();
	}
	// print a static string preset
	// the input `str` have to guarantee to not be freed
	// UB if the memory `str` points to is invalidated
	void printStatic(const char* str) {
		// add to line buffer
		lineBufferPush_impl(Line_impl{ str });
		renderEvent_output();
	}
	// note: use `const char*` to indicate that this have to be a C string with
	// '\0' indicator at the end

	// init data setter

	// callback when an complete line is entered
	void setInputCallback(InputCallback_t cb) {
		m_inputCb = cb;
		m_inputCbInited = true;
	}
	// callback when an complete line is entered
	template <std::invocable<std::string_view> Func>
	void setInputCallback(Func&& f) {
		m_inputCb = std::forward<Func>(f);
		m_inputCbInited = true;
	}

private:
	inline static constexpr const u32 InputBufferSize = 16;
	inline static constexpr const u32 InputLineBufferSize = 128;
	inline static constexpr const u32 StringPoolCharSize = 65536;
	inline static constexpr const u32 StringPoolStrSize = 512;
	inline static constexpr const u32 LineBufferSize = 1024;

	/**
	 * Memory Usage
	 * 
	 * Each line:
	 * - Absolute length max: 128
	 * 
	 * Terminal Line Buffer:
	 * * each entry is an user input and a program output
	 * - max entry size: 1024
	 * 
	 * StringPool:
	 * - String Capacity: 1024
	 * - Character Capacity: 512 * 128 = 65536 (64Kb)
	 * 
	 * Total Memory Usage
	 * - String Pool: 65536 + 512 * 8 = 69632
	 * - Line Buffer: 1024 * (8 + 4 + 4) = 16384
	 * - Input Line: 128 * 2 = 256
	 * - Input Buffer: 16 * 12 = 192
	 * 
	 * Total: 86464b (84kb)
	 * 
	 */

private:
	// ==========================================
	// **************** Callback ****************
	// ==========================================

	InputCallback_t m_inputCb;
	bool m_inputCbInited = false;

private:
	// =========================================================
	// **************** source data (init data) ****************
	// =========================================================

	// clang-format off
	inline static constexpr const i8 keyShiftTable[] = {
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		' ', /*   */
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		'"', /* ' */
		i8{-1}, i8{-1}, i8{-1}, i8{-1},
		'<', /* , */
		'_', /* - */
		'>', /* . */
		'?', /* / */
		'!', /* 0 */
		'@', /* 1 */
		'#', /* 2 */
		'$', /* 3 */
		'%', /* 4 */
		'^', /* 5 */
		'&', /* 6 */
		'*', /* 7 */
		'(', /* 8 */
		')', /* 9 */
		i8{-1},
		':', /* ; */
		i8{-1},
		'+', /* = */
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		i8{-1}, i8{-1}, i8{-1}, i8{-1}, i8{-1},
		'{', /* [ */
		'|', /* \ */
		'}', /* ] */
		i8{-1}, i8{-1},
		'~', /* ` */
		'A', /* a */
		'B', /* b */
		'C', /* c */
		'D', /* d */
		'E', /* e */
		'F', /* f */
		'G', /* g */
		'H', /* h */
		'I', /* i */
		'J', /* j */
		'K', /* k */
		'L', /* l */
		'M', /* m */
		'N', /* n */
		'O', /* o */
		'P', /* p */
		'Q', /* q */
		'R', /* r */
		'S', /* s */
		'T', /* t */
		'U', /* u */
		'V', /* v */
		'W', /* w */
		'X', /* x */
		'Y', /* y */
		'Z', /* z */
	};
	// clang-format on

private:
	// ==============================================
	// **************** Impl Structs ****************
	// ==============================================

	/**
	 * a line could be either an entry in the string pool
	 * or a string that's outside of the TerminalEngine
	 * There is only one u32 value in this struct. it could either be a u32
	 * id in the StringPool, or a const char* pointer since on 32 bit
	 * architecture a pointer is 4 bytes.
	 * the last bit is used as the boolean flag to identify whether it's an
	 * id (1) or a pointer (0)
	 * 
	 * Specificly for an id, there is another boolean flag: isUserInput
	 * (does not exist for pointer because user input could never be a pointer)
	 * this boolean flag is the second least significant digit, at the left of
	 * the type flag.
	 */
	struct Line_impl {
		u32 m_value = 0;
		Line_impl() {}
		Line_impl(u32 val) { set_impl(val); }
		Line_impl(const char* val) { set_impl(val); }

		bool isPointer() const { return bit::contains_none(m_value, u32{ 0x1 }); }
		u32 id() const { return m_value >> 2; }
		const char* pointer() const {
			return reinterpret_cast<const char*>(m_value);
		}
		Line_impl& operator=(u32 val) {
			set_impl(val);
			return *this;
		}
		Line_impl& operator=(const char* val) {
			set_impl(val);
			return *this;
		}

		Line_impl userInput() {
			return bit::setTrue_copy(m_value, u32{ 0b10 });
		}
		bool isUserInput() {
			return bit::contains_all(m_value, u32{ 0b10 });
		}

	private:
		void set_impl(u32 val) {
			m_value = bit::setTrue_copy(val << 2, u32{ 0x1 });
		}
		void set_impl(const char* val) {
			m_value = bit::setFalse_copy(reinterpret_cast<u32>(val), u32{ 0x1 });
		}
	};

private:
	// ============================================
	// **************** Core Logic ****************
	// ============================================

	// input
	esp::Buffer_CircularQueue<InputEvent> m_inputBuffer;
	std::mutex m_inputLock;
	esp::Buffer_GrowArray<InputEvent> m_inputLocalBuffer;

	InputLine m_inputLine;

	// storage buffers
	StringPool m_stringPool;
	esp::Buffer_RingBuffer<Line_impl> m_lineBuffer;

	void init_impl() {
		InputHandler::init();
		InputHandler::setBuffer(m_inputBuffer, m_inputLock);

		m_stringPool.setDeleteCallback([&](u32 id) {
			this->handleDeleteEvent_impl(id);
		});
	}

	struct State_impl {
		bool isLineBufferEvicting = false;
		bool isStringPoolEvicting = false;
	} m_state;

	void handleInputEvent_impl(const InputEvent& event) {
		if (event.key == Key::Invalid) return;

		if (event.action == Action::Release) {
			return;
		};

		if (isPrintableKey(event.key) && bit::contains_none(event.mod, Mod::Control)) {
			char c = static_cast<char>(enumval(event.key));
			if (bit::contains_any(event.mod, Mod::Shift))
				c = keyShiftTable[static_cast<int>(c)];
			if (bit::contains_any(event.mod, Mod::CapsLock))
				c = reverseCase(c);

			m_inputLine.input(c);
			renderEvent_input();
			return;
		}
		// functional keys
		switch (event.key) {
		case Key::Enter:
			handleNewLine_impl();
			renderEvent_enter();
			break;
		case Key::Backspace:
			if (m_inputLine.deleteFront())
				renderEvent_backspace();
			break;
		case Key::Delete:
			if (m_inputLine.deleteBack())
				renderEvent_delete();
			break;
		case Key::Left:
			if (m_inputLine.cursorMoveLeft())
				renderEvent_moveLeft();
			break;
		case Key::Right:
			if (m_inputLine.cursorMoveRight())
				renderEvent_moveRight();
			break;
		default:
			break;
		}
	}

	void handleNewLine_impl() {
		// extract input string
		u32 id = m_stringPool.add(m_inputLine.size());
		m_inputLine.output(m_stringPool.getSpan(id));
		m_inputLine.clear();

		// update line buffer
		lineBufferPush_impl(Line_impl{ id }.userInput());

		// invoke user input callback
		if (m_inputCbInited)
			m_inputCb(m_stringPool.get(id));
	}
	void lineBufferPush_impl(Line_impl line) {
		// push line buffer
		if (m_lineBuffer.full()) lineBufferEvict_impl();
		m_lineBuffer.push_back(line);

		// update lhCache
		if (m_lhCacheState.bottomLineEnd == 0) {
			if (m_lhCache.full()) lhCache_pop_front();
			lhCache_push_back(line);
		}
	}

	// @return the type of line is evicted: pointer (0 / preset) or id (1 / literal)
	u8 lineBufferEvict_impl() {
		Line_impl line = m_lineBuffer.front();
		m_lineBuffer.pop_front();
		m_lhCacheState.topLine--; // since topLine is index in
		// lineBuffer, it need to be updated to match it's original address

		if (line.isPointer()) return 0;
		if (m_state.isStringPoolEvicting) return 1;

		// not a pointer - it's an id in StringPool
		m_state.isLineBufferEvicting = true;
		u32 id = line.id();
		if (m_stringPool.getOldestIndex() != id) {
			// DevNote: this will be a problem
			// either stringPool delete callback didn't delete properly, or
			// data curruption....
			return InvalidU8;
		}
		m_stringPool.deleteOldest();
		m_state.isLineBufferEvicting = false;
		return 1;
	}

	void handleDeleteEvent_impl(u32 id) {
		m_state.isStringPoolEvicting = true;
		if (!m_state.isLineBufferEvicting) {
			// evict line buffer
			while (lineBufferEvict_impl() == 0 &&
			       !m_lineBuffer.empty()) {}
		}
		m_state.isStringPoolEvicting = false;
	}


private:
	// ========================================
	// **************** Render ****************
	// ========================================

	/**
	 * Terminology:
	 * - Logical Line / Line_impl: the entry in lineBuffer, that
	 *   represents one line without wrapping
	 * - Screen Line / Render Line: the actual displaying line of text on the
	 *   screen, consider wrapping
	 */

	esp::TextRenderer m_rr;
	Coord m_gridSpanDimension; // the grid dimension of the entire screen
	Coord m_gridUnitDimension; // the pixel dimension of each grid

	inline static constexpr const Coord m_screenDimension{ 480, 320 };

	// ################ line height cache ################
	/**
	 * front is top, back is bottom (of the screen)
	 * Stores the height (screen line count) of every logical line that are
	 * visible on the screen
	 * It's index is linearly parallel to lineBuffer, meaning that the second
	 * entry in lhCache is corresponding to:
	 * m_lineBuffer[m_lhCacheState.topLine + 1]
	 */
	esp::Buffer_RingBuffer<u32> m_lhCache;

	struct LhCacheState_impl {
		/**
		 * topLine is index of the first Line_impl that is visible on
		 * the screen. It is also the index of Line_impl that the front() of
		 * lhCache is according to.
		 * topLineBegin is the index of the screen line that is the
		 * actual first line on the screen. it is the index within the range
		 * of the topLine object
		 * bottomLineEnd is the index of the screen line that is the next line
		 * of the actual last line on the screen. it is the index within the
		 * range of the last logical line in lhCache. 0 is invalid state to this
		 * variable, since if it's 0 nothing from the last logical line is
		 * visible, then by correct logic that logical line shouldn't exist (it
		 * will just be the bottomLineEnd be the height of the last logical
		 * line). therefore when bottomLineEnd is 0, the only possible state is
		 * that there had never been scrolled.
		 * 
		 * Abstraction:
		 * bottomLineEnd is the "end" index, therefore it could also be
		 * considered as the visible size of the last logical line.
		 * 
		 */
		u32 topLine = 0, topLineBegin = 0, bottomLineEnd = 0;
		/**
		 * the total amount of screen lines of all the logical lines that are
		 * currently visible on screen, including the invisible screen lines of
		 * the top and bottom logical line
		 */
		u32 screenLineCount = 0;
	} m_lhCacheState;


	u32 findLineHeight_impl(Line_impl line) {
		std::string_view lineStr = getLineStr(line);
		return divideCeil(static_cast<u32>(lineStr.size()), m_gridSpanDimension.x);
	}
	u32 findLineHeight_impl(u32 lineIndex) {
		return findLineHeight_impl(m_lineBuffer[lineIndex]);
	}

	void lhCache_push_back(u32 height) {
		m_lhCache.push_back(height);
		m_lhCacheState.screenLineCount += m_lhCache.back();
	}
	void lhCache_pop_back() {
		m_lhCacheState.screenLineCount -= m_lhCache.back();
		m_lhCache.pop_back();
	}
	void lhCache_push_front(u32 height) {
		m_lhCache.push_front(height);
		m_lhCacheState.screenLineCount += m_lhCache.front();
	}
	void lhCache_pop_front() {
		m_lhCacheState.screenLineCount -= m_lhCache.front();
		m_lhCache.pop_front();
	}

	void lhCache_push_back(Line_impl line) {
		lhCache_push_back(findLineHeight_impl(line));
	}
	void lhCache_push_front(Line_impl line) {
		lhCache_push_front(findLineHeight_impl(line));
	}


	// core rendering

	/**
	 * The dimension of the screen in pixel is always 480*320
	 * The origin is at top left.
	 */

	struct RenderState_impl {
		/**
		 * cursorPos always points to the end of left buffer (exclusively), and
		 * points to the begin of right buffer (inclusively).
		 * Speaking in plain english: cursorPos is the first character of right
		 * buffer, which is also the character behind the last character of
		 * left buffer
		 */
		Coord cursorPos;
	} m_renderState;

	// ---------------------------------------------
	// ++++++++++++++++ RenderEvent ++++++++++++++++
	// ---------------------------------------------
	/**
	 * The render events related with the inputLine will be reading state from
	 * the inputLine
	 * Every event is an action that user can perform, and will be directly
	 * called from the event handler - encapsulation
	 */

	void renderEvent_input() {
		// if an input just happened, the inputed character must be the first
		// character to the left of the cursor in the inputLine
		renderImpl_drawChar(m_renderState.cursorPos,
		                    m_inputLine.leftChar(0));
		renderEvent_moveRight();
		renderImpl_redrawBack();
	}
	void renderEvent_delete() {
		renderImpl_redrawBack();
	}
	void renderEvent_backspace() {
		renderEvent_moveLeft();
		renderImpl_redrawBack();
	}
	void renderEvent_moveLeft() {
		renderImpl_cursorMoveLeft(m_renderState.cursorPos);
		renderImpl_scrollToCursor();
	}
	void renderEvent_moveRight() {
		renderImpl_cursorMoveRight(m_renderState.cursorPos);
		renderImpl_scrollToCursor();
	}

	void renderEvent_enter() {
		renderImpl_scrollToEnd();
		renderImpl_fullRedraw();
	}

	void renderEvent_output() {
		renderImpl_scrollToEnd();
		renderImpl_fullRedraw();
	}

	// ################ triggering redraw ################
	// offset is unit of actual screen line / y position
	// any invalid offset input is an hard UB

	/**
	 * There are 2 things to consider in this function:
	 * 1. Call `renderImpl_scrollDown / _scrollUp`, which update lhCache,
	 *    providen context for the actual rendering
	 * 2. Trigger redraw. Use the updated lhCache to draw the new text grid
	 */

	void renderEvent_scrollDown(u32 offset) {
		m_renderState.cursorPos.y -= offset;
		renderImpl_scrollDown(offset);
		renderImpl_fullRedraw();
	}
	void renderEvent_scrollUp(u32 offset) {
		m_renderState.cursorPos.y += offset;
		renderImpl_scrollUp(offset);
		renderImpl_fullRedraw();
	}

	// --------------------------------------------
	// ++++++++++++++++ RenderImpl ++++++++++++++++
	// --------------------------------------------

	// ################ Logic ################

	void renderImpl_cursorMoveLeft(Coord& pos) {
		pos.x--;
		if (pos.x < 0) {
			pos.y--;
			pos.x = m_gridSpanDimension.x - 1;
		}
	}
	void renderImpl_cursorMoveRight(Coord& pos) {
		pos.x++;
		if (pos.x >= m_gridSpanDimension.x) {
			pos.y++;
			pos.x = 0;
		}
	}

	void renderImpl_scrollToCursor() {
		if (m_renderState.cursorPos.y >= m_gridSpanDimension.y)
			renderEvent_scrollDown(
			    m_renderState.cursorPos.y - m_gridSpanDimension.y + 1);
		else if (m_renderState.cursorPos.y < 0)
			renderEvent_scrollUp(0 - m_renderState.cursorPos.y);
	}

	// updates only lhCache
	void renderImpl_scrollDown(u32 offset) {
		/**
		 * There are 2 things to consider in this function:
		 * 1. The front of lhCache. They are the logical lines that will become
		 *    invisible because of this scroll
		 * 2. The back of lhCache. They are the logical lines that will become
		 *    visible because of this scroll
		 */

		/**
		 * There are 3 cases to this function:
		 * 1. Simple scroll:   scroll offset is within the height of the first
		 *                     logical line, no change to the front
		 * 2. Complex scroll:  scroll offset is within the screen, at least
		 *                     one logical line entry will last
		 * 3. Complete scroll: scroll offset is greater then the screen,
		 *                     everything in lhCache is reset (cache miss ig)
		 */

		// orientation: push_back, pop_front

		// is already at the end
		if (m_lhCacheState.screenLineCount < m_gridSpanDimension.y ||
		    (m_lhCacheState.topLine + m_lhCache.size() == m_lineBuffer.size() &&
		     m_lhCacheState.bottomLineEnd == m_lhCache.back())) return;

		// ---------------- Front ----------------
		// delete logical line entries that will become invisible from lhCache
		u32 remainedOffset = offset;

		// complete scroll
		if (offset >=
		    m_lhCacheState.screenLineCount - m_lhCacheState.topLineBegin) {
			u32 lineIndex = m_lhCacheState.topLine + m_lhCache.size();
			u32 lineHeight = findLineHeight_impl(lineIndex);

			// march line buffer to find new begin
			while (remainedOffset >= lineHeight) {
				remainedOffset -= lineHeight;
				lineIndex++;
				lineHeight = findLineHeight_impl(lineIndex);
				// DevNote: potential throw: lineIndex out of range
				// possible reason: `offset` is too big
			}

			m_lhCache.clear();
			m_lhCacheState.screenLineCount = 0;
			m_lhCacheState.topLine = lineIndex;
			// the first logical line
			lhCache_push_back(m_lineBuffer[lineIndex]);

			m_lhCacheState.topLineBegin = remainedOffset;
		} else {
			// incomplete scroll
			renderImpl_deleteFront_impl(remainedOffset);
		}

		// ---------------- Back ----------------
		// push new logical lines that became visible (determind bottomLineEnd)
		i32 remainedLineCapacity =
		    m_gridSpanDimension.y -
		    (m_lhCacheState.screenLineCount -
		     m_lhCacheState.topLineBegin);
		// start with the next logical line of the last entry in lhCache
		u32 lineIndex = m_lhCacheState.topLine + m_lhCache.size();
		u32 lineHeight = findLineHeight_impl(lineIndex);

		while (true) {
			remainedLineCapacity -= lineHeight;
			lhCache_push_back(lineHeight);
			if (remainedLineCapacity <= 0) break;
			lineIndex++; // iterate to next line
			if (lineIndex >= m_lineBuffer.size()) {
				/**
				 * this should never happen. because this means the buffer had
				 * ran out to fill the empty space. which then means scrolling
				 * shouldn't even happen (the total line did not filled the
				 * screen yet), or invalid offset was entered
				 */
				// DevNote: Error
				return;
			}
			lineHeight = findLineHeight_impl(lineIndex);
		}

		// remainedLineCapacity must be negative or 0, adding it is equivalant
		// to subtracting or no-op; adding std::min just in case
		m_lhCacheState.bottomLineEnd = std::min(
		    lineHeight + remainedLineCapacity, lineHeight);
	}
	// updates only lhCache
	// mirror of renderImpl_scrollDown
	void renderImpl_scrollUp(u32 offset) {
		// orientation: push_front, pop_back

		// is already at the top
		if (m_lhCacheState.screenLineCount < m_gridSpanDimension.y ||
		    m_lhCacheState.topLine == 0 &&
		        m_lhCacheState.topLineBegin == 0) return;

		// compatibility variables to match the scroll logic

		//u32 bottomLine = m_lhCacheState.topLine + m_lhCache.size() - 1; // inclusive

		// ---------------- Back ----------------
		// delete logical line entries that will become invisible from lhCache
		u32 remainedOffset = offset;

		// complete scroll
		if (offset >=
		    m_lhCacheState.screenLineCount - m_lhCache.back() +
		        m_lhCacheState.bottomLineEnd) {
			u32 lineIndex = m_lhCacheState.topLine - 1;
			u32 lineHeight = findLineHeight_impl(lineIndex);

			// march line buffer to find new bottom
			while (remainedOffset >= lineHeight) {
				remainedOffset -= lineHeight;
				lineIndex--;
				lineHeight = findLineHeight_impl(lineIndex);
				// DevNote: potential throw: lineIndex out of range
				// possible reason: `offset` is too big
			}

			m_lhCache.clear();
			m_lhCacheState.screenLineCount = 0;
			//bottomLine = lineIndex;
			// the first logical line
			m_lhCacheState.topLine = lineIndex; // logically it should bottom
			// line here, but since there is only one line it's the same
			lhCache_push_front(lineHeight);

			m_lhCacheState.bottomLineEnd = lineHeight - remainedOffset;
		} else {
			// incomplete scroll
			u32 lineHeight = m_lhCache.back();
			if (remainedOffset >= lineHeight) {
				// complex scroll
				// march lhCache to find new begin
				do {
					remainedOffset -= lineHeight;
					lhCache_pop_back();
					lineHeight = m_lhCache.back();
					//bottomLine--;
				} while (remainedOffset >= lineHeight);
				m_lhCacheState.bottomLineEnd = lineHeight - remainedOffset;
			} else {
				// simple scroll
				m_lhCacheState.bottomLineEnd -= offset;
			}
		}

		// ---------------- Front ----------------
		// push new logical lines that became visible (determind topLine and
		// topLineBegin)
		i32 remainedLineCapacity =
		    m_gridSpanDimension.y -
		    (m_lhCacheState.screenLineCount -
		     (m_lhCache.back() - m_lhCacheState.bottomLineEnd));
		// start with the previous logical line of the first entry in lhCache
		u32 lineIndex = m_lhCacheState.topLine - 1;
		u32 lineHeight = findLineHeight_impl(lineIndex);

		while (true) {
			remainedLineCapacity -= lineHeight;
			lhCache_push_front(lineHeight);
			if (remainedLineCapacity <= 0) break;
			if (lineIndex == 0) {
				/**
				 * this should never happen. because this means the buffer had
				 * ran out to fill the empty space. which then means scrolling
				 * shouldn't even happen (the total line did not filled the
				 * screen yet), or invalid offset was entered
				 */
				// DevNote: Error
				return;
			}
			lineIndex--; // iterate to next line
			lineHeight = findLineHeight_impl(lineIndex);
		}

		// remainedLineCapacity
		m_lhCacheState.topLineBegin = -remainedLineCapacity;
		m_lhCacheState.topLine = lineIndex;
	}
	void renderImpl_scrollToEnd() {
		if (m_lhCacheState.screenLineCount < m_gridSpanDimension.y ||
		    (m_lhCacheState.topLine + m_lhCache.size() == m_lineBuffer.size() &&
		     m_lhCacheState.bottomLineEnd == m_lhCache.back())) return;

		u32 bottomLine = m_lhCacheState.topLine + m_lhCache.size() - 1;
		renderImpl_deleteFront_impl(
		    m_lhCache.back() - m_lhCacheState.bottomLineEnd);

		// march to find bottom line
		while (bottomLine != m_lineBuffer.size() - 1) {
			bottomLine++;
			u32 lineHeight = findLineHeight_impl(bottomLine);
			renderImpl_deleteFront_impl(lineHeight);
			lhCache_push_back(lineHeight);
		}

		m_lhCacheState.bottomLineEnd = m_lhCache.back();
	}
	void renderImpl_deleteFront_impl(u32 offset) {
		u32 lineHeight = m_lhCache.front();
		if (offset >= lineHeight) {
			// complex scroll
			// march lhCache to find new begin
			do {
				offset -= lineHeight;
				lhCache_pop_front();
				lineHeight = m_lhCache.front();
				m_lhCacheState.topLine++;
			} while (offset >= lineHeight);
			m_lhCacheState.topLineBegin = offset;
		} else {
			// simple scroll
			m_lhCacheState.topLineBegin += offset;
		}
	}


	// ################ Drawing ################

	// encapulates the converstion between grid coord and pixel coord
	void renderImpl_drawChar(Coord pos, char c) {
		m_rr.draw(pos * m_gridUnitDimension, c);
	}

	// redraw based on current cursorPos and inputLine buffer state
	void renderImpl_redrawBack() {
		Coord cursor = m_renderState.cursorPos;
		for (u32 i = 0; i < m_inputLine.rightSize(); i++) {
			renderImpl_drawChar(cursor, m_inputLine.rightChar(i));
			renderImpl_cursorMoveRight(cursor);
		}
	}

	void renderImpl_drawLine(u32 lineIndex, u32 yPos) {
		renderImpl_drawLinePartial(lineIndex, yPos, 0, u32{ 0xFFFF });
	}
	// @param lineBegin the screen line index of the targeting line, that is
	// the begin line to render in the targeting line (start at)
	void renderImpl_drawLinePartialBegin(u32 lineIndex, u32 yPos, u32 lineBegin) {
		renderImpl_drawLinePartial(lineIndex, yPos, lineBegin, u32{ 0xFFFF });
	}
	// @param lineEnd the screen line index of the targeting line, that is
	// the end line to render in the targeting line (end with)
	void renderImpl_drawLinePartialEnd(u32 lineIndex, u32 yPos, u32 lineEnd) {
		renderImpl_drawLinePartial(lineIndex, yPos, 0, lineEnd);
	}
	// @param lineBegin the screen line index of the targeting line, that is
	// the begin line to render in the targeting line (start at)
	// @param lineEnd the screen line index of the targeting line, that is
	// the end line to render in the targeting line (end with)
	void renderImpl_drawLinePartial(u32 lineIndex, u32 yPos, u32 lineBegin, u32 lineEnd) {
		Coord cursor{ 0, static_cast<int>(yPos) };
		std::string_view str = getLineStr(lineIndex);
		u32 beginIndex = lineBegin * m_gridSpanDimension.x;
		u32 endIndex = std::min(static_cast<u32>(str.size()),
		                        lineEnd * m_gridSpanDimension.x);
		for (u32 i = beginIndex; i < endIndex; i++) {
			renderImpl_drawChar(cursor, str[i]);
			renderImpl_cursorMoveRight(cursor);
		}
	}
	/**
	 * Redraw the entire screen according to the lhCache
	 */
	void renderImpl_fullRedraw() {
		if (m_lhCache.size() == 1) {
			renderImpl_drawLinePartial(
			    m_lhCacheState.topLine,
			    0,
			    m_lhCacheState.topLineBegin,
			    m_lhCacheState.bottomLineEnd);
			return;
		}

		u32 screenLineIndex = 0;
		u32 logicalLineIndex = m_lhCacheState.topLine;

		renderImpl_drawLinePartialBegin(
		    logicalLineIndex,
		    screenLineIndex,
		    m_lhCacheState.topLineBegin);
		logicalLineIndex++;
		screenLineIndex += m_lhCache.front() - m_lhCacheState.topLineBegin;
		for (u32 i = 1; i < m_lhCache.size() - 1; i++) {
			renderImpl_drawLine(
			    logicalLineIndex,
			    screenLineIndex);
			logicalLineIndex++;
			screenLineIndex += m_lhCache[i];
		}
		renderImpl_drawLinePartialEnd(
		    logicalLineIndex,
		    screenLineIndex,
		    m_lhCacheState.bottomLineEnd);
	}

private:
	// =========================================
	// **************** Helpers ****************
	// =========================================

	static bool isPrintableKey(Key key) {
		return enumval(key) <= 122;
	}
	static char reverseCase(char c) {
		if (!std::isalpha(c)) return c;
		return c <= 90 ? c + 32 : c - 32;
	}

	static u32 divideCeil(u32 a, u32 b) {
		return (a + b - 1) / b;
	}

	std::string_view getLineStr(u32 index) {
		return getLineStr(m_lineBuffer[index]);
	}
	std::string_view getLineStr(Line_impl line) {
		if (line.isPointer()) {
			return std::string_view(line.pointer());
		} else {
			return m_stringPool.get(line.id());
		}
	}
};
} // namespace tx::terminal

/**
 * Things to add:
 * - output session (when user is inputing, all output is ignored; when program
 *   is outputing, all user input is ignored (`InputHandler::stale()`))
 */