// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/frame_composer.hpp"

/**
 * @note
 * this class will allocate memory:
 * - double render buffer, each `width * height * 2` (u16)
 */
class TextRenderer {
public:
	TextRenderer(tx::u32 width, tx::u32 height, std::span<tx::u8> bitmapData)
	    : m_data(
	          width * height * 2,
	          static_cast<tx::u16*>(heap_caps_malloc(width * height * 2, MALLOC_CAP_DMA)),
	          static_cast<tx::u16*>(heap_caps_malloc(width * height * 2, MALLOC_CAP_DMA))),
	      m_bitmap(bitmapData),
	      m_bitmapDimension(width, height),
	      m_bitmapUnitSize(std::ceil(width * height / 8)),
	      m_leftOverLength(m_bitmapUnitSize - std::floor(width * height / 8)) {
	}
	~TextRenderer() {
		heap_caps_free(m_data.frameBufferData[0]);
		heap_caps_free(m_data.frameBufferData[1]);
	}

	/**
	 * @param position topLeft
	 */
	void draw(tx::Coord position, char character) {
		pushFrameData_inline_impl(static_cast<tx::u8>(character - offset));
		getFrameComposer().draw(position, m_bitmapDimension, m_data.frameBuffer[m_currentFrameBufferIndex].data());
		m_currentFrameBufferIndex = !m_currentFrameBufferIndex;
	}

private:
	inline static constexpr const tx::u8 offset = 33;
	inline static constexpr const std::array<tx::u16, 2> color = { 0x0000, 0xFFFF };

private:
	struct Data_impl {
		Data_impl(
		    tx::u32 size,
		    tx::u16* first, tx::u16* second)
		    : frameBufferData({ first, second }),
		      frameBuffer({ std::span<tx::u16>(first, (size_t)size),
		                    std::span<tx::u16>(second, (size_t)size) }) {}
		std::array<tx::u16*, 2> frameBufferData;
		std::array<std::span<tx::u16>, 2> frameBuffer;
	} m_data;
	std::span<tx::u8> m_bitmap;
	tx::Coord m_bitmapDimension;
	tx::u32 m_bitmapUnitSize, m_leftOverLength;
	bool m_currentFrameBufferIndex = 0; // the double buffer swaper
	bool m_valid = 0;

	template <std::invocable<bool> Func>
	void foreachBit_impl(tx::u8 index, Func&& f) {
		tx::u32 offset = index * m_bitmapUnitSize;
		for (tx::u32 i = offset; i < offset + m_bitmapUnitSize - 1; i++) {
			for (tx::i8 j = 7; j >= 0; j--) {
				f((m_bitmap[i] >> j) & 0x1);
			}
		}
		offset += m_bitmapUnitSize - 1;
		for (tx::u8 i = 0; i < m_leftOverLength; i++) { // the last byte
			f((m_bitmap[offset] >> i) & 0x1);
		}
	}

	void pushFrameData_impl(tx::u8 charIndex) {
		tx::u32 i = 0;
		std::span<tx::u16> frameBuffer = m_data.frameBuffer[m_currentFrameBufferIndex];
		foreachBit_impl(charIndex, [&](bool bit) {
			frameBuffer[i] = color[bit];
			i++;
		});
	}

	/**
	 * @note manuel inlined version of `pushFrameData_impl`
	 * Because i don't trust the ESP32 compiler's optimization
	 */
	void pushFrameData_inline_impl(tx::u8 charIndex) {
		tx::u32 pixelIndex = 0;
		std::span<tx::u16> frameBuffer = m_data.frameBuffer[m_currentFrameBufferIndex];

		tx::u32 offset = charIndex * m_bitmapUnitSize;
		for (tx::u32 i = offset; i < offset + m_bitmapUnitSize - 1; i++) {
			for (tx::i8 j = 7; j >= 0; j--) {
				frameBuffer[pixelIndex] = color[(m_bitmap[i] >> j) & 0x1];
				pixelIndex++;
			}
		}
		offset += m_bitmapUnitSize - 1;
		for (tx::u8 i = 0; i < m_leftOverLength; i++) { // the last byte
			frameBuffer[pixelIndex] = color[(m_bitmap[offset] >> i) & 0x1];
			pixelIndex++;
		}
	}
};
