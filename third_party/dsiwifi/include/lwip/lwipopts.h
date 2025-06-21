#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// TODO TCP_MSS 1352?

#define NO_SYS                      0
#define MEM_LIBC_MALLOC             1
#define MEMP_MEM_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (4 * 1024 * 1024)
#define MEMP_NUM_PBUF               1024
#define MEMP_NUM_UDP_PCB            20
#define MEMP_NUM_TCP_PCB            20
#define MEMP_NUM_TCP_PCB_LISTEN     16
#define MEMP_NUM_TCP_SEG            64
#define MEMP_NUM_REASSDATA          32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              512
#define LWIP_ARP                    1
#define IP_REASS_MAX_PBUFS          64
#define IP_FRAG_USES_STATIC_BUF     0
#define IP_DEFAULT_TTL              255
#define IP_SOF_BROADCAST            1
#define IP_SOF_BROADCAST_RECV       1
#define LWIP_ICMP                   1
#define LWIP_BROADCAST_PING         1
#define LWIP_MULTICAST_PING         1
#define LWIP_RAW                    1
#define TCP_WND                     (2 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (4 * TCP_MSS)
#define TCP_LISTEN_BACKLOG          1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HWADDRHINT       1
#define LWIP_NETCONN                1
#define LWIP_SOCKET                 1
#define LWIP_STATS_DISPLAY          1
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
#define ETHARP_TRUST_IP_MAC         0
#define ETH_PAD_SIZE                2
#define LWIP_CHKSUM_ALGORITHM       2
#define SO_REUSE 1
#define LWIP_SO_RCVBUF 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_SNDTIMEO 1
#define SO_REUSE_RXTOALL 1
#define LWIP_SO_LINGER 1

#define LWIP_NETBUF_RECVINFO 1
#define LWIP_SINGLE_NETIF 1
#define TCP_OVERSIZE 1
#define MEMP_NUM_NETCONN 32
#define MEMP_NUM_NETBUF 4
#define LWIP_TCPIP_CORE_LOCKING 1
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1
#define LWIP_TCP_SACK_OUT 1
#define LWIP_TCP_MAX_SACK_NUM 4
#define LWIP_NETIF_TX_SINGLE_PBUF 1
#define LWIP_FIONREAD_LINUXMODE 1

#define SYS_LIGHTWEIGHT_PROT        0

#define LWIP_TCP_KEEPALIVE          1

// Keepalive values, compliant with RFC 1122. Don't change this unless you know what you're doing
#define TCP_KEEPIDLE_DEFAULT        10000UL // Default KEEPALIVE timer in milliseconds
#define TCP_KEEPINTVL_DEFAULT       2000UL  // Default Time between KEEPALIVE probes in milliseconds
#define TCP_KEEPCNT_DEFAULT         9U      // Default Counter for KEEPALIVE probes

#define LWIP_ETHERNET 1
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_DHCP 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_DNS 1
#define LWIP_IGMP 1
#define LWIP_ACD 1
#define LWIP_DHCP_DOES_ACD_CHECK 0
#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_PROVIDE_ERRNO 0
#define LWIP_NETCONN_FULLDUPLEX 0
#define TCPIP_THREAD_TEST 1
#define LWIP_ERRNO_STDINCLUDE 1

#define LWIP_DEBUG                  0

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */
