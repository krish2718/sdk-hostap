/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SUPP_MGMT_H__
#define __SUPP_MGMT_H__

/* Management part definitions */
#include <net/net_mgmt.h>
#include <net/wifi.h>

#define _NET_SUPP_LAYER NET_MGMT_LAYER_L2
/* TODO: See if this needs to be assigned a different code */
#define _NET_SUPP_CODE 0x157
#define _NET_SUPP_BASE                                                         \
	(NET_MGMT_IFACE_BIT | NET_MGMT_LAYER(_NET_SUPP_LAYER) |                \
	 NET_MGMT_LAYER_CODE(_NET_SUPP_CODE))
#define _NET_SUPP_EVENT (_NET_SUPP_BASE | NET_MGMT_EVENT_BIT)

/* Request a Wi-Fi scan */
#define NET_REQUEST_SUPP_SCAN (_NET_SUPP_BASE | NET_REQUEST_SUPP_CMD_SCAN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_SUPP_SCAN);

/* Request a connection to a specified SSID */
#define NET_REQUEST_SUPP_CONNECT (_NET_SUPP_BASE | NET_REQUEST_SUPP_CMD_CONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_SUPP_CONNECT);

#ifdef notyet
#define NET_REQUEST_SUPP_DISCONNECT                                            \
	(_NET_SUPP_BASE | NET_REQUEST_SUPP_CMD_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_SUPP_DISCONNECT);
#endif /* notyet */

#define NET_REQUEST_SUPP_AP_ENABLE                                             \
	(_NET_SUPP_BASE | NET_REQUEST_SUPP_CMD_AP_ENABLE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_SUPP_AP_ENABLE);

#define NET_REQUEST_SUPP_AP_DISABLE                                            \
	(_NET_SUPP_BASE | NET_REQUEST_SUPP_CMD_AP_DISABLE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_SUPP_AP_DISABLE);


/* Scan result event */
#define NET_EVENT_SUPP_SCAN_RESULT                                             \
	(_NET_SUPP_EVENT | NET_EVENT_SUPP_CMD_SCAN_RESULT)

/* Event to indicate that a scan is completed */
#define NET_EVENT_SUPP_SCAN_DONE                                               \
	(_NET_SUPP_EVENT | NET_EVENT_SUPP_CMD_SCAN_DONE)

/* Event to indicate the status of a connection request */
#define NET_EVENT_SUPP_CONNECT_RESULT                                          \
	(_NET_SUPP_EVENT | NET_EVENT_SUPP_CMD_CONNECT_RESULT)

#ifdef notyet
#define NET_EVENT_SUPP_DISCONNECT_RESULT                                       \
	(_NET_SUPP_EVENT | NET_EVENT_SUPP_CMD_DISCONNECT_RESULT)
#endif /* notyet */

/**
 * enum net_request_supp_cmd - Network management API commands for WPA supplicant.
 * @NET_REQUEST_SUPP_CMD_SCAN: Initiate a Wi-Fi scan.
 * @NET_REQUEST_SUPP_CMD_CONNECT: Initiate a Wi-Fi connection to a specified SSID.
 *
 * This enum lists the possible WLAN access categories.
 */
enum net_request_supp_cmd {
	NET_REQUEST_SUPP_CMD_SCAN = 1,
	NET_REQUEST_SUPP_CMD_CONNECT,
#ifdef notyet
	NET_REQUEST_SUPP_CMD_DISCONNECT,
#endif /* notyet */
	NET_REQUEST_SUPP_CMD_AP_ENABLE,
	NET_REQUEST_SUPP_CMD_AP_DISABLE,
};

/**
 * enum net_event_supp_cmd - Network management API events from WPA supplicant.
 * @NET_EVENT_SUPP_CMD_SCAN_RESULT: Scan result event.
 * @NET_EVENT_SUPP_CMD_SCAN_DONE: Event to indicate scan complete.
 * @NET_EVENT_SUPP_CMD_CONNECT_RESULT: Event to indicate status of a connect request.
 *
 * This enum lists the possible WLAN access categories.
 */
enum net_event_supp_cmd {
	NET_EVENT_SUPP_CMD_SCAN_RESULT = 1,
	NET_EVENT_SUPP_CMD_SCAN_DONE,
	NET_EVENT_SUPP_CMD_CONNECT_RESULT,
#ifdef notyet
	NET_EVENT_SUPP_CMD_DISCONNECT_RESULT,
#endif /* notyet */
};

#endif /* __SUPP_MGMT_H__ */
