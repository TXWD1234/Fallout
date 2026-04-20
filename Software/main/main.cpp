#include <stdio.h>
#include "tx/math.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"


extern "C" void app_main(void) {
	esp_lcd_i80_bus_handle_t bus = nullptr;
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
	esp_lcd_new_i80_bus(&config_bus, &bus);

	esp_lcd_panel_io_handle_t io = nullptr;
	esp_lcd_panel_io_i80_config_t config_io = {
		.cs_gpio_num = GPIO_NUM_10,
		.pclk_hz = 10 * 1000 * 1000,
		.trans_queue_depth = 10,
		.on_color_trans_done = nullptr,
		.user_ctx = nullptr,
		.lcd_cmd_bits = 8,
		.lcd_param_bits = 8,
		.dc_levels = {
		    .dc_idle_level = 0,
		    .dc_cmd_level = 0,
		    .dc_dummy_level = 0,
		    .dc_data_level = 1,
		},
		.flags = {
		    .cs_active_high = false,
		},
	};
	esp_lcd_new_panel_io_i80(bus, &config_io, &io);


	esp_lcd_panel_io_del(io);
	io = nullptr;
	esp_lcd_del_i80_bus(bus);
	bus = nullptr;
}