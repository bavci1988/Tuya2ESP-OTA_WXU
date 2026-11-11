# XR806 platform host tests

Host tests for the pure-logic headers of the ESPHome XR806 platform
(`esphome/components/xr806/*.h` and `ota/ota_backend_xr806` helpers in
`bavci1988/esphome-xr806`). Upstream's C++ test runner does not build a
target platform's headers on the host, so these run in this repository's
firmware CI instead (`.github/workflows/xr806-dev-integration.yml`).

Run locally from the parent directory of both checkouts:

    for t in xr806-sdk/ci/esphome-tests/*_test.cpp; do
      g++ -std=gnu++20 -pthread -Wall -Wextra -Werror -Iesphome-xr806 "$t" -o /tmp/t && /tmp/t
    done
