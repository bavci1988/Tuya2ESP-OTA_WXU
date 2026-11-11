deps_config := \
	src/debug/Kconfig \
	src/libc/Kconfig \
	src/trustzone/Kconfig \
	src/pm/Kconfig \
	src/driver/chip/sdmmc/Kconfig \
	src/driver/chip/psram/Kconfig \
	src/driver/chip/Kconfig \
	src/driver/Kconfig \
	src/kernel/Kconfig \
	src/fs/Kconfig \
	src/ble/mesh/Kconfig \
	src/ble/host/Kconfig.gatt \
	src/ble/host/Kconfig.l2cap \
	src/ble/host/Kconfig \
	src/ble/porting/Kconfig \
	src/ble/settings/Kconfig \
	src/ble/logging/Kconfig.formatting \
	src/ble/logging/Kconfig.filtering \
	src/ble/logging/Kconfig \
	src/ble/porting/Kconfig.debug \
	src/ble/net/Kconfig \
	src/ble/common/Kconfig \
	src/ble/Kconfig \
	src/wlan/Kconfig \
	src/net/Kconfig \
	project/Kconfig \
	./Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
