/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wifi_host.h"

#include <nds.h>

#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/lwip_init.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/etharp.h"
#include "lwip/netif/ethernet.h"
#include "lwip/sys.h"
#include "lwip/netbiosns.h"
#include "lwip/tcpip.h"

#include "dsiwifi9.h"
#include "dsiwifi_cmds.h"

static u8 device_mac[6];
static u8 ap_mac[6];

static bool host_bInitted = false;
static bool host_bLwipInitted = false;
static bool host_bNeedsDHCPRenew = false;

static int ip_data_buf_idx = 0;

#define DATA_BUF_RINGS (6)
#define DATA_IN_BUF_RINGS (6)
#define DATA_BUF_LEN (0x600)

#define IF_MTU_SIZE (0x570)

static u8 __attribute((aligned(16))) ip_data_buf[DATA_BUF_LEN*DATA_BUF_RINGS];
static u8 __attribute((aligned(16))) ip_data_in_buf[DATA_BUF_LEN*DATA_IN_BUF_RINGS];

// LWIP state
static struct netif ath_netif = {0};
static ip_addr_t ath_myip_addr = {0};
static ip_addr_t gw_addr = {0}, netmask = {0};

void ath_lwip_tick();
err_t ath_link_output(struct netif *netif, struct pbuf *p);

static int idk_hack = 0;

void data_init_bufs()
{
    for (int i = 0; i < DATA_BUF_RINGS; i++)
    {
        void* dst = memUncached(ip_data_buf + (DATA_BUF_LEN * i));
        *(vu32*)dst = 0xF00FF00F;
    }
    
    for (int i = 0; i < DATA_IN_BUF_RINGS; i++)
    {
        void* dst = memUncached(ip_data_in_buf + (DATA_BUF_LEN * i));
        *(vu32*)dst = 0xF00FF00F;
    }
    
    
}

void* data_next_buf()
{
#if 0
    //for (int j = 0; j < 1000000; j++)
    //while (1)
    for (int j = 0; j < 100; j++)
    {
        for (int i = 0; i < DATA_BUF_RINGS; i++)
        {
            void* dst = memUncached(ip_data_buf + (DATA_BUF_LEN * i));
            if (*(vu32*)dst == 0xF00FF00F)
            {
                //wifi_printf("ret %u\n", i);
                return dst;
            }
        }
        //wifi_printf("arm9 loop...\n");
    }
    //return memUncached(ip_data_buf);
    return NULL;
#endif

#if 1
    void* ret = memUncached(ip_data_buf + (DATA_BUF_LEN * ip_data_buf_idx));

    //wifi_printf("ret %u\n", ip_data_buf_idx);

    ip_data_buf_idx = (ip_data_buf_idx + 1) % DATA_BUF_RINGS;
    
    //memset(ret, 0, DATA_BUF_LEN);
    
    return ret;
#endif
}

static void wifi_print_mac(const char* prefix, const u8* mac)
{
    if (prefix) {
        wifi_printlnf("%s %02x:%02x:%02x:%02x:%02x:%02x", prefix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else {
        wifi_printlnf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

//
// LwIP
//

err_t data_send_link(void* ip_data, u32 ip_data_len)
{
    if (!ip_data_len) return ERR_OK;
    if (ip_data_len > DATA_BUF_LEN)
        ip_data_len = DATA_BUF_LEN;

    void* dst = data_next_buf();
    if (!dst)
        return ERR_MEM;

    memcpy(dst, ip_data, ip_data_len);
    
    //wifi_printf("send pkt size %d\n", ip_data_len - 0x40);
    
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCCMD_SENDPKT;
    msg.pkt_data = dst;
    msg.pkt_len = ip_data_len;
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
    
    /*for(int i = 0; i < 0x10000; i++)
    {
        asm("nop");
    }*/
    
    //while (*(vu32*)dst != 0xF00FF00F);
    return ERR_OK;
}

err_t ath_init_fn(struct netif *netif)
{
    ath_netif.output = etharp_output;
    ath_netif.linkoutput = ath_link_output;
    ath_netif.mtu = IF_MTU_SIZE;
        
    ath_netif.name[0] = 'w';
    ath_netif.name[1] = 'l';
    ath_netif.hwaddr_len = 6;
    ath_netif.hostname = "WhoNeedTheyPzy8";
    memcpy(ath_netif.hwaddr, device_mac, 6);
    ath_netif.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6 | NETIF_FLAG_LINK_UP;
    
    return ERR_OK;
}

err_t ath_link_output(struct netif *netif, struct pbuf *p)
{
    // TODO pbuf_coalesce?
    
    //wifi_printlnf("link output %x", p->len);
    
    //hexdump(p->payload,  0x20);
    
    return data_send_link(p->payload, p->len);
}

void ath_lwip_tick()
{
    sys_check_timeouts();
    tcpip_thread_poll_one();
}

void data_send_to_lwip(void* data, u32 len)
{
    if (len > DATA_BUF_LEN-6)
        len = DATA_BUF_LEN-6;

    struct pbuf *p = pbuf_alloc(PBUF_IP, len, PBUF_POOL);

    // TODO error check?
    
    if (!p) {
        *(vu32*)(data - 6) = 0xF00FF00F;
        return;
    }
    
    if (len > p->len)
        len = p->len;
    
    memcpy(p->payload, data, len);
    *(vu32*)(data - 6) = 0xF00FF00F;
    
    //wifi_printlnf("link in 0x%x bytes", len);
    
    if (ath_netif.input(p, &ath_netif) != ERR_OK) {
        pbuf_free(p);
    }
    
    // ~Hopefully get a fast ACK by doing this
    //int lock = enterCriticalSection();
    //ath_lwip_tick();
    //leaveCriticalSection(lock);
    
    //wifi_printlnf("link in done");
    
    //ath_lwip_tick();
}

//
// FIFO
//

static void wifi_host_get_mac()
{
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCCMD_GET_DEVICE_MAC;
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}

static void wifi_host_get_ap_mac()
{
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCCMD_GET_AP_MAC;
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}

static void wifi_host_init_bufs(void)
{
    data_init_bufs();
    
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCCMD_INITBUFS;
    msg.pkt_data = memUncached(ip_data_in_buf);
    msg.pkt_len = sizeof(ip_data_in_buf);
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}

static void wifi_host_init_iop(void)
{
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCCMD_INIT_IOP;
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}


static void wifi_host_lwip_init()
{
    
    // If this is just the AP renewing keys, don't redo lwip init.
    if (host_bLwipInitted) {
        if (host_bNeedsDHCPRenew)
        {
            if (DSiWifi_pfnReconnectHandler)
                DSiWifi_pfnReconnectHandler();

            dhcp_start(&ath_netif);
            host_bNeedsDHCPRenew = false;
        }
        return;
    }
    
    // Initialize LwIP

    ath_myip_addr.addr = IPADDR_NONE;
    gw_addr.addr = IPADDR_NONE;
    netmask.addr = IPADDR_NONE;
    
    lwip_init();
    
    if (netif_add(&ath_netif, &ath_myip_addr, &netmask, &gw_addr, NULL /* priv state */,
                ath_init_fn, ethernet_input) == NULL) {
        wifi_printlnf("wifi_host_lwip_init: netif_add failed");
        return;
    }

    netif_set_default(&ath_netif);
    netif_set_up(&ath_netif);
    netif_set_link_up(&ath_netif);
    
    netbiosns_set_name(ath_netif.hostname);
    netbiosns_init();
    
    dhcp_start(&ath_netif);
    
    if (DSiWifi_pfnConnectHandler)
        DSiWifi_pfnConnectHandler();
    
    host_bLwipInitted = true;
    host_bNeedsDHCPRenew = false;
}

static void wifi_host_handleMsg(int len, void* user_data)
{
    Wifi_FifoMsgExt msg;
    
    if (len < 4)
    {
        wifi_printf("Bad msg len %x\n", len);
        return;
    }
    
    fifoGetDatamsg(FIFO_DSWIFI, len, (u8*)&msg);
    
    u32 cmd = msg.cmd;
    if (cmd == WIFI_IPCINT_READY)
    {
        wifi_host_get_mac();
        wifi_host_get_ap_mac();
        wifi_host_init_bufs();
        
        host_bInitted = true;
    }
    else if (cmd == WIFI_IPCINT_CONNECT)
    {
        wifi_printf("Connecting to AP\n");
        
        host_bNeedsDHCPRenew = true;
    }
    else if (cmd == WIFI_IPCINT_DEVICE_MAC)
    {
        memcpy(device_mac, msg.mac_addr, 6);
        wifi_print_mac("Dev", device_mac);
    }
    else if (cmd == WIFI_IPCINT_AP_MAC)
    {
        memcpy(ap_mac, msg.mac_addr, 6);
        wifi_print_mac("AP", ap_mac);
        
        wifi_host_lwip_init();
    }
    else if (cmd == WIFI_IPCINT_PKTDATA)
    {
        void* data = memUncached(msg.pkt_data);
        u32 len = msg.pkt_len;

        data_send_to_lwip(data, len);
    }
    else if (cmd == WIFI_IPCINT_PKTSENT)
    {
        void* data = msg.pkt_data;
        u32 len = msg.pkt_len;
            
        *(vu32*)data = 0xF00FF00F;
    }
    else if (cmd == WIFI_IPCINT_DBGLOG)
    {
        wifi_printf("%s", msg.log_str);
    }
    else
    {
        wifi_printf("val %x\n", cmd);
    }
}

extern void sys_now_inc(u32 amt);

u32 wifi_host_get_ipaddr(void)
{
    return ath_netif.ip_addr.addr;
}

int wifi_host_get_status(void)
{
    if (host_bLwipInitted)
    {
        if (wifi_host_get_ipaddr() != 0xFFFFFFFF)
            return ASSOCSTATUS_ASSOCIATED;

        return ASSOCSTATUS_ACQUIRINGDHCP;
    }
    
    return ASSOCSTATUS_SEARCHING;
}

void wifi_host_tick_display()
{
    static int counter = 100000;
    
    if (counter++ > 2*1000)
    {
        u32 addr = ath_netif.ip_addr.addr;
        u8 addr_bytes[4];
        
        memcpy(addr_bytes, &addr, 4);
        
        //wifi_printf("\x1b[s\x1b[0;0HCur IP: \x1b[36m%u.%u.%u.%u        \x1b[u\x1b[37m", addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3]);
        //font_draw_string(0,0, WHITE, tmp);
        
        //data_send_ip(broadcast_all, device_mac, NULL, 0);
        counter = 0;
    }
}

void wifi_host_tick()
{
    static int inc_cnt = 0;
    
    if (inc_cnt++ == 1) {
        sys_now_inc(1);
        inc_cnt = 0;
    }

    // For debugging weird hangs
    static int sec_cnt = 0;
    //static int sec_num = 0;
    if (sec_cnt++ == 1000) {
        //wifi_printf("tick %u\n", sec_num++);
        sec_cnt = 0;
        
        // Sometimes DHCP can have some trouble enumerating.
        // Since we *really* need an IP address, just keep prodding every so often
        // if we don't get one quickly.
        if (host_bLwipInitted && ath_netif.ip_addr.addr == 0xFFFFFFFF || ath_netif.ip_addr.addr == 0x0)
        {
            if (DSiWifi_pfnReconnectHandler)
                DSiWifi_pfnReconnectHandler();

            dhcp_start(&ath_netif);
            host_bNeedsDHCPRenew = false;
        }
    }
    
    if (host_bLwipInitted)
    {
        ath_lwip_tick();
        wifi_host_tick_display();
    }
}

void wifi_host_init()
{
    wifi_printf("wifi_host_init\n");
    
    // 100ms timer
    timerStart(3, ClockDivider_1024, TIMER_FREQ_1024(1000), wifi_host_tick);
    
    // Enable FIFO handlers
	fifoSetDatamsgHandler(FIFO_DSWIFI, wifi_host_handleMsg, 0);
	
	wifi_host_init_iop();
	wifi_printf("did init iop\n");
}
