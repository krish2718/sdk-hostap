/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief WPA supplicant shell module
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_supp_shell, LOG_LEVEL_INF);

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <init.h>

#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>
#include "supp_mgmt.h"

#define SUPPLICANT_SHELL_MODULE "wpa_supp"

#define SUPPLICANT_SHELL_MGMT_EVENTS                                           \
	(NET_EVENT_SUPP_SCAN_RESULT | NET_EVENT_SUPP_SCAN_DONE |               \
	 NET_EVENT_SUPP_CONNECT_RESULT)

static struct {
	const struct shell *shell;

	union {
		struct {
			uint8_t connecting : 1;
			uint8_t disconnecting : 1;
			uint8_t _unused : 6;
		};
		uint8_t all;
	};
} context;

static uint32_t scan_result;

static struct net_mgmt_event_callback supplicant_shell_mgmt_cb;

#define print(shell, level, fmt, ...)                                          \
	do {                                                                   \
		if (shell) {                                                   \
			shell_fprintf(shell, level, fmt, ##__VA_ARGS__);       \
		} else {                                                       \
			printk(fmt, ##__VA_ARGS__);                            \
		}                                                              \
	} while (false)

static void handle_supplicant_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;

	scan_result++;

	if (scan_result == 1U) {
		print(context.shell, SHELL_NORMAL,
		      "%-4s | %-32s %-5s | %-4s | %-4s | %-5s\n", "Num", "SSID",
		      "(len)", "Chan", "RSSI", "Sec");
	}

	print(context.shell, SHELL_NORMAL,
	      "%-4d | %-32s %-5u | %-4u | %-4d | %-5s\n", scan_result,
	      entry->ssid, entry->ssid_length, entry->channel, entry->rssi,
	      (entry->security == WIFI_SECURITY_TYPE_PSK ? "WPA/WPA2" :
							   "Open"));
}

static void handle_supplicant_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		print(context.shell, SHELL_WARNING,
		      "Scan request failed (%d)\n", status->status);
	} else {
		print(context.shell, SHELL_NORMAL, "Scan request done\n");
	}

	scan_result = 0U;
}

static void handle_supplicant_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		print(context.shell, SHELL_WARNING,
		      "Connection request failed (%d)\n", status->status);
	} else {
		print(context.shell, SHELL_NORMAL, "Connected\n");
	}

	context.connecting = false;
}

#ifdef notyet
static void
handle_supplicant_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (context.disconnecting) {
		print(context.shell,
		      status->status ? SHELL_WARNING : SHELL_NORMAL,
		      "Disconnection request %s (%d)\n",
		      status->status ? "failed" : "done", status->status);
		context.disconnecting = false;
	} else {
		print(context.shell, SHELL_NORMAL, "Disconnected\n");
	}
}
#endif /* notyet */

static void supplicant_mgmt_event_handler(struct net_mgmt_event_callback *cb,
					  uint32_t mgmt_event,
					  struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_SUPP_SCAN_RESULT:
		handle_supplicant_scan_result(cb);
		break;
	case NET_EVENT_SUPP_SCAN_DONE:
		handle_supplicant_scan_done(cb);
		break;
	case NET_EVENT_SUPP_CONNECT_RESULT:
		handle_supplicant_connect_result(cb);
		break;
#ifdef notyet
	case NET_EVENT_SUPP_DISCONNECT_RESULT:
		handle_supplicant_disconnect_result(cb);
		break;
#endif /* notyet */
	default:
		break;
	}
}

static int __wifi_args_to_params(size_t argc, char *argv[],
				 struct wifi_connect_req_params *params)
{
	char *endptr;
	int idx = 1;

	if (argc < 1) {
		return -EINVAL;
	}

	/* SSID */
	params->ssid = argv[0];
	params->ssid_length = strlen(params->ssid);

	/* Channel (optional) */
	if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
		params->channel = strtol(argv[idx], &endptr, 10);
		if (*endptr != '\0') {
			return -EINVAL;
		}

		if (params->channel == 0U) {
			params->channel = WIFI_CHANNEL_ANY;
		}

		idx++;
	} else {
		params->channel = WIFI_CHANNEL_ANY;
	}

	/* PSK (optional) */
	if (idx < argc) {
		params->psk = argv[idx];
		params->psk_length = strlen(argv[idx]);
		params->security = WIFI_SECURITY_TYPE_PSK;
		idx++;
		if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
				params->security= strtol(argv[idx], &endptr, 10);
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	return 0;
}

static int cmd_supplicant_connect(const struct shell *shell, size_t argc,
				  char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(shell);
		return -ENOEXEC;
	}

	context.connecting = true;
	context.shell = shell;

	if (net_mgmt(NET_REQUEST_SUPP_CONNECT, iface, &cnx_params,
		     sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Connection request failed\n");
		context.connecting = false;

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Connection requested\n");
	}

	return 0;
}

#ifdef notyet
static int cmd_supplicant_disconnect(const struct shell *shell, size_t argc,
				     char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnecting = true;
	context.shell = shell;

	status = net_mgmt(NET_REQUEST_SUPP_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnecting = false;

		if (status == -EALREADY) {
			shell_fprintf(shell, SHELL_INFO,
				      "Already disconnected\n");
		} else {
			shell_fprintf(shell, SHELL_WARNING,
				      "Disconnect request failed\n");
			return -ENOEXEC;
		}
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Disconnect requested\n");
	}

	return 0;
}
#endif /* notyet */

static int cmd_supplicant_scan(const struct shell *shell, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();

	context.shell = shell;

	/* shell_fprintf(shell, SHELL_WARNING, "Scan request failed\n"); */
	/* return 0; */

	if (net_mgmt(NET_REQUEST_SUPP_SCAN, iface, NULL, 0)) {
		shell_fprintf(shell, SHELL_WARNING, "Scan request failed\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Scan requested\n");
	}

	return 0;
}

static int cmd_supplicant_ap_enable(const struct shell *shell, size_t argc,
				    char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(shell);
		return -ENOEXEC;
	}

	context.shell = shell;

	if (net_mgmt(NET_REQUEST_SUPP_AP_ENABLE, iface, &cnx_params,
		     sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(shell, SHELL_WARNING, "AP mode failed\n");
		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "AP mode enabled\n");
	}

	return 0;
}

static int cmd_supplicant_ap_disable(const struct shell *shell, size_t argc,
				     char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_SUPP_AP_DISABLE, iface, NULL, 0)) {
		shell_fprintf(shell, SHELL_WARNING, "AP mode disable failed\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "AP mode disabled\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(supplicant_cmd_ap,
			       SHELL_CMD(enable, NULL,
					 "<SSID> <SSID length> [channel] [PSK]",
					 cmd_supplicant_ap_enable),
			       SHELL_CMD(disable, NULL,
					 "Disable Access Point mode",
					 cmd_supplicant_ap_disable),
			       SHELL_SUBCMD_SET_END);


SHELL_STATIC_SUBCMD_SET_CREATE(
	supplicant_commands,
	SHELL_CMD(scan, NULL, "Scan AP", cmd_supplicant_scan),
	SHELL_CMD(connect, NULL,
		  "\"<SSID>\"\n<channel number (optional), "
		  "0 means all>\n"
		  "<PSK (optional: valid only for secured SSIDs)>",
		  cmd_supplicant_connect),
#ifdef notyet
	SHELL_CMD(disconnect, NULL, "Disconnect from AP",
		  cmd_supplicant_disconnect),
#endif /* notyet */
	SHELL_CMD(ap, &supplicant_cmd_ap, "Access Point mode commands", NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(wpa_supp, &supplicant_commands, "WPA supplicant commands",
		   NULL);

static int supplicant_shell_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	context.shell = NULL;
	context.all = 0U;
	scan_result = 0U;

	net_mgmt_init_event_callback(&supplicant_shell_mgmt_cb,
				     supplicant_mgmt_event_handler,
				     SUPPLICANT_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&supplicant_shell_mgmt_cb);

	return 0;
}

SYS_INIT(supplicant_shell_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
