/**
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * WPA Supplicant / main() function for Zephyr OS
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <zephyr/logging/log.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

#include "zephyr_fmac_main.h"
#include "zephyr_supp_api.h"

int cli_main(int, const char **);
extern struct wpa_global *global;

enum requested_ops {
	CONNECT = 0,
	DISCONNECT
};

#define DEFAULT_CONNECTION_TIMEOUT_S 15

struct wpa_supp_api_ctrl {
	const struct device *dev;
	int requested_op;
	int connection_timeout;
};

struct wpa_supp_api_ctrl wpa_supp_api_ctrl;

static inline struct wpa_supplicant * get_wpa_s_handle(const struct device *dev)
{
	struct wpa_supplicant *wpa_s = wpa_supplicant_get_iface(global, dev->name);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR,
		"%s: Unable to get wpa_s handle for %s\n", __func__, dev->name);
		return NULL;
	}

	return wpa_s;
}

static void supp_shell_connect_status(void *wpa_supp_api_ctrl)
{
	int i = 0, status = 0;
	struct wpa_supplicant *wpa_s;
	struct wpa_supp_api_ctrl *ctrl = ctrl;

	wpa_s = get_wpa_s_handle(ctrl->dev);
	if (!wpa_s) {
		status = 1;
		goto out;
	}

	if (ctrl->requested_op == CONNECT) {
		/* TODO: use a timer for connection timeout*/
		while (wpa_s->wpa_state != WPA_COMPLETED && i++ < ctrl->connection_timeout){
			k_yield();
			k_msleep(1000);
		}

		if (i >= ctrl->connection_timeout){
			if (wpa_s->wpa_state != WPA_COMPLETED){
				zephyr_supp_disconnect(ctrl->dev);
				status = 1;
				goto out;
			}
		}
	}
out:
	if (ctrl->requested_op == CONNECT) {
		wifi_mgmt_raise_connect_result_event(net_if_lookup_by_dev(ctrl->dev), status);
	} else if (ctrl->requested_op == DISCONNECT) {
		/* Disconnect is a synchronous operation i.e., we are already disconnected
		 * we are just using this thread to post net_mgmt event
		 */
		wifi_mgmt_raise_disconnect_result_event(net_if_lookup_by_dev(ctrl->dev), 0);
	}
}

K_THREAD_DEFINE(wpa_supp_api,
				1024,
				supp_shell_connect_status,
                &wpa_supp_api_ctrl,
                NULL,
                NULL,
                0,
                0,
                -1);


int zephyr_supp_scan(const struct device *dev, scan_result_cb_t cb)
{
	const struct wifi_nrf_dev_ops *dev_ops = dev->api;

	return dev_ops->off_api.disp_scan(dev, cb);
}


int zephyr_supp_connect(const struct device *dev,
						struct wifi_connect_req_params *params)
{
	struct wpa_ssid *ssid = NULL;
	bool pmf = true;
	struct wpa_supplicant *wpa_s;

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		return -1;
	}

	ssid = wpa_supplicant_add_network(wpa_s);
	ssid->ssid = os_zalloc(sizeof(u8) * MAX_SSID_LEN);

	memcpy(ssid->ssid, params->ssid, params->ssid_length);
	ssid->ssid_len = params->ssid_length;
	ssid->disabled = 1;
	ssid->key_mgmt = WPA_KEY_MGMT_NONE;

	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	if (params->psk) {
		// TODO: Extend enum wifi_security_type
		if (params->security == 3) {
			ssid->key_mgmt = WPA_KEY_MGMT_SAE;
			str_clear_free(ssid->sae_password);
			ssid->sae_password = dup_binstr(params->psk, params->psk_length);

			if (ssid->sae_password == NULL) {
				wpa_printf(MSG_ERROR, "%s:Failed t copy sae_password\n",
					      __func__);
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
				wpa_printf(MSG_ERROR, "%s:Failed t copy passphrase\n",
				__func__);
				return -1;
			}
		}

		wpa_config_update_psk(ssid);

		if (pmf)
			ssid->ieee80211w = 1;

	}

	wpa_supplicant_enable_network(wpa_s,
				      ssid);

	wpa_supplicant_select_network(wpa_s,
				      ssid);

	wpa_supp_api_ctrl.dev = dev;
	wpa_supp_api_ctrl.requested_op = CONNECT;
	wpa_supp_api_ctrl.connection_timeout = params->timeout <= 0 ?
			DEFAULT_CONNECTION_TIMEOUT_S: params->timeout;
	/* Cleanup previous spawns */
	k_thread_abort(wpa_supp_api);
	k_thread_start(wpa_supp_api);

	return 0;
}

int zephyr_supp_disconnect(const struct device *dev)
{
	struct wpa_supplicant *wpa_s;

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		return -1;
	}
	wpa_supp_api_ctrl.dev = dev;
	wpa_supp_api_ctrl.requested_op = DISCONNECT;
	wpas_request_disconnection(wpa_s);
	/* Cleanup previous spawns */
	k_thread_abort(wpa_supp_api);
	k_thread_start(wpa_supp_api);
	return 0;
}


static inline int wpas_band_to_zephyr(enum wpa_radio_work_band band)
{
	switch(band) {
		case BAND_2_4_GHZ:
			return WIFI_FREQ_BAND_2_4_GHZ;
		case BAND_5_GHZ:
			return WIFI_FREQ_BAND_5_GHZ;
		default:
			return -1; /* Unknown */
	}
}

static inline int wpas_key_mgmt_to_zephyr(int key_mgmt)
{
	switch(key_mgmt) {
		case WPA_KEY_MGMT_NONE:
			return WIFI_SECURITY_TYPE_NONE;
		case WPA_KEY_MGMT_PSK:
			return WIFI_SECURITY_TYPE_PSK;
		case WPA_KEY_MGMT_PSK_SHA256:
			return WIFI_SECURITY_TYPE_PSK_SHA256;
		case WPA_KEY_MGMT_SAE:
			return WIFI_SECURITY_TYPE_SAE;
		default:
			return -1; /* Unknown */
	}
}


struct wifi_iface_status* zephyr_supp_status(const struct device *dev)
{
	struct wifi_iface_status *status;
	struct wpa_supplicant *wpa_s;
	struct wpa_signal_info si;
	int ret;

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		return NULL;
	}

	status = os_zalloc(sizeof(*status));
	if (!status) {
		wpa_printf(MSG_ERROR, "failed to allocate memory for status\n");
		return NULL;
	}

	status->state = wpa_s->wpa_state;

	if (wpa_s->wpa_state >= WPA_ASSOCIATED) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		u8 channel;

		os_memcpy(status->bssid, wpa_s->bssid, WIFI_MAC_ADDR_LEN);
		status->band = wpas_band_to_zephyr(wpas_freq_to_band(wpa_s->assoc_freq));
		status->security = wpas_key_mgmt_to_zephyr(ssid->key_mgmt);
		status->mfp = ssid->ieee80211w; /* Same mapping */
		ieee80211_freq_to_chan(wpa_s->assoc_freq, &channel);
		status->channel = channel;

		if (ssid) {
			u8 *_ssid = ssid->ssid;
			size_t ssid_len = ssid->ssid_len;
			u8 ssid_buf[SSID_MAX_LEN] = {0};

			if (ssid_len == 0) {
				int _res = wpa_drv_get_ssid(wpa_s, ssid_buf);

				if (_res < 0)
					ssid_len = 0;
				else
					ssid_len = _res;
				_ssid = ssid_buf;
			}
			os_memcpy(status->ssid, _ssid, ssid_len);
			status->ssid_len = ssid_len;
			status->iface_mode = ssid->mode;
			/* TODO: Derive this based on association IEs */
			status->link_mode = WIFI_6;
		}
		ret = wpa_drv_signal_poll(wpa_s, &si);
		if (!ret) {
			status->rssi = si.current_signal;
		}
	}

	return status;
}
