#include <stdint.h>
#include "trv16/protocol.hpp"

extern "C" int trv16_uart_receive(
    uint8_t *buffer,
    int32_t length,
    uint32_t timeout_ms
);

namespace {

trv16::FrameParser parser;
trv16::ThermostatModel thermostat;

void on_frame(const trv16::Frame &frame, void *ctx) {
    (void)ctx;
    thermostat.consume(frame);
}

}

// Einmalig ein Byte empfangen und verarbeiten
extern "C" int trv16_runtime_poll_once(void) {
    uint8_t byte = 0;

    int received = trv16_uart_receive(
        &byte,
        1,
        10
    );

    if (received == 1) {
        parser.feed(byte, on_frame, nullptr);
    }

    return received;
}
