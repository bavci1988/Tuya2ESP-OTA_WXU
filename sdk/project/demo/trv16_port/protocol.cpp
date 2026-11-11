#include "trv16/protocol.hpp"

#include <cstring>

namespace trv16 {

FrameParser::FrameParser() : pos_(0), expected_(0), good_(0), bad_(0) {}

void FrameParser::reset() { pos_ = expected_ = 0; }

void FrameParser::begin_sync_from(uint8_t byte) {
  reset();
  if (byte == 0x55) {
    buf_[0] = 0x55;
    pos_ = 1;
  }
}

void FrameParser::feed(const uint8_t *bytes, std::size_t size, FrameCallback cb,
                       void *context) {
  if (bytes == nullptr) return;
  for (std::size_t i = 0; i < size; ++i) feed(bytes[i], cb, context);
}

void FrameParser::feed(uint8_t byte, FrameCallback cb, void *context) {
  if (pos_ == 0) { begin_sync_from(byte); return; }
  if (pos_ == 1) {
    if (byte == 0xAA) { buf_[pos_++] = byte; return; }
    begin_sync_from(byte); return;
  }
  if (pos_ >= kMaxFrame) { ++bad_; begin_sync_from(byte); return; }
  buf_[pos_++] = byte;
  if (pos_ == 6) {
    std::size_t payload_length = (static_cast<std::size_t>(buf_[4]) << 8) | buf_[5];
    if (payload_length > kMaxPayload) { ++bad_; begin_sync_from(byte); return; }
    expected_ = payload_length + 7;
  }
  if (expected_ == 0 || pos_ < expected_) return;
  uint8_t checksum = 0;
  for (std::size_t i = 0; i + 1 < expected_; ++i)
    checksum = static_cast<uint8_t>(checksum + buf_[i]);
  if (checksum == buf_[expected_ - 1]) {
    ++good_;
    Frame frame{buf_[2], buf_[3], buf_ + 6, expected_ - 7};
    if (cb != nullptr) cb(frame, context);
  } else {
    ++bad_;
  }
  // Callbacks must consume/copy data synchronously (zero-copy view).
  begin_sync_from(byte);
}

bool DataPoint::as_bool(bool *out) const {
  if (out == nullptr || type != BOOLEAN || length != 1 || data[0] > 1) return false;
  *out = data[0] != 0;
  return true;
}

bool DataPoint::as_enum(uint8_t *out) const {
  if (out == nullptr || type != ENUM || length != 1) return false;
  *out = data[0];
  return true;
}

bool DataPoint::as_u32(uint32_t *out) const {
  if (out == nullptr || type != VALUE || length != 4) return false;
  *out = (static_cast<uint32_t>(data[0]) << 24) |
         (static_cast<uint32_t>(data[1]) << 16) |
         (static_cast<uint32_t>(data[2]) << 8) | data[3];
  return true;
}

bool DataPoint::as_i32(int32_t *out) const {
  uint32_t v = 0;
  if (out == nullptr || !as_u32(&v)) return false;
  // Do not rely on implementation-defined unsigned-to-signed overflow conversion.
  int64_t signed_value = v < UINT32_C(0x80000000) ? static_cast<int64_t>(v)
      : static_cast<int64_t>(v) - INT64_C(4294967296);
  *out = static_cast<int32_t>(signed_value);
  return true;
}

bool parse_datapoints(const uint8_t *payload, std::size_t length,
                      DPCallback cb, void *context) {
  if (payload == nullptr || length < 5) return false;
  std::size_t offset = 0;
  // First complete pass; ensures a malformed trailing block cannot publish partial state.
  while (offset < length) {
    if (length - offset < 4) return false;
    uint8_t type = payload[offset + 1];
    if (type > BITMAP) return false;
    std::size_t n = (static_cast<std::size_t>(payload[offset + 2]) << 8) |
                    payload[offset + 3];
    if (n > length - offset - 4) return false;
    if ((type == BOOLEAN || type == ENUM) && n != 1) return false;
    if (type == VALUE && n != 4) return false;
    if (type == BOOLEAN && payload[offset + 4] > 1) return false;
    offset += 4 + n;
  }
  if (offset != length) return false;
  offset = 0;
  while (offset < length) {
    std::size_t n = (static_cast<std::size_t>(payload[offset + 2]) << 8) |
                    payload[offset + 3];
    DataPoint dp{payload[offset], payload[offset + 1], payload + offset + 4, n};
    if (cb != nullptr) cb(dp, context);
    offset += 4 + n;
  }
  return true;
}

std::size_t encode_frame(uint8_t command, const uint8_t *payload,
                         std::size_t length, uint8_t *out,
                         std::size_t capacity) {
  if (out == nullptr || length > kMaxPayload || capacity < length + 7 ||
      (length != 0 && payload == nullptr)) return 0;
  out[0] = 0x55; out[1] = 0xAA; out[2] = 0; out[3] = command;
  out[4] = static_cast<uint8_t>(length >> 8);
  out[5] = static_cast<uint8_t>(length & 0xff);
  if (length) std::memcpy(out + 6, payload, length);
  uint8_t checksum = 0;
  for (std::size_t i = 0; i < length + 6; ++i)
    checksum = static_cast<uint8_t>(checksum + out[i]);
  out[length + 6] = checksum;
  return length + 7;
}

std::size_t encode_dp_write(uint8_t dpid, uint8_t type, const uint8_t *value,
                            std::size_t length, uint8_t *out,
                            std::size_t capacity) {
  if (type > BITMAP || length + 4 > kMaxPayload || (length && value == nullptr)) return 0;
  if ((type == BOOLEAN || type == ENUM) && length != 1) return 0;
  if (type == VALUE && length != 4) return 0;
  if (type == BOOLEAN && value[0] > 1) return 0;
  uint8_t payload[kMaxPayload];
  payload[0] = dpid; payload[1] = type;
  payload[2] = static_cast<uint8_t>(length >> 8);
  payload[3] = static_cast<uint8_t>(length);
  if (length) std::memcpy(payload + 4, value, length);
  return encode_frame(0x09, payload, 4 + length, out, capacity);
}

std::size_t encode_refresh_request(uint8_t *out, std::size_t capacity) {
  const uint8_t value = 1;
  return encode_dp_write(106, BOOLEAN, &value, 1, out, capacity);
}

std::size_t encode_dp_ack(uint8_t dpid, uint8_t *out, std::size_t capacity) {
  const uint8_t ack[] = {0, dpid};
  return encode_frame(0x05, ack, sizeof(ack), out, capacity);
}

void ThermostatModel::on_dp(const DataPoint &dp, void *ctx) {
  ThermostatSnapshot &s = static_cast<ThermostatModel *>(ctx)->state_;
  bool b = false;
  uint8_t e = 0;
  uint32_t u = 0;
  int32_t i = 0;
  switch (dp.id) {
    case 1: if (dp.as_bool(&b)) { s.has_power = true; s.power = b; } break;
    case 2: if (dp.as_enum(&e)) { s.has_mode = true; s.mode = e; } break;
    case 3: if (dp.as_u32(&u) && u <= 1000) { s.has_target = true; s.target_tenths = static_cast<int32_t>(u); } break;
    case 4: if (dp.as_u32(&u) && u <= 1000) { s.has_current = true; s.current_tenths = static_cast<int32_t>(u); } break;
    case 19: if (dp.as_enum(&e) && e <= 1) { s.has_heating = true; s.heating = e == 1; } break;
    case 17: if (dp.as_bool(&b)) { s.has_child_lock = true; s.child_lock = b; } break;
    case 20: if (dp.as_bool(&b)) { s.has_frost = true; s.frost = b; } break;
    case 16: if (dp.as_u32(&u)) { s.has_battery_candidate = true; s.battery_candidate = u; } break;
    case 105: if (dp.as_i32(&i)) { s.has_hysteresis = true; s.hysteresis_tenths = i; } break;
    default: break; // preserve unknown and time-program DPIDs for later investigation
  }
}

bool ThermostatModel::consume(const Frame &frame) {
  // 0x05: report, except two-byte 00-DPID acknowledgements.
  if (frame.version != 0 || frame.command != 0x05 || frame.payload_length < 5)
    return false;
  return parse_datapoints(frame.payload, frame.payload_length, on_dp, this);
}

} // namespace trv16
