#include <stdint.h>
#include "trv16/protocol.hpp"

extern "C" int trv16_uart_init(void);
extern "C" int trv16_runtime_poll_once(void);
extern "C" int trv16_uart_receive(
    uint8_t *buffer,
    int32_t length,
    uint32_t timeout_ms
);

static volatile bool uart_test_enabled = false;

static void on_frame(const trv16::Frame &frame, void *ctx) {
    trv16::ThermostatModel *model =
        static_cast<trv16::ThermostatModel *>(ctx);
    model->consume(frame);
}

extern "C" int trv16_cpp_test(void) {
    uint8_t refresh[16];

    if (trv16::encode_refresh_request(
            refresh, sizeof(refresh)) != 12) {
        return -1;
    }

    // Echter HC32-Frame: Solltemperatur 24,0 C
    const uint8_t sample[] = {
        0x55, 0xAA, 0x00, 0x05, 0x00, 0x08,
        0x03, 0x02, 0x00, 0x04,
        0x00, 0x00, 0x00, 0xF0, 0x05
    };

    trv16::FrameParser parser;
    trv16::ThermostatModel model;

    parser.feed(sample, sizeof(sample), on_frame, &model);

    if (parser.good_frames() != 1 ||
        !model.state().has_target ||
        model.state().target_tenths != 240) {
        return -2;
    }

    // UART bleibt deaktiviert
    if (uart_test_enabled) {
        if (trv16_uart_init() != 0) return -3;

        return trv16_runtime_poll_once();
    }

    return 0;
}
