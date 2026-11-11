#pragma once
// OTA layout fields of the XRadio BOOT section header at flash offset 0.
//
// The ROM reads the OTA geometry from priv[0..2] of this header: FDCM flash and size, FDCM address,
// and the image and XZ slot limits in KiB. A WXU converted from Tuya over the air keeps the Tuya
// bootloader, whose header limits the XZ slot to 400 KiB. ESPHome moves those fields once to the
// layout of its own image.cfg, so converted and wired installs take the same update files.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::xr806 {

static constexpr size_t BOOT_HEADER_BYTES = 64;
static constexpr uint32_t BOOT_HEADER_MAGIC = 0x48495741;  // "AWIH"
static constexpr uint32_t BOOT_HEADER_VERSION = 5;
static constexpr uint32_t BOOT_SECTION_ID = 0xA5FF5A00;
static constexpr uint32_t BOOT_APP_OFFSET = 0x8000;
static constexpr size_t BOOT_HEADER_CHECKSUM_OFFSET = 8;
static constexpr size_t BOOT_HEADER_NEXT_OFFSET = 32;
static constexpr size_t BOOT_HEADER_ID_OFFSET = 36;
static constexpr size_t BOOT_HEADER_PRIV_OFFSET = 40;

struct BootOTALayout {
  uint32_t priv[3];
};

constexpr uint32_t boot_layout_priv0(uint32_t fdcm_bytes) { return (fdcm_bytes << 8) | 0xFF; }
constexpr uint32_t boot_layout_priv2(uint32_t image_kib, uint32_t slot_kib) { return (slot_kib << 16) | image_kib; }

// project/demo/trv16_port/image/xr806/image.cfg in xr806-sdk: FDCM 1276K (4K), image 1276K, XZ slot 640K.
static constexpr BootOTALayout ESPHOME_BOOT_OTA_LAYOUT{
    {boot_layout_priv0(4096), 1276u * 1024u, boot_layout_priv2(1276, 640)}};
// The original WXU Tuya bootloader (BOOT header of the TRV16 2 MiB dump): FDCM 1500K, image 1496K, XZ slot 400K.
static constexpr BootOTALayout TUYA_WXU_BOOT_OTA_LAYOUT{
    {boot_layout_priv0(4096), 1500u * 1024u, boot_layout_priv2(1496, 400)}};

enum class BootLayoutState : uint8_t {
  BOOT_LAYOUT_STATE_INVALID,  // not a valid BOOT section header: never touch it
  BOOT_LAYOUT_STATE_ESPHOME,  // already the ESPHome layout
  BOOT_LAYOUT_STATE_TUYA,     // the original WXU Tuya layout: move it
  BOOT_LAYOUT_STATE_OTHER,    // a valid header with any other layout: leave it alone
};

inline uint32_t boot_header_le32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

inline uint16_t boot_header_sum16(const uint8_t *header) {
  uint16_t sum = 0;
  for (size_t i = 0; i < BOOT_HEADER_BYTES; i += 2)
    sum = static_cast<uint16_t>(sum + (header[i] | (header[i + 1] << 8)));
  return sum;
}

inline bool boot_header_has_layout(const uint8_t *header, const BootOTALayout &layout) {
  for (size_t i = 0; i < 3; ++i) {
    if (boot_header_le32(header + BOOT_HEADER_PRIV_OFFSET + 4 * i) != layout.priv[i])
      return false;
  }
  return true;
}

inline BootLayoutState classify_boot_header(const uint8_t *header) {
  if (boot_header_le32(header) != BOOT_HEADER_MAGIC || boot_header_le32(header + 4) != BOOT_HEADER_VERSION ||
      boot_header_le32(header + BOOT_HEADER_ID_OFFSET) != BOOT_SECTION_ID ||
      boot_header_le32(header + BOOT_HEADER_NEXT_OFFSET) != BOOT_APP_OFFSET || boot_header_sum16(header) != 0xFFFF)
    return BootLayoutState::BOOT_LAYOUT_STATE_INVALID;
  if (boot_header_has_layout(header, ESPHOME_BOOT_OTA_LAYOUT))
    return BootLayoutState::BOOT_LAYOUT_STATE_ESPHOME;
  if (boot_header_has_layout(header, TUYA_WXU_BOOT_OTA_LAYOUT))
    return BootLayoutState::BOOT_LAYOUT_STATE_TUYA;
  return BootLayoutState::BOOT_LAYOUT_STATE_OTHER;
}

/// Write the layout into priv[0..2] and fix the header checksum. Every other byte stays as it was.
inline void set_boot_ota_layout(uint8_t *header, const BootOTALayout &layout) {
  for (size_t i = 0; i < 3; ++i) {
    for (size_t b = 0; b < 4; ++b)
      header[BOOT_HEADER_PRIV_OFFSET + 4 * i + b] = static_cast<uint8_t>(layout.priv[i] >> (8 * b));
  }
  header[BOOT_HEADER_CHECKSUM_OFFSET] = 0;
  header[BOOT_HEADER_CHECKSUM_OFFSET + 1] = 0;
  const uint16_t checksum = static_cast<uint16_t>(0xFFFF - boot_header_sum16(header));
  header[BOOT_HEADER_CHECKSUM_OFFSET] = static_cast<uint8_t>(checksum);
  header[BOOT_HEADER_CHECKSUM_OFFSET + 1] = static_cast<uint8_t>(checksum >> 8);
}

}  // namespace esphome::xr806
