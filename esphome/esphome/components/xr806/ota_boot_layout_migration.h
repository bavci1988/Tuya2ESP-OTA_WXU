#pragma once
#include "esphome/core/defines.h"
#if defined(USE_XR806) && defined(USE_XR806_OTA_LAYOUT_MIGRATION)
#include <cstdint>

namespace esphome::xr806 {

enum class BootLayoutMigration : uint8_t {
  BOOT_LAYOUT_MIGRATION_NOT_NEEDED,  // the BOOT header already has the ESPHome layout
  BOOT_LAYOUT_MIGRATION_DONE,        // moved from the Tuya layout and verified; restart to use it
  BOOT_LAYOUT_MIGRATION_SKIPPED,     // nothing written: unknown header, pending update or no memory
  BOOT_LAYOUT_MIGRATION_FAILED,      // sector 0 could not be written and verified
};

/// Move the OTA fields of the BOOT header from the original WXU Tuya layout to the ESPHome layout.
/// Runs before setup(), so the vendor flash driver is ready but nothing else uses flash yet.
BootLayoutMigration migrate_boot_ota_layout();

}  // namespace esphome::xr806
#endif
