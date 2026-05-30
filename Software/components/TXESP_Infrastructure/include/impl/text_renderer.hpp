// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/frame_composer.hpp"

namespace tx::esp {

/**
 * @note
 * this class will allocate memory:
 * - double render buffer, each `width * height * 2` (u16)
 */
class TextRenderer {
public:
	TextRenderer(u32 width, u32 height, std::span<u8> bitmapData)
	    : m_data(
	          width * height * 2,
	          static_cast<u16*>(heap_caps_malloc(width * height * 2, MALLOC_CAP_DMA)),
	          static_cast<u16*>(heap_caps_malloc(width * height * 2, MALLOC_CAP_DMA))),
	      m_bitmap(bitmapData),
	      m_bitmapDimension(width, height),
	      m_bitmapUnitSize(std::ceil(width * height / 8)),
	      m_leftOverLength(width * height % 8) {
	}
	~TextRenderer() {
		heap_caps_free(m_data.frameBufferData[0]);
		heap_caps_free(m_data.frameBufferData[1]);
	}

	/**
	 * @param position topLeft
	 */
	void draw(Coord position, char character) {
		pushFrameData_inline_impl(static_cast<u8>(character - offset));
		getFrameComposer().draw(position, m_bitmapDimension, m_data.frameBuffer[m_currentFrameBufferIndex].data());
		m_currentFrameBufferIndex = !m_currentFrameBufferIndex;
	}

private:
	inline static constexpr const u8 offset = 32;
	inline static constexpr const std::array<u16, 2> color = { 0x0000, 0xFFFF };

private:
	struct Data_impl {
		Data_impl(
		    u32 size,
		    u16* first, u16* second)
		    : frameBufferData({ first, second }),
		      frameBuffer({ std::span<u16>(first, (size_t)size),
		                    std::span<u16>(second, (size_t)size) }) {}
		std::array<u16*, 2> frameBufferData;
		std::array<std::span<u16>, 2> frameBuffer;
	} m_data;
	std::span<u8> m_bitmap;
	Coord m_bitmapDimension;
	u32 m_bitmapUnitSize, m_leftOverLength;
	bool m_currentFrameBufferIndex = 0; // the double buffer swaper
	bool m_valid = 0;

	template <std::invocable<bool> Func>
	void foreachBit_impl(u8 index, Func&& f) {
		u32 offset = index * m_bitmapUnitSize;
		for (u32 i = offset; i < offset + m_bitmapUnitSize - 1; i++) {
			for (i8 j = 7; j >= 0; j--) {
				f((m_bitmap[i] >> j) & 0x1);
			}
		}
		offset += m_bitmapUnitSize - 1;
		for (i8 i = m_leftOverLength; i >= 0; i--) { // the last byte
			f((m_bitmap[offset] >> i) & 0x1);
		}
	}

	void pushFrameData_impl(u8 charIndex) {
		u32 i = 0;
		std::span<u16> frameBuffer = m_data.frameBuffer[m_currentFrameBufferIndex];
		foreachBit_impl(charIndex, [&](bool bit) {
			frameBuffer[i] = color[bit];
			i++;
		});
	}

	/**
	 * @note manuel inlined version of `pushFrameData_impl`
	 * Because i don't trust the ESP32 compiler's optimization
	 */
	void pushFrameData_inline_impl(u8 charIndex) {
		u32 pixelIndex = 0;
		std::span<u16> frameBuffer = m_data.frameBuffer[m_currentFrameBufferIndex];

		u32 offset = charIndex * m_bitmapUnitSize;
		for (u32 i = offset; i < offset + m_bitmapUnitSize - 1; i++) {
			for (i8 j = 7; j >= 0; j--) {
				frameBuffer[pixelIndex] = color[(m_bitmap[i] >> j) & 0x1];
				pixelIndex++;
			}
		}
		offset += m_bitmapUnitSize - 1;
		for (i8 i = m_leftOverLength; i >= 0; i--) { // the last byte
			frameBuffer[pixelIndex] = color[(m_bitmap[offset] >> i) & 0x1];
			pixelIndex++;
		}
	}
};
}