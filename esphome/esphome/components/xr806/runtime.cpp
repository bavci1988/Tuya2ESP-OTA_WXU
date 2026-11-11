// ESPHome runtime bridge for the Allwinner/XRadio XR806.
//
// Called from the main FreeRTOS task created by the vendor SDK.
// The SDK must complete platform_init() before entering this function.

extern void setup();
extern void loop();

#include "esphome/core/defines.h"
#ifdef USE_XR806_OTA_LAYOUT_MIGRATION
#include "ota_boot_layout_migration.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#endif
#ifdef USE_XR806_OTA_RUNTIME_PROBE
#include "ota_vendor_sdk_snapshot.h"
#include "esphome/core/log.h"
#endif

#ifdef USE_XR806_LOWPOWER
#include "lowpower_policy.h"

namespace esphome::xr806 {

XR806LowPowerPolicy &lowpower_policy() {
  static XR806LowPowerPolicy policy;
  return policy;
}

}  // namespace esphome::xr806
#endif

extern "C" void xr806_esphome_run(void) {
#ifdef USE_XR806_OTA_LAYOUT_MIGRATION
  // Before setup(): no other task uses flash yet and the radio is still off.
  const auto migration = esphome::xr806::migrate_boot_ota_layout();
  if (migration == esphome::xr806::BootLayoutMigration::BOOT_LAYOUT_MIGRATION_DONE)
    esphome::arch_restart();  // the ROM reads the OTA layout only at boot
#endif
  setup();
#ifdef USE_XR806_OTA_LAYOUT_MIGRATION
  if (migration == esphome::xr806::BootLayoutMigration::BOOT_LAYOUT_MIGRATION_FAILED) {
    ESP_LOGE("xr806.ota", "Moving the bootloader OTA layout failed; the original sector was written back");
  } else if (migration == esphome::xr806::BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED) {
    ESP_LOGW("xr806.ota", "Bootloader OTA layout not moved; updates use the layout of the installed bootloader");
  }
#endif

#ifdef USE_XR806_OTA_RUNTIME_PROBE
  // Read-only diagnostic after platform_init() and ESPHome logger setup.
  // Flash size and prospective upload size are deliberately UNKNOWN (0),
  // so no OTA admission can be produced from these logs alone.
  esphome::xr806::XR806OTARuntimeGeometry geo{};
  if (!esphome::xr806::xr806_capture_vendor_ota_snapshot(0, 0, &geo)) {
    ESP_LOGW("xr806.ota", "Read-only OTA diagnostic: SDK/FDCM snapshot unavailable; standby/OTA unchanged");
  } else {
    ESP_LOGI("xr806.ota", "SDK OTA: run=%u FDCM=%u verified=%u, metadata flash=%u addr=0x%08lx size=%lu",
             static_cast<unsigned>(geo.running_seq), static_cast<unsigned>(geo.fdcm_selected_seq),
             static_cast<unsigned>(geo.fdcm_state_verified), static_cast<unsigned>(geo.metadata_flash),
             static_cast<unsigned long>(geo.metadata_addr), static_cast<unsigned long>(geo.metadata_bytes));
    ESP_LOGI("xr806.ota", "SDK OTA: img0 flash=%u at 0x%08lx, img1 flash=%u at 0x%08lx, BL=%lu",
             static_cast<unsigned>(geo.image_flash[0]), static_cast<unsigned long>(geo.image_addr[0]),
             static_cast<unsigned>(geo.image_flash[1]), static_cast<unsigned long>(geo.image_addr[1]),
             static_cast<unsigned long>(geo.bootloader_bytes));
    ESP_LOGI("xr806.ota", "SDK OTA: image_max=%lu KiB, compressed_max=%lu KiB; policy_compression=%u",
             static_cast<unsigned long>(geo.image_max_kib), static_cast<unsigned long>(geo.compressed_max_kib),
             static_cast<unsigned>(geo.compression_policy));
    ESP_LOGW("xr806.ota", "Diagnostic only: flash bytes unknown, no erase/write/upload/rollback authorization");
  }
#endif

  while (true) {
    loop();
  }
}
