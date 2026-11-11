#include <stdint.h>
#include "driver/chip/hal_clock.h"

// Aktuelle XR806-CPU-Frequenz in Hertz
extern "C" uint32_t xr806_cpu_freq_hz(void) {
    return HAL_GetCPUClock();
}
