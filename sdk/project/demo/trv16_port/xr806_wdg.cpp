#include "driver/chip/hal_wdg.h"

// Watchdog über den originalen XR806-SDK-Treiber füttern.
// Initialisiert oder aktiviert den Watchdog nicht.
extern "C" void xr806_wdt_feed(void) {
    HAL_WDG_Feed();
}

// Vollständiger XR806-Systemneustart über den SDK-Watchdog
extern "C" void xr806_reboot(void) {
    HAL_WDG_Reboot();
}
