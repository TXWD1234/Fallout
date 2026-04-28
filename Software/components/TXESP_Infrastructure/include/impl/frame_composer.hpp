// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/ili9488_driver_panel.hpp"

class FrameComposer {
public:
	FrameComposer() {
		init_impl();
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

	void init_impl() {
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
};

inline FrameComposer& getFrameComposer() {
	static FrameComposer fc;
	return fc;
}