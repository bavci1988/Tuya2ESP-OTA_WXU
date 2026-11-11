#pragma once

#include <cstddef>
#include <cstdint>

namespace trv16 {

// Bounded, allocation-free Tuya Low-Power v0 frame codec.
// Observed on WXU/XR806 <-> HC32L136 at 115200 8N1.
constexpr std::size_t kMaxPayload = 512;
constexpr std::size_t kMaxFrame = kMaxPayload + 7;

struct Frame {
  uint8_t version;
  uint8_t command;
  const uint8_t *payload; // valid only while within on_frame() callback
  std::size_t payload_length;
};

typedef void (*FrameCallback)(const Frame &, void *context);

class FrameParser {
 public:
  FrameParser();
  void reset();
  void feed(const uint8_t *bytes, std::size_t size, FrameCallback cb, void *context);
  void feed(uint8_t byte, FrameCallback cb, void *context);
  std::size_t good_frames() const { return good_; }
  std::size_t bad_frames() const { return bad_; }

 private:
  uint8_t buf_[kMaxFrame];
  std::size_t pos_;
  std::size_t expected_;
  std::size_t good_;
  std::size_t bad_;
  void begin_sync_from(uint8_t byte);
};

enum DataType : uint8_t { RAW = 0, BOOLEAN = 1, VALUE = 2, STRING = 3, ENUM = 4, BITMAP = 5 };

struct DataPoint {
  uint8_t id;
  uint8_t type;
  const uint8_t *data;
  std::size_t length;
  bool as_bool(bool *out) const;
  bool as_enum(uint8_t *out) const;
  bool as_u32(uint32_t *out) const;
  bool as_i32(int32_t *out) const;
};

typedef void (*DPCallback)(const DataPoint &, void *context);

// Only call for a known DP payload (0x05 reports, 0x09 writes).
// Validates the *entire* sequence before invoking any callbacks.
bool parse_datapoints(const uint8_t *payload, std::size_t length,
                      DPCallback cb, void *context);

// Produces a normal Tuya frame, checksum = sum(all bytes except checksum) & 0xff.
// Returns frame byte length, or 0 on error. Never writes past output capacity.
std::size_t encode_frame(uint8_t command, const uint8_t *payload,
                         std::size_t payload_length, uint8_t *out,
                         std::size_t capacity);

// CMD 0x09: DPID single write. Pure encoding; NOT a UART transmission.
std::size_t encode_dp_write(uint8_t dpid, uint8_t type, const uint8_t *value,
                            std::size_t length, uint8_t *out,
                            std::size_t capacity);

// Empirically observed app-opening status request DPID 106 bool 1.
std::size_t encode_refresh_request(uint8_t *out, std::size_t capacity);

// Observed WXU side acknowledgement of an HC32 0x05 DP report: 00 DPID.
std::size_t encode_dp_ack(uint8_t dpid, uint8_t *out, std::size_t capacity);

struct ThermostatSnapshot {
  bool has_power = false;         // DPID 1 meaning remains a HYPOTHESIS
  bool power = false;
  bool has_mode = false;          // DPID 2 enum
  uint8_t mode = 0;
  bool has_target = false;        // DPID 3, in tenths of degree Celsius
  int32_t target_tenths = 0;
  bool has_current = false;       // DPID 4, in tenths of degree Celsius
  int32_t current_tenths = 0;
  bool has_heating = false;       // DPID 19 confirmed flame indicator
  bool heating = false;
  bool has_child_lock = false;    // DPID 17
  bool child_lock = false;
  bool has_frost = false;         // DPID 20
  bool frost = false;
  bool has_battery_candidate = false; // DPID 16 meaning unconfirmed
  uint32_t battery_candidate = 0;
  bool has_hysteresis = false;    // DPID 105 / 10
  int32_t hysteresis_tenths = 0;
};

// Receives reports only. No WLAN, sleep, writes or ESPHome dependencies.
class ThermostatModel {
 public:
  bool consume(const Frame &frame);
  const ThermostatSnapshot &state() const { return state_; }

 private:
  ThermostatSnapshot state_;
  static void on_dp(const DataPoint &dp, void *ctx);
};

} // namespace trv16
