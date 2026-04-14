#include <stdio.h>
#include <driver/i2c.h>
#include "tx/math.h"

extern "C" void app_main(void) {
	tx::vec2 vec;
	tx::vec2 lp = tx::leftPerp(vec);
}