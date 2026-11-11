#include <stdint.h>
#include "driver/chip/hal_util.h"
#include "driver/chip/hal_rtc.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

// Millisekunden seit Systemstart
extern "C" uint32_t xr806_millis(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// Wartezeit in Millisekunden
extern "C" void xr806_delay(uint32_t ms) {
    if (ms == 0) {
        taskYIELD();
    } else {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

// Mikrosekunden seit Systemstart (RTC Free Running Counter)
extern "C" uint64_t xr806_micros64(void) {
    return HAL_RTC_GetFreeRunTime();
}

// Kurze Wartezeiten mit dem XR806-Hardwaretreiber
extern "C" void xr806_delay_us(uint32_t us) {
    if (us != 0) {
        HAL_UDelay(us);
    }
}
