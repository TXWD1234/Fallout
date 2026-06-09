#include "tx/esp.h"

#include <bits/stdc++.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tx/math.h"
#include "tx/bit_trick.hpp"

#include "tx/terminal.h"

#include "tx/compute.h"

extern "C" void app_main(void) {
	tx::compute::MainClass::init();
	{
		tx::compute::MainClass application;
		application.run();
	}
	tx::compute::MainClass::terminate();
}