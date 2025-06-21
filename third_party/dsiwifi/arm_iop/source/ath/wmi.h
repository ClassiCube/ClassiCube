/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _WMI_H
#define _WMI_H

#include "common.h"

#include "lwip/err.h"
#include "lwip/netif.h"

// Commands
#define WMI_CONNECT_CMD                             (0x0001)
#define WMI_DISCONNECT_CMD                          (0x0003)
#define WMI_SYNCHRONIZE_CMD                         (0x0004)
#define WMI_CREATE_PSTREAM_CMD                      (0x0005)
#define WMI_START_SCAN_CMD                          (0x0007)
#define WMI_SET_SCAN_PARAMS_CMD                     (0x0008)
#define WMI_SET_BSS_FILTER_CMD                      (0x0009)
#define WMI_SET_PROBED_SSID_CMD                     (0x000A)
#define WMI_SET_DISCONNECT_TIMEOUT_CMD              (0x000D)
#define WMI_GET_CHANNEL_LIST_CMD                    (0x000E)
#define WMI_GET_STATISTICS_CMD                      (0x0010)
#define WMI_SET_CHANNEL_PARAMS_CMD                  (0x0011)
#define WMI_SET_POWER_MODE_CMD                      (0x0012)
#define WMI_ADD_CIPHER_KEY_CMD                      (0x0016)
#define WMI_SET_TX_PWR_CMD                          (0x001B)
#define WMI_DELETE_BAD_AP_CMD                       (0x001F)
#define WMI_TARGET_ERROR_REPORT_BITMASK_CMD         (0x0022)
#define WMI_WMIX_CMD                                (0x002E)
#define WMI_SET_FIXRATES_CMD                        (0x0034)
#define WMI_SET_BT_STATUS_CMD                       (0x003B)
#define WMI_SET_KEEPALIVE_CMD                       (0x003D)
#define WMI_SET_WSC_STATUS_CMD                      (0x0041)
#define WMI_SET_HEARTBEAT_TIMEOUT_CMD               (0x0047)
#define WMI_SET_FRAMERATES_CMD                      (0x0048)

// WMIX Commands

#define WMIX_DBGLOG_CFG_MODULE_CMD      (0x2009)
#define WMIX_DBGLOG_EVENT               (0x3008)

// Nintendo specific?
#define WMI_SET_BITRATE_CMD             (0xF000)

// Reponses
#define WMI_GET_CHANNEL_LIST_RESP       (0x000E)

#define WMI_READY_EVENT                 (0x1001)
#define WMI_CONNECT_EVENT               (0x1002)
#define WMI_DISCONNECT_EVENT            (0x1003)
#define WMI_BSS_INFO_EVENT              (0x1004)
#define WMI_CMD_ERROR_EVENT             (0x1005)
#define WMI_REG_DOMAIN_EVENT            (0x1006)
#define WMI_PSTREAM_TIMEOUT_EVENT       (0x1007)
#define WMI_NEIGHBOR_REPORT_EVENT       (0x1008)
#define WMI_TKIP_MICERR_EVENT           (0x1009)
#define WMI_SCAN_COMPLETE_EVENT         (0x100A)
#define WMI_REPORT_STATISTICS_EVENT     (0x100B)
#define WMI_RSSI_THRESHOLD_EVENT        (0x100C)
#define WMI_TARGET_ERROR_REPORT_EVENT   (0x100D)
#define WMI_OPT_RX_FRAME_EVENT          (0x100E)
#define WMI_REPORT_ROAM_TBL_EVENT       (0x100F)
#define WMI_EXTENSION_EVENT             (0x1010)
#define WMI_ACL_DATA_EVENT              (0x1025)

typedef struct gtk_keyinfo gtk_keyinfo;

void wmi_handle_pkt(u16 pkt_cmd, u8* pkt_data, u32 len, u32 ack_len);
void wmi_send_pkt(u16 wmi_type, u8 ack_type, const void* data, u16 len);

void wmi_scan();
void wmi_dbgoff();
void wmi_add_cipher_key(u8 idx, u8 usage, const u8* key, const u8* rsc);
void wmi_tick(void);

void wmi_post_handshake(const u8* tk, const gtk_keyinfo* gtk_info, const u8* rsc);

void wmi_disconnect_cmd();

void wmi_tick_display();

bool wmi_is_ready();

void data_send_wpa_handshake2(const u8* dst_bssid, const u8* src_bssid, u64 replay_cnt);
void data_send_wpa_handshake4(const u8* dst_bssid, const u8* src_bssid, u64 replay_cnt);
void data_send_ip(const u8* dst_bssid, const u8* src_bssid, void* ip_data, u32 ip_data_len);
void data_send_link(void* ip_data, u32 ip_data_len);
void data_handle_auth(u8* pkt_data, u32 len, const u8* dev_bssid, const u8* ap_bssid);
void data_send_to_lwip(void* data, u32 len);
bool wmi_handshake_done();

err_t ath_init_fn(struct netif *netif);
err_t ath_link_output(struct netif *netif, struct pbuf *p);

u8* wmi_get_mac();
u8* wmi_get_ap_mac();

#endif // _WMI_H
