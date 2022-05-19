#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_WPA_SUPP)

zephyr_library()

set(WPA_SUPPLICANT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../wpa_supplicant)
set(HOSTAP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/../)

set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs -lnosys")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMISSING_SYSCALL_NAMES")

zephyr_compile_definitions(
	CONFIG_ZEPHYR
)

zephyr_include_directories(
	${WPA_SUPPLICANT_SRC}/
	${HOSTAP_BASE}/src/
)

zephyr_library_compile_definitions(
	TLS_DEFAULT_CIPHERS=\""DEFAULT:!EXP:!LOW"\"
	CONFIG_CRYPTO_INTERNAL
	CONFIG_SHA256
	CONFIG_SME
	CONFIG_NO_CONFIG_WRITE
	CONFIG_NO_CONFIG_BLOBS
	CONFIG_WPA_S_ZEPHYR_L2_WIFI_MGMT
	CONFIG_NO_STDOUT_DEBUG
	)

zephyr_library_include_directories(
    	${HOSTAP_BASE}/
	${WPA_SUPPLICANT_SRC}/
	${HOSTAP_BASE}/src/utils
	${HOSTAP_BASE}/src/drivers
	${HOSTAP_BASE}/src
	${GNUARMEMB_TOOLCHAIN_PATH}/opt/arm-none-eabi/include
	${ZEPHYR_BASE}/include
	${ZEPHYR_BASE}/include/net
)

zephyr_library_sources(
	${HOSTAP_BASE}/src/common/wpa_common.c
	${HOSTAP_BASE}/src/common/ieee802_11_common.c
	${HOSTAP_BASE}/src/common/hw_features_common.c
	#${HOSTAP_BASE}/src/common/wpa_ctrl.c
	#${HOSTAP_BASE}/src/common/cli.c

	${HOSTAP_BASE}/src/drivers/driver_common.c
	${HOSTAP_BASE}/src/drivers/drivers.c
	${HOSTAP_BASE}/src/l2_packet/l2_packet_zephyr.c
	${HOSTAP_BASE}/src/drivers/driver_zephyr.c
	${HOSTAP_BASE}/src/utils/base64.c
	${HOSTAP_BASE}/src/utils/common.c
	${HOSTAP_BASE}/src/utils/wpa_debug.c
	${HOSTAP_BASE}/src/utils/wpabuf.c
	${HOSTAP_BASE}/src/utils/bitfield.c
	${HOSTAP_BASE}/src/utils/eloop.c
	${HOSTAP_BASE}/src/utils/os_zephyr.c
	${HOSTAP_BASE}/src/utils/radiotap.c
	#${HOSTAP_BASE}/src/utils/edit_simple.c
	${WPA_SUPPLICANT_SRC}/config.c
	${WPA_SUPPLICANT_SRC}/notify.c
	${WPA_SUPPLICANT_SRC}/bss.c
	${WPA_SUPPLICANT_SRC}/eap_register.c
	${WPA_SUPPLICANT_SRC}/op_classes.c
	${WPA_SUPPLICANT_SRC}/rrm.c
	${WPA_SUPPLICANT_SRC}/wmm_ac.c
	${WPA_SUPPLICANT_SRC}/config_none.c
	${WPA_SUPPLICANT_SRC}/sme.c
	${WPA_SUPPLICANT_SRC}/wpa_supplicant.c
	${WPA_SUPPLICANT_SRC}/events.c
	${WPA_SUPPLICANT_SRC}/bssid_ignore.c
	${WPA_SUPPLICANT_SRC}/wpas_glue.c
	${WPA_SUPPLICANT_SRC}/scan.c
	${WPA_SUPPLICANT_SRC}/robust_av.c
	# crypto
	${HOSTAP_BASE}/src/rsn_supp/wpa.c
	${HOSTAP_BASE}/src/rsn_supp/preauth.c
	${HOSTAP_BASE}/src/rsn_supp/pmksa_cache.c
	${HOSTAP_BASE}/src/rsn_supp/wpa_ie.c

	${HOSTAP_BASE}/src/crypto/crypto_internal.c
	${HOSTAP_BASE}/src/crypto/tls_none.c
	${HOSTAP_BASE}/src/crypto/aes-internal.c
	${HOSTAP_BASE}/src/crypto/aes-internal-dec.c
	${HOSTAP_BASE}/src/crypto/aes-unwrap.c
	${HOSTAP_BASE}/src/crypto/aes-internal-enc.c
	${HOSTAP_BASE}/src/crypto/aes-omac1.c
	${HOSTAP_BASE}/src/crypto/md5.c
	${HOSTAP_BASE}/src/crypto/md5-internal.c
	${HOSTAP_BASE}/src/crypto/rc4.c
	${HOSTAP_BASE}/src/crypto/random.c
	${HOSTAP_BASE}/src/crypto/sha1.c
	${HOSTAP_BASE}/src/crypto/sha1-prf.c
	${HOSTAP_BASE}/src/crypto/sha1-internal.c
	${HOSTAP_BASE}/src/crypto/sha1-pbkdf2.c
	${HOSTAP_BASE}/src/crypto/sha256.c
	${HOSTAP_BASE}/src/crypto/sha256-prf.c
	${HOSTAP_BASE}/src/crypto/sha256-internal.c
	# Zephyr main
	src/supp_main.c
	src/supp_mgmt.c
	src/supp_shell.c
)
endif()
