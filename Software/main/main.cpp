#include <bits/stdc++.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "usb/usb_host.h"
#include "usb/usb_helpers.h"
#include "usb/hid_host.h"
#include "usb/hid.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tx/math.h"
#include "tx/bit_trick.hpp"

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


	inline static bool inited = 0;
	static void initLCD_impl(io_t io) {
		if (inited) return;
		// maybe Gamma correction?
		// https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ILI9488_Init.h

		for (tx::u32 i = 0; i <= AdjustControl; i++)
			sendCommand_impl(io, i);
		sendCommand_impl(io, SleepOut);
		delay_impl(120);
		sendCommand_impl(io, DisplayOn);
		delay_impl(25);
		inited = 1;
	}
};
using Driver = ILI9488DriverPanel;


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






class USBFramework {
	/**
	 * leave the callbacks for user, it is not what a framework should handle
	 * This class is almost just a wrapper to simplify the pipeline of USB Host
	 */
public:
	static void driverInit(hid_host_driver_event_cb_t driverCallback) { init_impl(driverCallback); }
	static void driverUninit() { uninit_impl(); }

	static void interfaceInit(hid_host_device_handle_t device, hid_host_interface_event_cb_t interfaceCallback) {
		hid_host_device_config_t deviceConfig = {
			interfaceCallback, nullptr
		};
		hid_host_device_open(device, &deviceConfig);
		hid_host_device_start(device);
	}
	static void interfaceUninit(hid_host_device_handle_t device) {
		hid_host_device_stop(device);
		hid_host_device_close(device);
	}
	static void interfaceGetReport(
	    hid_host_device_handle_t device,
	    uint8_t* data,
	    size_t dataLengthMax,
	    size_t* dataLength) {
		hid_host_device_get_raw_input_report_data(
		    device, data, dataLengthMax, dataLength);
	}

private:
	inline static std::atomic_bool usbMainLoopRunning = 0;
	inline static TaskHandle_t usbMainLoopTaskHandle{};
	static void usbMainLoop(void*) {
		while (usbMainLoopRunning) {
			uint32_t event_flags;
			usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
		}
	}
	static void init_impl(hid_host_driver_event_cb_t driverCallback) {
		// init USB host
		usb_host_config_t config_usbHost{};
		config_usbHost.intr_flags = ESP_INTR_FLAG_LEVEL1;
		usb_host_install(&config_usbHost);

		usbMainLoopRunning = 1;
		xTaskCreate(
		    usbMainLoop,
		    "USB_Host",
		    4096, nullptr,
		    5, &usbMainLoopTaskHandle);

		// init HID
		hid_host_driver_config_t hid_config = {
			.create_background_task = true,
			.task_priority = 5,
			.stack_size = 4096,
			.core_id = tskNO_AFFINITY,

			.callback = driverCallback,
			.callback_arg = nullptr,
		};
		hid_host_install(&hid_config);
	}
	static void uninit_impl() {
		hid_host_uninstall();
		usbMainLoopRunning = 0;
		usb_host_lib_unblock();
		while (eTaskGetState(usbMainLoopTaskHandle) != eDeleted) {
			// DevNote: maybe log here?
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		usb_host_uninstall();
	}
};
using USBFW = USBFramework;


class USBKeyboardInputHandler {
	/**
	 * Design referenced from GLFW
	 * 
	 * Stats:
	 * Repeat Begin Counter Max: 64
	 * Repeat Counter Max: 4
	 */
public:
	// clang-format off

	// event
	enum class Action : tx::u32 {
		Press,
		Release,
		Repeat
	};
	enum class Key : tx::u32 {
		Invalid       = tx::InvalidU32,
		/* Printable keys */
		Space         = 32,
		Apostrophe    = 39,  /* ' */
		Comma         = 44,  /* , */
		Minus         = 45,  /* - */
		Period        = 46,  /* . */
		Slash         = 47,  /* / */
		Num_0         = 48,
		Num_1         = 49,
		Num_2         = 50,
		Num_3         = 51,
		Num_4         = 52,
		Num_5         = 53,
		Num_6         = 54,
		Num_7         = 55,
		Num_8         = 56,
		Num_9         = 57,
		SemiColon     = 59,  /* ; */
		Equal         = 61,  /* = */
		A             = 65,
		B             = 66,
		C             = 67,
		D             = 68,
		E             = 69,
		F             = 70,
		G             = 71,
		H             = 72,
		I             = 73,
		J             = 74,
		K             = 75,
		L             = 76,
		M             = 77,
		N             = 78,
		O             = 79,
		P             = 80,
		Q             = 81,
		R             = 82,
		S             = 83,
		T             = 84,
		U             = 85,
		V             = 86,
		W             = 87,
		X             = 88,
		Y             = 89,
		Z             = 90,
		LeftBracket   = 91,  /* [ */
		BackSlash     = 92,  /* \ */
		RightBracket  = 93,  /* ] */
		GraveAccent   = 96,  /* ` */

		/* Function keys */
		Escape        = 256,
		Enter         = 257,
		Tab           = 258,
		Backspace     = 259,
		Insert        = 260,
		Delete        = 261,
		Right         = 262,
		Left          = 263,
		Down          = 264,
		Up            = 265,
		PageUp        = 266,
		PageDown      = 267,
		Home          = 268,
		End           = 269,
		CapsLock      = 280,
		ScrollLock    = 281,
		NumLock       = 282,
		PrintScreen   = 283,
		Pause         = 284,
		F1            = 290,
		F2            = 291,
		F3            = 292,
		F4            = 293,
		F5            = 294,
		F6            = 295,
		F7            = 296,
		F8            = 297,
		F9            = 298,
		F10           = 299,
		F11           = 300,
		F12           = 301,
		F13           = 302,
		F14           = 303,
		F15           = 304,
		F16           = 305,
		F17           = 306,
		F18           = 307,
		F19           = 308,
		F20           = 309,
		F21           = 310,
		F22           = 311,
		F23           = 312,
		F24           = 313,
		F25           = 314,
		KP_Num_0      = 320, // KP stands for "Key Pad"
		KP_Num_1      = 321, // KP stands for "Key Pad"
		KP_Num_2      = 322, // KP stands for "Key Pad"
		KP_Num_3      = 323, // KP stands for "Key Pad"
		KP_Num_4      = 324, // KP stands for "Key Pad"
		KP_Num_5      = 325, // KP stands for "Key Pad"
		KP_Num_6      = 326, // KP stands for "Key Pad"
		KP_Num_7      = 327, // KP stands for "Key Pad"
		KP_Num_8      = 328, // KP stands for "Key Pad"
		KP_Num_9      = 329, // KP stands for "Key Pad"
		KP_Decimal    = 330, // KP stands for "Key Pad"
		KP_Divide     = 331, // KP stands for "Key Pad"
		KP_Multiply   = 332, // KP stands for "Key Pad"
		KP_Minus      = 333, // KP stands for "Key Pad"
		KP_Plus       = 334, // KP stands for "Key Pad"
		KP_Enter      = 335, // KP stands for "Key Pad"
		KP_Equal      = 336, // KP stands for "Key Pad"
		LeftShift     = 340,
		LeftControl   = 341,
		LeftAlt       = 342,
		LeftSuper     = 343,
		RightShift    = 344,
		RightControl  = 345,
		RightAlt      = 346,
		RightSuper    = 347,
		Menu          = 348,
	};
	enum class Mod : tx::u32 {
		Shift         = 0x0001, // 0x22
		Control       = 0x0002, // 0x11
		Alt           = 0x0004, // 0x33
		Super         = 0x0008, // 0x44
		CapsLock      = 0x0010,
		NumLock       = 0x0020,
	};
	// clang-format on

	using InputCallbackFunc_t = void (*)(Key, Action, Mod); // key, action, mod

	static void init() { USBFramework::driverInit(driverCallback_impl); }
	static void setCallback(InputCallbackFunc_t func) { cb = func; }
	static void uninit() { USBFramework::driverUninit(); }

private:
	// clang-format off

	inline static constexpr const Key keyCodeTable[] = {
		Key::Invalid,      // 0x00
		Key::Invalid,      // 0x01
		Key::Invalid,      // 0x02
		Key::Invalid,      // 0x03
		Key::A,            // 0x04
		Key::B,            // 0x05
		Key::C,            // 0x06
		Key::D,            // 0x07
		Key::E,            // 0x08
		Key::F,            // 0x09
		Key::G,            // 0x0A
		Key::H,            // 0x0B
		Key::I,            // 0x0C
		Key::J,            // 0x0D
		Key::K,            // 0x0E
		Key::L,            // 0x0F
		Key::M,            // 0x10
		Key::N,            // 0x11
		Key::O,            // 0x12
		Key::P,            // 0x13
		Key::Q,            // 0x14
		Key::R,            // 0x15
		Key::S,            // 0x16
		Key::T,            // 0x17
		Key::U,            // 0x18
		Key::V,            // 0x19
		Key::W,            // 0x1A
		Key::X,            // 0x1B
		Key::Y,            // 0x1C
		Key::Z,            // 0x1D
		Key::Num_1,        // 0x1E
		Key::Num_2,        // 0x1F
		Key::Num_3,        // 0x20
		Key::Num_4,        // 0x21
		Key::Num_5,        // 0x22
		Key::Num_6,        // 0x23
		Key::Num_7,        // 0x24
		Key::Num_8,        // 0x25
		Key::Num_9,        // 0x26
		Key::Num_0,        // 0x27
		Key::Enter,        // 0x28
		Key::Escape,       // 0x29
		Key::Backspace,    // 0x2A
		Key::Tab,          // 0x2B
		Key::Space,        // 0x2C
		Key::Minus,        // 0x2D
		Key::Equal,        // 0x2E
		Key::LeftBracket,  // 0x2F
		Key::RightBracket, // 0x30
		Key::BackSlash,    // 0x31
		Key::Invalid,      // 0x32
		Key::SemiColon,    // 0x33
		Key::Apostrophe,   // 0x34
		Key::GraveAccent,  // 0x35
		Key::Comma,        // 0x36
		Key::Period,       // 0x37
		Key::Slash,        // 0x38
		Key::CapsLock,     // 0x39
		Key::F1,           // 0x3A
		Key::F2,           // 0x3B
		Key::F3,           // 0x3C
		Key::F4,           // 0x3D
		Key::F5,           // 0x3E
		Key::F6,           // 0x3F
		Key::F7,           // 0x40
		Key::F8,           // 0x41
		Key::F9,           // 0x42
		Key::F10,          // 0x43
		Key::F11,          // 0x44
		Key::F12,          // 0x45
		Key::PrintScreen,  // 0x46
		Key::ScrollLock,   // 0x47
		Key::Pause,        // 0x48
		Key::Insert,       // 0x49
		Key::Home,         // 0x4A
		Key::PageUp,       // 0x4B
		Key::Delete,       // 0x4C
		Key::End,          // 0x4D
		Key::PageDown,     // 0x4E
		Key::Right,        // 0x4F
		Key::Left,         // 0x50
		Key::Down,         // 0x51
		Key::Up,           // 0x52
		Key::NumLock,      // 0x53
		Key::KP_Divide,    // 0x54
		Key::KP_Multiply,  // 0x55
		Key::KP_Minus,     // 0x56
		Key::KP_Plus,      // 0x57
		Key::KP_Enter,     // 0x58
		Key::KP_Num_1,     // 0x59
		Key::KP_Num_2,     // 0x5A
		Key::KP_Num_3,     // 0x5B
		Key::KP_Num_4,     // 0x5C
		Key::KP_Num_5,     // 0x5D
		Key::KP_Num_6,     // 0x5E
		Key::KP_Num_7,     // 0x5F
		Key::KP_Num_8,     // 0x60
		Key::KP_Num_9,     // 0x61
		Key::KP_Num_0,     // 0x62
		Key::KP_Decimal,   // 0x63
		Key::Invalid,      // 0x64
		Key::Menu,         // 0x65
		Key::Invalid,      // 0x66
		Key::KP_Equal,     // 0x67
		Key::F13,          // 0x68
		Key::F14,          // 0x69
		Key::F15,          // 0x6A
		Key::F16,          // 0x6B
		Key::F17,          // 0x6C
		Key::F18,          // 0x6D
		Key::F19,          // 0x6E
		Key::F20,          // 0x6F
		Key::F21,          // 0x70
		Key::F22,          // 0x71
		Key::F23,          // 0x72
		Key::F24,          // 0x73
	};
	// clang-format on

	inline static InputCallbackFunc_t cb = nullptr;

	static void driverCallback_impl(
	    hid_host_device_handle_t device,
	    const hid_host_driver_event_t event, void*) {

		if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
			USBFramework::interfaceInit(device, interfaceCallback_impl);
		}
	}
	static void interfaceCallback_impl(
	    hid_host_device_handle_t device,
	    const hid_host_interface_event_t event, void*) {
		switch (event) {
		case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
			USBFramework::interfaceUninit(device);
			break;
		case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
			tx::u8 data[8];
			size_t dataSize;
			USBFramework::interfaceGetReport(device, data, 8, &dataSize);
			handleReport_impl(data, dataSize);
			break;
		default:
			break;
		}
	}

	// Actual handler logic

	inline static constexpr const tx::u8 RepeatBeginCounterMax = 64; // DevNote: Maybe make these configurable?
	inline static constexpr const tx::u8 RepeatCounterMax = 4;

	// runtime data
	struct KeyCacheEntry_impl {
		Key key;
		tx::u16 repeatBeginCounter; // this is just a "have we waited long enough to start repeating" flag effectively
		tx::u16 repeatCounter;
		void clear() {
			key = Key::Invalid;
			repeatBeginCounter = 0;
			repeatCounter = 0;
		}
	};
	inline static constexpr const tx::u32 KeyCacheSize = 6;
	inline static KeyCacheEntry_impl keyCache[KeyCacheSize] = {
		{ Key::Invalid, 0, 0 },
		{ Key::Invalid, 0, 0 },
		{ Key::Invalid, 0, 0 },
		{ Key::Invalid, 0, 0 },
		{ Key::Invalid, 0, 0 },
		{ Key::Invalid, 0, 0 }
	};
	inline static tx::u32 keyCacheOffset = 0;
	inline static Mod modifier;

	inline static constexpr const tx::u32 KeyReportKeySize = 6;

	// the entry function of key handling
	static void handleReport_impl(tx::u8* reportData, size_t size) {
		size -= 2;
		const tx::u8* data = reportData + 2; // current pressed key data

		// * modifier
		handleModifier_impl(reportData[0]);
		/**
		 * @note
		 * The other half of the modifier logic (CapsLock and NumLock)
		 * is after the current key analysis, together with `handlePress_impl`
		 */

		// * update key cache
		bool used[KeyReportKeySize] = {};
		bool release[KeyCacheSize] = {};
		bool repeat[KeyCacheSize] = {};
		bool press[KeyReportKeySize] = {};

		// ** key cache integrity check & iteration
		for (tx::u32 i = 0; i < keyCacheOffset; i++) {
			tx::u32 it = keyDataFind_impl(data, keyCache[i].key);
			if (tx::valid(it)) { // it was pressed at last time
				repeat[i] = 1;
				used[it] = 1;
			} else { // release
				release[i] = 1;
			}
		}

		// ** handle new pressed keys
		for (tx::u32 i = 0; i < KeyReportKeySize; i++) {
			press[i] = !used[i] && data[i] <= 0x73 && keyCodeTable[data[i]] != Key::Invalid;
		}

		// * handle key event callback
		for (tx::u32 i = 0; i < KeyReportKeySize; i++) { // the other half of the modifier logic
			if (!press[i]) continue;
			press[i] = 0; // to erase it from press if it's a modifier
			Key key = keyCodeTable[data[i]];

			if (key == Key::CapsLock)
				tx::bit::flip(modifier, Mod::CapsLock);
			else if (key == Key::NumLock)
				tx::bit::flip(modifier, Mod::NumLock);
			else
				press[i] = 1; // add it back if it's not a modifier
		}

		for (tx::u32 i = 0; i < keyCacheOffset; i++) { // repeat
			if (repeat[i]) handleRepeat_impl(i);
		}
		for (int i = static_cast<int>(keyCacheOffset) - 1; i >= 0; i--) { // release - iterating backward to avoid "move and iterate" index problem
			if (release[i]) handleRelease_impl(i);
		}
		for (tx::u32 i = 0; i < KeyReportKeySize; i++) { // press
			if (press[i]) handlePress_impl(data[i]);
		}
		/**
		 * @note
		 * This order matters!
		 * release change indices order.
		 * repeat  operates on the original indices.
		 * press   don't care about indice placement, but must be after release, otherwise overflow may occur.
		 * So... press have to be after release, and repeat have to be before release... and we have the order!
		 * repeat -> release -> press
		 */
	}
	static void handleModifier_impl(tx::u8 mod) {
		// clang-format off
		tx::bit::set(modifier, Mod::Control, tx::bit::contains_any(mod, (tx::u8)0x11));
		tx::bit::set(modifier, Mod::Shift,   tx::bit::contains_any(mod, (tx::u8)0x22));
		tx::bit::set(modifier, Mod::Alt,     tx::bit::contains_any(mod, (tx::u8)0x44));
		tx::bit::set(modifier, Mod::Super,   tx::bit::contains_any(mod, (tx::u8)0x88));
		// clang-format on
	}
	static void handleRepeat_impl(tx::u32 keyIndex) {
		KeyCacheEntry_impl& key = keyCache[keyIndex];
		if (key.repeatBeginCounter >= RepeatBeginCounterMax) { // already at repeating stage
			// key.repeatBeginCounter = 0; <---- this used to be a big bug
			if (key.repeatCounter >= RepeatCounterMax) { // counter terminates; call click event
				key.repeatCounter = 0;
				callCallback_impl(key.key, Action::Repeat);
			} else
				key.repeatCounter++;
		} else
			key.repeatBeginCounter++;
	}
	static void handleRelease_impl(tx::u32 keyIndex) {
		KeyCacheEntry_impl& key = keyCache[keyIndex];
		callCallback_impl(key.key, Action::Release);
		keyCacheRemove_impl(keyIndex);
	}
	static void handlePress_impl(tx::u8 keyCode) {
		Key key = keyCodeTable[keyCode];

		callCallback_impl(key, Action::Press);
		keyCacheAdd_impl(key);
	}
	static bool isModifier_impl(Key key) { return key == Key::CapsLock || key == Key::NumLock; }
	static void callCallback_impl(Key key, Action action) {
		if (!cb) return;
		cb(key, action, modifier);
	}

	// key cache operations

	static tx::u32 keyDataFind_impl(const tx::u8* data, Key key) {
		for (tx::u32 i = 0; i < 6; i++) {
			if (keyCodeTable[data[i]] == key) return i;
		}
		return tx::InvalidU32;
	}
	static bool keyDataInclude_impl(const tx::u8* data, Key key) {
		return tx::valid(keyDataFind_impl(data, key));
	}
	static void keyCacheRemove_impl(tx::u32 index) {
		if (keyCacheOffset == 0) return;
		keyCacheOffset--;
		while (index < keyCacheOffset) {
			keyCache[index] = keyCache[index + 1];
			index++;
		}
		keyCache[keyCacheOffset].clear();
	}
	static void keyCacheAdd_impl(Key key) {
		if (keyCacheOffset >= KeyCacheSize) return;
		keyCache[keyCacheOffset].key = key;
		keyCache[keyCacheOffset].repeatBeginCounter = 0;
		keyCache[keyCacheOffset].repeatCounter = 0;
		keyCacheOffset++;
	}
};
using InputHandler = USBKeyboardInputHandler;





extern "C" void app_main(void) {
	//ExpressionEvaluator aaa;
}