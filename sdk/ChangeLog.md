# ChangeLog for XR806 SDK

## xr806_sdk-v1.2.2 (20230110)

  * version 1.2.2 for XR806 SDK

  * WLAN
    - Support automatic power control.
    - Add APIs to config parameters of low power.
    - Fix a configuration issue about group rekey time in softAP mode.
    - Fix a fast connection issus about wlan cold start in WPA3 mode.
    - Optimize function of receiving beacon.
    - Add some APIs for WMM debug.
    - Fix a bug that softAP can not send the raw frame.
    - Fix some bugs about wlan low power.
    - Optimize the peak current.
    - Optimize the performance of receiving frames.
    - Optimize the performance to pass TELEC certification.

  * BLE
    - Add an API to get local random address.
    - Enable BLE power manager.
    - Optimize configurations of RF.
    - Optimize BLE de-init flow.
    - Add an API to config the debug level.

  * Network
    - Add an API to get mac address by IP address.
    - Add the xrlink module to support mutil net stack.

  * Driver
    - Enable flash arbitration to optimize erasing and writing flash.
    - Fix the SPI de-init bug to avoid reboot fail.

  * Misc
    - Tools(phoenixMC): Add mass production mode.
    - Tools(phoenixMC): Optimize the procedure to make synchronization faster.
    - Tools(phoenixMC): Fix the verification failed issue when settings_win.ini is missing.
    - Tools(phoenixMC): Optimize the debug log.
    - Add the xrlink example.

## xr806_sdk-v1.2.1 (20220310)

  * version 1.2.1 for XR806 SDK

  * WLAN
    - Support more low power features.
    - Support sending and receiving raw packets.
    - Add an API to config timeout of scan.
    - Fix a bug that station can not connect to AP after WPS connection.

  * BLE
    - Fix connection issues about coexistence with wlan.
    - Add an API to config the transmitting power.

  * System
    - Use internal LDO to supply power by default.
    - Reset the Nor Flash pinmux to fix the reboot failed issue.
    - Fix some pm bugs.

  * Misc
    - Add the vfs example.

## xr806_sdk-v1.2.0 (20210607)

  * version 1.2.0 for XR806 SDK

  * WLAN
    - Support WPA3.

  * BLE
    - Fix connect issue.
    - Fix low probability packet loss in coexistence.

  * Misc
    - Optimize code style

## xr806_sdk-v1.1.0 (20210420)

  * version 1.1.0 for XR806 SDK

  * WLAN
    - Support ps policy config.
    - Fix some bugs

  * BLE
    - Update BLE host to v2.5.0.
    - Support low power - working in standby mode.
    - Support mesh provisioner .
    - Use spiffs for a better performance as default.
    - Optimize BLE setup timing for a better performance.
    - Passed BQB 5.0 tests.
    - Support 32768 crystal.

  * System
    - Optimize power in idle status
    - Fix RTC inaccurate issue during hibernation.

## xr806_sdk-v1.0.0 (20210226)

  * version 1.0.0 for XR806 SDK

  * WLAN
    - Optimizes RF performance.
    - Fix some bugs

  * BLE
    - Support BLE deinit.
    - Fix compatibility bug.
    - Add BLE adapter.

  * Network
    - iperf: fix exception bug.

  * FS
    - Support vfs.
    - Support littleFS and spifFS.

  * System
    - Fix some bugs.
    - Support temperature monitor.

  * Driver
    - Flash: support flash write protection.

  * PM
    - Adjust PM wakelocks interface.
    - Add some apis.

  * Misc
    - Tools: Add mklittlefs tools and mkspiffs tools.
    - Tools(ETF tool): Add BLE test
    - Tools(phoenixMC): Add boot image sequence selection in combined image.
    - Tools(eFuse tool): Support XR806.

## xr806_sdk-v0.3.0 (20210108)

  * version 0.3.0 for XR806 SDK

  * WLAN
    - fix TxSpur problem in temperature measurement.
    - TX Power Optimize 2mA by Rx_abb_ldo enters Lp mode.
    - Optimizes the DTM power consumption.
    - Beacon window adaptation.
    - Fix AP_Sniffer BLE disconnection under BLE co-existence.
    - Remove lmac drv epat debug function to save bss.
    - Lwip support getsockack and getsockseq.

  * BLE
    - Fix exception bug.
    - Fix memory leak.
    - Optimize RX performance.

  * System
    - Use ChipID to adjust Chip Voltage.
    - Change SDD to public module.
    - Fix startup uncompress image in compress OTA area.
    - Change Flash default config to 96M 4Line from 48M 2Line.
    - Support Watchpoint.

  * PM
    - Add enter suspend again when wakedup by WLAN Calibrarion.


## xr806_sdk-v0.2.0 (20201116)

  * Beta version for XR806 SDK

  * WLAN
    - Fix some bugs.
    - Support coexist with BLE.

  * BLE
    - Support GAP
      - Roles: Broadcaster/Observer/Peripheral/Central
      - Security Mode 1 & 2
      - Privacy
    - Support SM
      - Master Role & Slave Role
      - Just Works & Passkey Entry
    - Support GATT
      - GATT Client
      - GATT Server
    - Support Mesh
      - Node
        - PB-ADV, PB-GATT
        - Relay, Proxy Server, Friend, LPN
        - Configuration Server Model, Health Server Model, Configuration Client Model, Health Client Model
        - Generic Onoff Server Model & Generic Onoff Client Model

  * Network
    - lwip: Support lwip-2.1.2.

  * Driver
    - EFUSE: Adjust some EFUSE item.

  * System
    - Power: Adjust some power configuration.
    - 32K: Adjust 32K RCCAL calibration interval.

  * Misc
    - Sdd: Adjust DCXO frequency offset for evaluation board.


## xr806_sdk-v0.1.0 (20201016)

  * Alpha version for XR806 SDK
