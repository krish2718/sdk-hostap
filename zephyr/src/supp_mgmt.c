/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(wpa_supplicant_mgmt, LOG_LEVEL_DBG);

#include "main.h"

#include "includes.h"
#include "common.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "fst/fst.h"
#include "includes.h"
#include "p2p_supplicant.h"
#include "wpa_supplicant_i.h"
#include "wifi_mgmt.h"
#include "supp_mgmt.h"
#include "driver_zephyr.h"

extern struct wpa_supplicant *wpa_s_0;

#define MAX_SSID_LEN 32

static void scan_result_cb(struct net_if *iface, int status,
			   struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		struct wifi_status scan_status = {
			.status = status,
		};

		net_mgmt_event_notify_with_info(NET_EVENT_SUPP_SCAN_DONE, iface,
						&scan_status,
						sizeof(struct wifi_status));
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_SUPP_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
}

static int wifi_supp_scan(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct zep_wpa_supp_dev_ops *nvlsi_dev_ops = dev->api;

	return nvlsi_dev_ops->off_api.disp_scan(dev, scan_result_cb);
}

static int wifi_supp_connect(uint32_t mgmt_request, struct net_if *iface,
			     void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
#ifdef notyet
	const struct device *dev = net_if_get_device(iface);
	const struct img_rpu_dev_ops_zep *nvlsi_dev_ops = dev->api;
#endif /* notyet */
	struct wpa_ssid *ssid = wpa_supplicant_add_network(wpa_s_0);
	bool pmf = true;

	ssid->ssid = os_zalloc(sizeof(u8) * MAX_SSID_LEN);

	memcpy(ssid->ssid, params->ssid, params->ssid_length);
	ssid->ssid_len = params->ssid_length;
	ssid->disabled = 1;
	ssid->key_mgmt = WPA_KEY_MGMT_NONE;

	wpa_s_0->conf->filter_ssids = 1;
	wpa_s_0->conf->ap_scan= 1;

	if (params->psk) {
		// TODO: Extend enum wifi_security_type
		if (params->security == 3) {
			ssid->key_mgmt = WPA_KEY_MGMT_SAE;
			str_clear_free(ssid->sae_password);
			ssid->sae_password = dup_binstr(params->psk, params->psk_length);
			if (ssid->sae_password == NULL) {
				printk("%s:Failed to copy sae_password\n", __func__);
				return -1;
			}
		} else {
			if (params->security == 2)
				ssid->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
			else
				ssid->key_mgmt = WPA_KEY_MGMT_PSK;
			str_clear_free(ssid->passphrase);
			ssid->passphrase = dup_binstr(params->psk, params->psk_length);
			if (ssid->passphrase == NULL) {
				printk("%s:Failed to copy passphrase\n", __func__);
				return -1;
			}
		}

		wpa_config_update_psk(ssid);
	}

	if (pmf)
		ssid->ieee80211w = 1;

	wpa_supplicant_enable_network(wpa_s_0, ssid);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_SUPP_SCAN, wifi_supp_scan);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_SUPP_CONNECT, wifi_supp_connect);
