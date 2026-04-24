#include <bits/stdc++.h>
#include "tx/math.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tx/csl_engine.h"

class ILI9488DriverPanel {
	using io_t = esp_lcd_panel_io_handle_t;

public:
	static void init(io_t io) { initLCD_impl(io); }
	static void draw(io_t io, tx::Coord topLeft, tx::Coord dimension, tx::u16* data) { sendData_impl(io, topLeft, dimension, data); }


private:
	// @param duration ms
	static void delay_impl(tx::u32 duration) {
		vTaskDelay(pdMS_TO_TICKS(duration));
	}

	// clang-format off

	// command system

	// place all commands with default parameter at the top
	inline static constexpr tx::u8 cmd[] = {
		// Initialization
		0xC0, // Power Control 1
		0xC1, // Power Control 2
		0xC5, // VCOM control
		0x36, // memory access control
		0x3A, // Pixel Interface format
		0xB0, // Interface mode
		0xB1, // Framerate control
		0xB4, // Display inversion control
		0xB6, // Display function control
		0xB7, // Entry mode set2
		0xF7, // Adjust control 3
		0x11, // Exit sleep - delay 120 ms after
		0x29, // Display on - delay 25 ms after
		// operations
		0x2A, // Set the X begin and end of the pushing data
		0x2B, // Set the Y begin and end of the pushing data
		0x2C  // Write memory
	};
	struct CommandParamMeta_impl {
		tx::u16 offset = 0;
		tx::u8 size = 0;
	};
	inline static constexpr CommandParamMeta_impl paramMeta[] = {
		{ 0 , 2 },
		{ 2 , 1 },
		{ 3 , 3 },
		{ 6 , 1 },
		{ 7 , 1 },
		{ 8 , 1 },
		{ 9 , 1 },
		{ 10, 1 },
		{ 11, 3 },
		{ 14, 1 },
		{ 15, 4 }
	};
	// stores the tuned parameters
	inline static constexpr tx::u8 param[] = {
		0x17, 0x15,
		0x41,
		0x00, 0x12, 0x80,
		0x48,
		0x55,
		0x00,
		0xA0,
		0x02,
		0x02, 0x02, 0x3B,
		0xC6,
		0xA9, 0x51, 0x2C, 0x82
	};
	inline static constexpr tx::u32 m_cmdParamCount = 11;

	// this is more likely the documentation?
	enum Command_impl : tx::u32 {
		Power1           =  0, // Power Control 1
		Power2           =  1, // Power Control 2
		VCOM             =  2, // VCOM control
		MAD              =  3, // memory access control
		PxFmt            =  4, // Pixel Interface format
		InterfaceMode    =  5, // Interface mode
		FrameRate        =  6, // Framerate control
		DisplayInversion =  7, // Display inversion control
		DisplayFunction  =  8, // Display function control
		EntryMode        =  9, // Entry mode set2
		AdjustControl    = 10, // Adjust control 3
		SleepOut         = 11, // Exit sleep - delay 120 ms after
		DisplayOn        = 12, // Display on - delay 25 ms after
		SetDataRegionX   = 13, // Set the X begin and end of the pushing data
		SetDataRegionY   = 14, // Set the Y begin and end of the pushing data
		WriteMemory      = 15  // Write memory
	};

	// clang-format on

	// basic helper functions

	static void sendCommand_impl(io_t io, tx::u32 commandId, const void* parameter = nullptr, size_t param_size = 0) {
		if (commandId < m_cmdParamCount && !param_size)
			esp_lcd_panel_io_tx_param(io, cmd[commandId], param + paramMeta[commandId].offset, paramMeta[commandId].size);
		else
			esp_lcd_panel_io_tx_param(io, cmd[commandId], parameter, param_size);
	}
	static tx::u8 getCommand(tx::u32 commandId) { return cmd[commandId]; }

	static void setDataRegion_impl(io_t io, tx::Coord topLeft, tx::Coord dimension) {
		tx::Coord bottomRight = topLeft + dimension.offset(-1, -1);
		tx::u8 xparam[] = {
			static_cast<tx::u8>((topLeft.x >> 8) & 0xFF), static_cast<tx::u8>(topLeft.x & 0xFF),
			static_cast<tx::u8>((bottomRight.x >> 8) & 0xFF), static_cast<tx::u8>(bottomRight.x & 0xFF)
		};
		tx::u8 yparam[] = {
			static_cast<tx::u8>((topLeft.y >> 8) & 0xFF), static_cast<tx::u8>(topLeft.y & 0xFF),
			static_cast<tx::u8>((bottomRight.y >> 8) & 0xFF), static_cast<tx::u8>(bottomRight.y & 0xFF)
		};
		esp_lcd_panel_io_tx_param(io, getCommand(SetDataRegionX), xparam, 4);
		esp_lcd_panel_io_tx_param(io, getCommand(SetDataRegionY), yparam, 4);
	}
	static void sendData_impl(io_t io, tx::Coord topLeft, tx::Coord dimension, tx::u16* data) {
		setDataRegion_impl(io, topLeft, dimension);
		esp_lcd_panel_io_tx_color(io, 0x2C, data, dimension.x * dimension.y * sizeof(tx::u16));
	}



	static void initLCD_impl(io_t io) {
		// maybe Gamma correction?
		// https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ILI9488_Init.h

		for (tx::u32 i = 0; i <= AdjustControl; i++)
			sendCommand_impl(io, i);
		sendCommand_impl(io, SleepOut);
		delay_impl(120);
		sendCommand_impl(io, DisplayOn);
		delay_impl(25);
	}
};
using Driver = ILI9488DriverPanel;


class FrameComposer {
public:
	FrameComposer() {
		m_valid = 1;
		// init bus
		esp_lcd_i80_bus_config_t config_bus = {
			.dc_gpio_num = GPIO_NUM_11,
			.wr_gpio_num = GPIO_NUM_12,
			.clk_src = LCD_CLK_SRC_DEFAULT,

			.data_gpio_nums = {
			    GPIO_NUM_1,
			    GPIO_NUM_2,
			    GPIO_NUM_3,
			    GPIO_NUM_4,
			    GPIO_NUM_5,
			    GPIO_NUM_6,
			    GPIO_NUM_7,
			    GPIO_NUM_8 },
			.bus_width = 8,

			.max_transfer_bytes = 2 * 480 * 320,
			.dma_burst_size = 0
		};
		esp_err_t errorBus = esp_lcd_new_i80_bus(&config_bus, &m_bus);
		if (errorBus != ESP_OK) {
			m_valid = 0;
			return;
		}

		// init io
		esp_lcd_panel_io_i80_config_t config_io = {
			.cs_gpio_num = GPIO_NUM_10,
			.pclk_hz = 10 * 1000 * 1000, // DevNote: can be potentially increased up to 30MHz to increase efficiency
			.trans_queue_depth = 10,
			.on_color_trans_done = nullptr,
			.user_ctx = nullptr,
			.lcd_cmd_bits = 8,
			.lcd_param_bits = 8,
			.dc_levels = {
			    // DveNote: cmd and data might need to swap value <------------
			    .dc_idle_level = 0,
			    .dc_cmd_level = 0,
			    .dc_dummy_level = 0,
			    .dc_data_level = 1,
			},
			.flags = {
			    .cs_active_high = false, .reverse_color_bits = 0,
			    .swap_color_bytes = 0, // Set to 1 only if your colors look "swapped" (e.g., Red and Blue are flipped)
			    .pclk_active_neg = 0, // Set to 1 if the display captures data on the falling edge
			    .pclk_idle_low = 0 // Set to 1 if the clock line should stay low when not transmitting
			}
		};
		esp_err_t errorIO = esp_lcd_new_panel_io_i80(m_bus, &config_io, &m_io);
		if (errorIO != ESP_OK) {
			m_valid = 0;
			return;
		}

		// init driver
		Driver::init(m_io);
	}
	~FrameComposer() {
		esp_lcd_panel_io_del(m_io);
		m_io = nullptr;
		esp_lcd_del_i80_bus(m_bus);
		m_bus = nullptr;
	}

	// disable copy and move

	FrameComposer(const FrameComposer&) = delete;
	FrameComposer& operator=(const FrameComposer&) = delete;
	FrameComposer(FrameComposer&&) = delete;
	FrameComposer& operator=(FrameComposer&&) = delete;

	void draw(tx::Coord topLeft, tx::Coord dimension, tx::u16* data) { // DevNote: probably add finish callback for async correctness
		Driver::draw(m_io, topLeft, dimension, data);
	}
	bool valid() const { return m_valid; }

private:
	esp_lcd_i80_bus_handle_t m_bus = nullptr;
	esp_lcd_panel_io_handle_t m_io = nullptr;
	bool m_valid = 0;
};

inline FrameComposer& getFrameComposer() {
	static FrameComposer fc;
	return fc;
}

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


extern "C" void app_main(void) {
	ExpressionEvaluator aaa;
}