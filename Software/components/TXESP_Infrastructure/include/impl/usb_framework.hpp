// Copyright (c) 2026 TXCompute. Licensed under the MIT License.

#pragma once
#include "usb/usb_host.h"
#include "usb/usb_helpers.h"
#include "usb/hid_host.h"
#include "usb/hid.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#include <atomic>

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
