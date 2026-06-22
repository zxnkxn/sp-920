/*
 * Copyright (c) 2023-2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Source:
 * hpm_sdk/samples/lwip/common/multiple/netconf.c
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "board.h"
#include "netconf.h"

#if defined(LWIP_DHCP) && LWIP_DHCP
#include "lwip/dhcp.h"
#endif

#include "lwip/netifapi.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "common.h"
//#include "netinfo.h"

#if defined(__ENABLE_FREERTOS) && __ENABLE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#elif defined(__ENABLE_RTTHREAD_NANO) && __ENABLE_RTTHREAD_NANO
#include "rtthread.h"
#endif

#if defined(NO_SYS) && !NO_SYS
sys_mbox_t netif_status_mbox[BOARD_ENET_COUNT];
#endif

ip_init_t ip_init[BOARD_ENET_COUNT] = {
    {HPM_STRINGIFY(IP0_CONFIG), HPM_STRINGIFY(NETMASK0_CONFIG), HPM_STRINGIFY(GW0_CONFIG)},
    {HPM_STRINGIFY(IP1_CONFIG), HPM_STRINGIFY(NETMASK1_CONFIG), HPM_STRINGIFY(GW1_CONFIG)}
};

mac_init_t mac_init[BOARD_ENET_COUNT] = {
    {HPM_STRINGIFY(MAC0_CONFIG)},
    {HPM_STRINGIFY(MAC1_CONFIG)}
};

enet_frame_pointer_t frame_pointer[BOARD_ENET_COUNT];

#if defined(LWIP_DHCP) && LWIP_DHCP
/**
* @brief  LwIP_DHCP_Process_Handle
* @param  parameter the parameter of thread enter function
* @retval None
*/
void LwIP_DHCP_task(void *pvParameters)
{
    struct netif *netif = (struct netif *)pvParameters;

   dhcp_start(netif);

    for (;;) {
        enet_update_dhcp_state(netif);
#if defined(__ENABLE_FREERTOS) && __ENABLE_FREERTOS
        vTaskDelay(5);
#elif defined(__ENABLE_RTTHREAD_NANO) && __ENABLE_RTTHREAD_NANO
        rt_thread_mdelay(5);
#endif
    }
}
#endif

static void netif_update_status(struct netif *netif)
{
    if (netif_is_link_up(netif)) {
#if defined(LWIP_DHCP) && !LWIP_DHCP
        netif_user_notification(netif);
#endif
    }
}

//void netif_config(struct netif *netif, uint8_t i)
//{
//    ip_addr_t ipaddr, netmask, gw;

//    ip4addr_aton(ip_init[i].ip_addr, &ipaddr);
//    ip4addr_aton(ip_init[i].netmask, &netmask);
//    ip4addr_aton(ip_init[i].gw, &gw);

//#if defined(NO_SYS) && NO_SYS
//    netif_add(netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);
//    netif_set_up(netif);
//    netif_set_default(netif);
//#else
//    netifapi_netif_add(netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
//    netifapi_netif_set_up(netif);
//    netifapi_netif_set_default(netif);
//#endif
//    netif->state = &frame_pointer[i];
//    netif_set_link_callback(netif, netif_update_status);
//}

void netif_config(struct netif *netif, uint8_t i)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    if (i == 0) {
        IP4_ADDR(&ipaddr, 192, 168, 100, 10);
        IP4_ADDR(&netmask, 255, 255, 255, 0);
        IP4_ADDR(&gw, 192, 168, 100, 1);
    } else {
        IP4_ADDR(&ipaddr, 192, 168, 200, 10);
        IP4_ADDR(&netmask, 255, 255, 255, 0);
        IP4_ADDR(&gw, 192, 168, 200, 1);
    }

#if defined(NO_SYS) && NO_SYS
    netif_add(netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);
    netif_set_up(netif);
    netif_set_default(netif);
#else
    netifapi_netif_add(netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    netifapi_netif_set_up(netif);
    netifapi_netif_set_default(netif);
#endif

    netif->state = &frame_pointer[i];
    netif_set_link_callback(netif, netif_update_status);

    netif_show_ip_info(netif);
}

void netif_show_ip_info(struct netif *netif)
{
printf("\r\n");

printf("================ Network Interface %d ================\r\n", netif->num);

printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
       netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
       netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

printf("IPv4 Address: %s\r\n", ip4addr_ntoa(netif_ip4_addr(netif)));
printf("IPv4 Netmask: %s\r\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
printf("IPv4 Gateway: %s\r\n", ip4addr_ntoa(netif_ip4_gw(netif)));
}

void netif_user_notification(struct netif *netif)
{
   (void)netif;
}

#if defined(NO_SYS) && !NO_SYS
void netif0_update_link_status(void *pvParameters)
{
    struct netif *netif = (struct netif *)pvParameters;
    uint32_t *msg;

    sys_mbox_new(&netif_status_mbox[0], SYS_LWIP_MBOX_SIZE);

    for (;;) {
        sys_arch_mbox_fetch(&netif_status_mbox[0], (void **)&msg, SYS_LWIP_TIMEOUT_FOREVER);
        if (*msg == enet_phy_link_up) {
            netif_set_link_up(netif);
        } else {
            netif_set_link_down(netif);
        }
    }
}

void netif1_update_link_status(void *pvParameters)
{
    struct netif *netif = (struct netif *)pvParameters;
    uint32_t *msg;

    sys_mbox_new(&netif_status_mbox[1], SYS_LWIP_MBOX_SIZE);

    for (;;) {
        sys_arch_mbox_fetch(&netif_status_mbox[1], (void **)&msg, SYS_LWIP_TIMEOUT_FOREVER);
        if (*msg == enet_phy_link_up) {
            netif_set_link_up(netif);
        } else {
            netif_set_link_down(netif);
        }
    }
}
#endif
