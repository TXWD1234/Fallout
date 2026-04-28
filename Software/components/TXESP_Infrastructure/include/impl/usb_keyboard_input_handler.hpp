// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "impl/usb_framework.hpp"
#include "tx/math.h"
#include "tx/bit_trick.hpp"

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
