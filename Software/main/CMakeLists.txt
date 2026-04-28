idf_component_register(
	SRCS
		"main.cpp"
	INCLUDE_DIRS
		"."
	REQUIRES
		"driver"
		"esp_driver_gpio"
		"esp_lcd"
		"esp_timer"
		"esp_common"
		"espressif__usb"
		"espressif__usb_host_hid"

		"TXLib"
		"TXCSL_Engine"
		"TXESP_Infrastructure"
)