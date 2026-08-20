#pragma once
#include <freertos/FreeRTOS.h>
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline int xSemaphoreTake(SemaphoreHandle_t, unsigned long) { return 1; }
inline int xSemaphoreGive(SemaphoreHandle_t) { return 1; }
