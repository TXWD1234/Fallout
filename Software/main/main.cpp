#include <stdio.h>
#include "tx/math.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"


extern "C" void app_main(void) {
	size_t heapCapacity = heap_caps_get_free_size(MALLOC_CAP_8BIT),
	       internalCapacity = heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
	       dmaCapacity = heap_caps_get_free_size(MALLOC_CAP_DMA),
	       heapChunk = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
	       internalChunk = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
	       dmaChunk = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

	ESP_LOGI("Jerry", "Current Heap Capacity: %u", heapCapacity);
	ESP_LOGI("Jerry", "Current Internal Capacity: %u", internalCapacity);
	ESP_LOGI("Jerry", "Current DMA Capacity: %u", dmaCapacity);
	ESP_LOGI("Jerry", "Current Heap Chunk: %u", heapChunk);
	ESP_LOGI("Jerry", "Current Internal Chunk: %u", internalChunk);
	ESP_LOGI("Jerry", "Current DMA Chunk: %u", dmaChunk);

	void* ptrs[64];
	for (int i = 0; i < 64; i++) { ptrs[i] = nullptr; }
	for (int i = 0; i < 64 && heapChunk >= 1024; i++) {
		ptrs[i] = heap_caps_malloc(heapChunk, MALLOC_CAP_8BIT);
		if (!ptrs[i]) {
			ESP_LOGE("Jerry", "allocation failed!");
			break;
		}
		heapChunk = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
		ESP_LOGI("Jerry", "Current Heap Chunk: %u", heapChunk);
	}
	for (int i = 0; i < 64 && ptrs[i] != nullptr; i++) {
		heap_caps_free(ptrs[i]);
	}
}