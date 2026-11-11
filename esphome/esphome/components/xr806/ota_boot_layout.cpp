#include "esphome/core/defines.h"
#if defined(USE_XR806) && defined(USE_XR806_OTA_LAYOUT_MIGRATION)
#include "ota_boot_layout_migration.h"
#include "ota_boot_layout.h"
#include "esphome/core/helpers.h"
extern "C" {
#include "image/flash.h"
#include "image/image.h"
}

namespace esphome::xr806 {

namespace {
constexpr uint32_t BOOT_SECTOR_ADDR = 0;
constexpr uint32_t SECTOR_BYTES = 4096;
constexpr uint8_t WRITE_ATTEMPTS = 3;

bool write_sector(uint32_t flash, const uint8_t *data, uint8_t *readback) {
  return flash_erase(flash, BOOT_SECTOR_ADDR, SECTOR_BYTES) == 0 &&
         flash_write(flash, BOOT_SECTOR_ADDR, data, SECTOR_BYTES) == SECTOR_BYTES &&
         flash_read(flash, BOOT_SECTOR_ADDR, readback, SECTOR_BYTES) == SECTOR_BYTES &&
         std::memcmp(data, readback, SECTOR_BYTES) == 0;
}

BootLayoutMigration rewrite_boot_sector(uint32_t flash, uint8_t *original, uint8_t *updated, uint8_t *readback) {
  if (flash_read(flash, BOOT_SECTOR_ADDR, original, SECTOR_BYTES) != SECTOR_BYTES ||
      classify_boot_header(original) != BootLayoutState::BOOT_LAYOUT_STATE_TUYA)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  std::memcpy(updated, original, SECTOR_BYTES);
  set_boot_ota_layout(updated, ESPHOME_BOOT_OTA_LAYOUT);
  if (classify_boot_header(updated) != BootLayoutState::BOOT_LAYOUT_STATE_ESPHOME)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  // The new FDCM sector may hold leftovers of the Tuya update; an empty FDCM boots seq0.
  if (flash_erase(flash, ESPHOME_BOOT_OTA_LAYOUT.priv[1], SECTOR_BYTES) != 0)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  // From the first erase of sector 0 until a verified write, a power loss leaves no bootloader.
  for (uint8_t attempt = 0; attempt < WRITE_ATTEMPTS; ++attempt) {
    if (write_sector(flash, updated, readback))
      return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_DONE;
  }
  for (uint8_t attempt = 0; attempt < WRITE_ATTEMPTS; ++attempt) {
    if (write_sector(flash, original, readback))
      break;
  }
  return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_FAILED;
}
}  // namespace

BootLayoutMigration migrate_boot_ota_layout() {
  const image_ota_param_t *iop = image_get_ota_param();
  if (iop == nullptr)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  const uint32_t flash = iop->flash[0];
  uint8_t header[BOOT_HEADER_BYTES];
  if (flash_read(flash, BOOT_SECTOR_ADDR, header, sizeof(header)) != sizeof(header))
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  switch (classify_boot_header(header)) {
    case BootLayoutState::BOOT_LAYOUT_STATE_ESPHOME:
      return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_NOT_NEEDED;
    case BootLayoutState::BOOT_LAYOUT_STATE_TUYA:
      break;
    default:
      return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  }
  // An update committed under the old layout waits in the old XZ slot; moving the slot now would make the
  // bootloader expand whatever is at the new address. Let the bootloader install it first.
  image_cfg_t cfg{};
  if (image_get_cfg(&cfg) == 0 && cfg.seq != 0)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;

  RAMAllocator<uint8_t> allocator;
  uint8_t *buffer = allocator.allocate(3 * SECTOR_BYTES);
  if (buffer == nullptr)
    return BootLayoutMigration::BOOT_LAYOUT_MIGRATION_SKIPPED;
  const BootLayoutMigration result =
      rewrite_boot_sector(flash, buffer, buffer + SECTOR_BYTES, buffer + 2 * SECTOR_BYTES);
  allocator.deallocate(buffer, 3 * SECTOR_BYTES);
  return result;
}

}  // namespace esphome::xr806
#endif
