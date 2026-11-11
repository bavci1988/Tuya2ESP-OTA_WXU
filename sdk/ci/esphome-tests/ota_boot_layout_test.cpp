// Host test for esphome/components/xr806/ota_boot_layout.h: moving the BOOT header OTA fields
// from the original WXU Tuya layout to the ESPHome layout.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "esphome/components/xr806/ota_boot_layout.h"
using namespace esphome::xr806;

namespace {

void put_le32(uint8_t *p, uint32_t v) {
  for (int i = 0; i < 4; ++i)
    p[i] = static_cast<uint8_t>(v >> (8 * i));
}

// A BOOT header as the Tuya bootloader on the TRV16 carries it (fields from the 2 MiB dump).
void tuya_header(uint8_t *h) {
  std::memset(h, 0xFF, BOOT_HEADER_BYTES);
  put_le32(h, BOOT_HEADER_MAGIC);
  put_le32(h + 4, BOOT_HEADER_VERSION);
  put_le32(h + 12, 23364);  // bootloader body length
  put_le32(h + 16, 0x230000);
  put_le32(h + 20, 0x230101);
  put_le32(h + 24, 23364);
  put_le32(h + 28, 1);
  put_le32(h + BOOT_HEADER_NEXT_OFFSET, BOOT_APP_OFFSET);
  put_le32(h + BOOT_HEADER_ID_OFFSET, BOOT_SECTION_ID);
  for (int i = 0; i < 3; ++i)
    put_le32(h + BOOT_HEADER_PRIV_OFFSET + 4 * i, TUYA_WXU_BOOT_OTA_LAYOUT.priv[i]);
  h[10] = 0x12;  // body checksum, untouched by the move
  h[11] = 0x34;
  h[8] = h[9] = 0;
  const uint16_t sum = static_cast<uint16_t>(0xFFFF - boot_header_sum16(h));
  h[8] = static_cast<uint8_t>(sum);
  h[9] = static_cast<uint8_t>(sum >> 8);
}

}  // namespace

int main() {
  // The constants are what the ROM reads: FDCM 1500K/1276K, (slot << 16) | image limit, in KiB.
  assert(TUYA_WXU_BOOT_OTA_LAYOUT.priv[0] == 0x001000FFu);
  assert(TUYA_WXU_BOOT_OTA_LAYOUT.priv[1] == 0x00177000u);
  assert(TUYA_WXU_BOOT_OTA_LAYOUT.priv[2] == 0x019005D8u);
  assert(ESPHOME_BOOT_OTA_LAYOUT.priv[1] == 0x0013F000u);
  assert(ESPHOME_BOOT_OTA_LAYOUT.priv[2] == ((640u << 16) | 1276u));

  uint8_t h[BOOT_HEADER_BYTES];
  tuya_header(h);
  assert(classify_boot_header(h) == BootLayoutState::BOOT_LAYOUT_STATE_TUYA);

  uint8_t moved[BOOT_HEADER_BYTES];
  std::memcpy(moved, h, sizeof(h));
  set_boot_ota_layout(moved, ESPHOME_BOOT_OTA_LAYOUT);
  assert(classify_boot_header(moved) == BootLayoutState::BOOT_LAYOUT_STATE_ESPHOME);
  assert(boot_header_sum16(moved) == 0xFFFF);
  // Only the header checksum and priv[0..2] change.
  for (size_t i = 0; i < BOOT_HEADER_BYTES; ++i) {
    const bool field = (i >= 8 && i < 10) || (i >= BOOT_HEADER_PRIV_OFFSET && i < BOOT_HEADER_PRIV_OFFSET + 12);
    if (!field)
      assert(moved[i] == h[i]);
  }
  // Moving again is a no-op.
  uint8_t again[BOOT_HEADER_BYTES];
  std::memcpy(again, moved, sizeof(moved));
  set_boot_ota_layout(again, ESPHOME_BOOT_OTA_LAYOUT);
  assert(std::memcmp(again, moved, sizeof(moved)) == 0);

  // Anything that is not a valid BOOT header is never touched.
  uint8_t bad[BOOT_HEADER_BYTES];
  std::memcpy(bad, h, sizeof(h));
  bad[20] ^= 1;  // checksum no longer matches
  assert(classify_boot_header(bad) == BootLayoutState::BOOT_LAYOUT_STATE_INVALID);
  std::memcpy(bad, h, sizeof(h));
  put_le32(bad + BOOT_HEADER_ID_OFFSET, 0xA5FE5A01);  // APP section, not BOOT
  assert(classify_boot_header(bad) == BootLayoutState::BOOT_LAYOUT_STATE_INVALID);
  std::memset(bad, 0xFF, sizeof(bad));  // erased flash
  assert(classify_boot_header(bad) == BootLayoutState::BOOT_LAYOUT_STATE_INVALID);

  // A valid header with an unknown layout is left alone.
  std::memcpy(bad, h, sizeof(h));
  set_boot_ota_layout(bad, BootOTALayout{{boot_layout_priv0(4096), 1100u * 1024u, boot_layout_priv2(1096, 700)}});
  assert(classify_boot_header(bad) == BootLayoutState::BOOT_LAYOUT_STATE_OTHER);
  return 0;
}
