/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Source:
 * hpm_sdk/samples/lwip/lwip_tcpecho_multi_ports/inc/lwipopts_app.h
 */

#ifndef LWIPOPTS_APP_H
#define LWIPOPTS_APP_H

/*
 * To use this feature let the following define uncommented.
 * To disable it and process by CPU comment the checksum.
 */
// #define CHECKSUM_BY_HARDWARE 1

#define CHECKSUM_GEN_IP                  1 // Added for SP-920
#define CHECKSUM_GEN_UDP                 1 // Added for SP-920
#define CHECKSUM_GEN_TCP                 1 // Added for SP-920
#define CHECKSUM_GEN_ICMP                1 // Added for SP-920

#define CHECKSUM_CHECK_IP                1 // Added for SP-920
#define CHECKSUM_CHECK_UDP               1 // Added for SP-920
#define CHECKSUM_CHECK_TCP               1 // Added for SP-920
#define CHECKSUM_CHECK_ICMP              1 // Added for SP-920

// baremetal / no_sys configuration
#define NO_SYS                     1 // Added for SP-920
#define SYS_LIGHTWEIGHT_PROT       0 // Added for SP-920
#define LWIP_SOCKET                0 // Added for SP-920
#define LWIP_NETCONN               0 // Added for SP-920
#define LWIP_TCPIP_CORE_LOCKING    0 // Added for SP-920
#define LWIP_NETIF_LINK_CALLBACK   1 // Added for SP-920
#define LWIP_TIMEVAL_PRIVATE       0 // Added for SP-920
#define LWIP_COMPAT_MUTEX          0 // Added for SP-920

/*
 * Debug Options
 */
#define LWIP_DEBUG                 1
#define LWIP_DBG_MIN_LEVEL         0
#define PPP_DEBUG                  LWIP_DBG_OFF
#define MEM_DEBUG                  LWIP_DBG_OFF
#define MEMP_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                 LWIP_DBG_OFF
#define API_LIB_DEBUG              LWIP_DBG_OFF
#define API_MSG_DEBUG              LWIP_DBG_OFF
#define TCPIP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                LWIP_DBG_OFF
#define SOCKETS_DEBUG              LWIP_DBG_OFF
#define DNS_DEBUG                  LWIP_DBG_OFF
#define AUTOIP_DEBUG               LWIP_DBG_OFF
#define DHCP_DEBUG                 LWIP_DBG_OFF
#define IP_DEBUG                   LWIP_DBG_OFF
#define IP_REASS_DEBUG             LWIP_DBG_OFF
#define ICMP_DEBUG                 LWIP_DBG_OFF
#define IGMP_DEBUG                 LWIP_DBG_OFF
#define UDP_DEBUG                  LWIP_DBG_OFF
#define TCP_DEBUG                  LWIP_DBG_OFF
#define TCP_INPUT_DEBUG            LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG           LWIP_DBG_OFF
#define TCP_RTO_DEBUG              LWIP_DBG_OFF
#define TCP_CWND_DEBUG             LWIP_DBG_OFF
#define TCP_WND_DEBUG              LWIP_DBG_OFF
#define TCP_FR_DEBUG               LWIP_DBG_OFF
#define TCP_QLEN_DEBUG             LWIP_DBG_OFF
#define TCP_RST_DEBUG              LWIP_DBG_OFF
#define ETHARP_DEBUG               LWIP_DBG_OFF

#endif /* LWIPOPTS_APP_H */
