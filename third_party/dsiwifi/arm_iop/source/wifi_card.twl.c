/*
 * Copyright (c) 2021 Max Thomas
 * Copyright (c) 2015-2016, Daz Jones
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wifi_card.h"

#include "utils.h"
#include "wifi_debug.h"

#include "wifi_gpio.h"
#include "wifi_ndma.h"
//#include "pdn.h" // TODO
//#include "mcu.h"  // TODO
//#include "irq.h"
//#include "timer.h" // TODO
//#include "spi.h" // TODO
//#include "3ds.h" // TODO

//#include "rpc.h" // TODO?

#if 0
#include "atheros_bins/nwm_ar6014_softap.h"
#include "atheros_bins/ar6014-parta.h"
#include "atheros_bins/ar6014-partb.h"
#include "atheros_bins/ar6014-partc.h"
#include "atheros_bins/ar6014-partd.h"
#endif

#include "ath/wmi.h"
#include "ath/bmi.h"
#include "ath/htc.h"
#include "ath/mbox.h"
#include "ieee/wpa.h"

#include <nds.h>
#include <nds/interrupts.h>
#include <nds/arm7/clock.h>
#include <nds/arm7/serial.h>
#include <malloc.h>

#include "dsiwifi_cmds.h"

wifi_card_ctx wlan_ctx = {0};
int wifi_card_wlan_init();

static u8* mbox_out_buffer;
static u8* mbox_buffer;

static int ip_data_out_buf_idx = 0;
static u8* ip_data_out_buf = NULL;
static u32 ip_data_out_buf_totlen = 0;

#define ID_AR6002 (0x02000271)
#define ID_AR601x (0x02010271)

#define CHIP_ID_AR6002 (0x02000001)
#define CHIP_ID_AR6013 (0x0D000000)
#define CHIP_ID_AR6014 (0x0D000001)

#define AR6002_HOST_INTEREST_ADDRESS (0x00500400)
#define AR601x_HOST_INTEREST_ADDRESS (0x00520000)

#define SDIO_TICK_INTERVAL_MS (1)
#define MBOX_TMPBUF_SIZE (0x600)
#define DATA_BUF_LEN (0x600)

// Uncomment to send all data bytewise, helpful for debugging.
//#define SDIO_NO_BLOCKRW // MelonDS needs this uncommented... hangs on wifi_ndma_wait after write

#define WRITE_FOUT_1  0x72
#define READ_FOUT_1   0x73
#define WRITE_FOUT_2  0x74
#define READ_FOUT_2   0x75

#define NVRAM_ADDR_WIFICFG      (0x1F400)

#define IRQ_WIFI_SDIO_CARDIRQ BIT(11)

// Collected device info during init
static wifi_card_ctx* device_curctx;
static u32 device_chip_id;
static u32 device_manufacturer;
static u32 device_host_interest_addr;
static u32 device_eeprom_addr;
static u32 device_eeprom_version;

static bool wifi_card_bInitted = false;

static u32 __attribute((aligned(16))) wifi_card_alignedbuf_small[4];

nvram_cfg_wep wifi_card_nvram_wep_configs[3];
nvram_cfg wifi_card_nvram_configs[3];

int wifi_printf_allowed = 0;

// CMD52 - IO_RW_DIRECT (read/write single register).
static const wifi_sdio_command cmd52 = {
    .cmd = 52,
    .response_type = wifi_sdio_resp_48bit,
};

// CMD53 - IO_RW_EXTENDED
static const wifi_sdio_command cmd53_read = {
    .cmd = 53,
    .command_type = 0,
    .response_type = wifi_sdio_resp_48bit,

    .data_transfer = true,
    .data_direction = wifi_sdio_data_read,
    .data_length = wifi_sdio_multiple_block,
    .secure = true,
};

static const wifi_sdio_command cmd53_read_single = {
    .cmd = 53,
    .command_type = 0,
    .response_type = wifi_sdio_resp_48bit,

    .data_transfer = true,
    .data_direction = wifi_sdio_data_read,
    .data_length = wifi_sdio_single_block,
    .secure = true,
};

static const wifi_sdio_command cmd53_write = {
    .cmd = 53,
    .command_type = 0,
    .response_type = wifi_sdio_resp_48bit,
    

    .data_transfer = true,
    .data_direction = wifi_sdio_data_write,
    .data_length = wifi_sdio_multiple_block,
    .secure = true,
};

static const wifi_sdio_command cmd53_write_single = {
    .cmd = 53,
    .command_type = 0,
    .response_type = wifi_sdio_resp_48bit,
    

    .data_transfer = true,
    .data_direction = wifi_sdio_data_write,
    .data_length = wifi_sdio_single_block,
    .secure = true,
};


// Device info

const char* wifi_card_get_chip_str()
{
    switch (device_chip_id)
    {
        case CHIP_ID_AR6013:
            return "AR6013";
        case CHIP_ID_AR6014:
            return "AR6014";
        case CHIP_ID_AR6002:
            return "AR6002";
        default:
            return "Unknown";
    }
}

void wifi_card_print_mac(const char* prefix, const u8* mac)
{
    if (prefix) {
        wifi_printlnf("%s %02x:%02x:%02x:%02x:%02x:%02x", prefix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else {
        wifi_printlnf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

// SDIO basics

int wifi_card_write_func_byte(u8 func, u32 addr, u8 val)
{
    // Read register 0x02 (function enable) hibyte until it's ready
    wifi_card_send_command(cmd52, BIT(31) /* write flag */ | (func << 28) | addr << 9 | val);
    if(device_curctx->tmio.status & 4) {
        return -1;
    }
    return 0;
}

int wifi_card_read_func1_32bit(u32 addr, void* buf, u32 len)
{
    if(!device_curctx) return -2;

    //wifi_sdio_stop(device_curctx->tmio.controller);

    device_curctx->tmio.buffer = buf;
    device_curctx->tmio.size = len;
    device_curctx->tmio.break_early = true;

    u32 old_blocksize = device_curctx->tmio.block_size;
    device_curctx->tmio.block_size = sizeof(u32);
    u8 blkcnt = len;
    u8 funcnum = 1;
    wifi_card_send_command_alt(cmd53_read_single, (funcnum << 28) | (1 << 26) | (addr & 0x1FFFF) << 9 | (blkcnt));

    //device_curctx->tmio.break_early = false;

    //wifi_sdio_stop(device_curctx->tmio.controller);

    device_curctx->tmio.block_size = old_blocksize;

    if(device_curctx->tmio.status & 4)
    {
        return -1;
    }

    return 0;
}

int wifi_card_read_func1_block(u32 addr, void* buf, u32 len)
{
    if(!device_curctx) return -2;

    wifi_ndma_wait();
    wifi_sdio_stop(device_curctx->tmio.controller);

    device_curctx->tmio.buffer = buf;
    device_curctx->tmio.size = len;

    u8 blkcnt = len / device_curctx->tmio.block_size;
    u8 funcnum = 1;
    wifi_card_send_command_alt(cmd53_read, (funcnum << 28) | (1 << 27) | (1 << 26) | (addr & 0x1FFFF) << 9 | (blkcnt));

    wifi_sdio_stop(device_curctx->tmio.controller);
    wifi_ndma_wait();

    if(device_curctx->tmio.status & 4)
    {
        return -1;
    }

    return 0;
}

int wifi_card_write_func1_block(u32 addr, void* buf, u32 len)
{
    if(!device_curctx) return -2;
    
    wifi_ndma_wait();

    wifi_sdio_stop(device_curctx->tmio.controller);

    device_curctx->tmio.buffer = buf;
    device_curctx->tmio.size = len;

    u8 blkcnt = len / device_curctx->tmio.block_size;
    u8 funcnum = 1;
    wifi_card_send_command_alt(cmd53_write, BIT(31) /* write flag */ | (funcnum << 28) | (1 << 27) | (1 << 26) | ((addr & 0x1FFFF) << 9) | (blkcnt));

    wifi_ndma_wait();
    wifi_sdio_stop(device_curctx->tmio.controller);

    //wifi_printlnf("asdf");

    if(device_curctx->tmio.status & 4)
    {
        return -1;
    }

    return 0;
}

u8 wifi_card_read_func_byte(u8 func, u32 addr)
{
    // Read register 0x02 (function enable) hibyte until it's ready
    wifi_card_send_command(cmd52, (func << 28) | (addr & 0x1FFFF) << 9);
    if(device_curctx->tmio.status & 4)
    {
        return 0xFF;
    }
    return (device_curctx->tmio.resp[0] & 0xFF);
}

// Func0 boilerplate
int wifi_card_write_func0_u8(u32 addr, u8 val)
{
    return wifi_card_write_func_byte(0, addr, val);
}

u8 wifi_card_read_func0_u8(u32 addr)
{
    return wifi_card_read_func_byte(0, addr);
}

u16 wifi_card_read_func0_u16(u32 addr)
{
    return wifi_card_read_func0_u8(addr) | (wifi_card_read_func0_u8(addr+1) << 8);
}

u32 wifi_card_read_func0_u32(u32 addr)
{
    return wifi_card_read_func0_u16(addr) | (wifi_card_read_func0_u16(addr+2) << 16);
}

u16 wifi_card_write_func0_u16(u32 addr, u16 val)
{
    return wifi_card_write_func0_u8(addr, val & 0xFF) | (wifi_card_write_func0_u8(addr+1, val >> 8) << 8);
}

u32 wifi_card_write_func0_u32(u32 addr, u32 val)
{
    return wifi_card_write_func0_u16(addr, val & 0xFFFF) | (wifi_card_write_func0_u16(addr+2, val >> 16) << 16);
}

// Func1 boilerplate
int wifi_card_write_func1_u8(u32 addr, u8 val)
{
    return wifi_card_write_func_byte(1, addr, val);
}

u8 wifi_card_read_func1_u8(u32 addr)
{
    return wifi_card_read_func_byte(1, addr);
}

u16 wifi_card_read_func1_u16(u32 addr)
{
    return wifi_card_read_func1_u8(addr) | (wifi_card_read_func1_u8(addr+1) << 8);
}

u32 wifi_card_read_func1_u32(u32 addr)
{
    return wifi_card_read_func1_u16(addr) | (wifi_card_read_func1_u16(addr+2) << 16);
}

u32 wifi_card_read_func1_u32_fast(u32 addr)
{
    //return wifi_card_read_func1_u16(addr) | (wifi_card_read_func1_u16(addr+2) << 16);
    wifi_card_read_func1_32bit(addr, wifi_card_alignedbuf_small, sizeof(u32));
    return wifi_card_alignedbuf_small[0];
}

void wifi_card_write_func1_u16(u32 addr, u16 val)
{
    // Write from MSB to LSB
    wifi_card_write_func1_u8(addr+1, val >> 8);
    wifi_card_write_func1_u8(addr, val & 0xFF);
}

void wifi_card_write_func1_u32(u32 addr, u32 val)
{
    // Write from MSB to LSB
    wifi_card_write_func1_u16(addr+2, val >> 16);
    wifi_card_write_func1_u16(addr, val & 0xFFFF);
}

// Internal RAM
u32 wifi_card_read_intern_word(u32 addr)
{
    wifi_card_write_func1_u32(0x0047C, addr); // WINDOW_READ_ADDR;
    u32 ret = wifi_card_read_func1_u32(0x00474); // WINDOW_DATA
    return ret;
}

u32 wifi_card_read_intern_word_fast(u32 addr)
{
    wifi_card_write_func1_u32(0x0047C, addr); // WINDOW_READ_ADDR;
    u32 ret = wifi_card_read_func1_u32_fast(0x00474); // WINDOW_DATA
    return ret;
}

void wifi_card_write_intern_word(u32 addr, u32 data)
{
    wifi_card_write_func1_u32(0x00474, data); // WINDOW_DATA
    wifi_card_write_func1_u32(0x00478, addr); // WINDOW_WRITE_ADDR;
}

//
// MBOX
//

void wifi_card_mbox_clear(void)
{
    wifi_printf("%x %x %x\n", wifi_card_read_func1_u8(F1_HOST_INT_STATUS), wifi_card_read_func1_u8(F1_MBOX_FRAME), wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID));
    /*wifi_card_write_func1_u8(0x468, 1);
    wifi_card_write_func1_u8(0x469, 1);
    
    ioDelay(0x400000);
    
    wifi_card_write_func1_u8(0x469, 0);*/
    
    // Wait for start of data... At least 4 bytes valid
    /*timeout = 100;
    if (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1))
    {
        wifi_printlnf("b");
    }
    while (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1) && --timeout)
    {
        
    }
    if (!timeout)
    {
        return 0;
    }*/
    
    while (wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 0x1)
    {
        wifi_card_read_func1_u8(0xFF);
        //wifi_printf("%x %x %x - %02x\n", wifi_card_read_func1_u8(F1_HOST_INT_STATUS), wifi_card_read_func1_u8(F1_MBOX_FRAME), wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID), val);
    }
    //wifi_printf("%02x\n", wifi_card_read_func1_u8(0xFF));
    //wifi_printf("%02x\n", wifi_card_read_func1_u8(0xFF));
    //wifi_printf("%02x\n", wifi_card_read_func1_u8(0xFF));
    
    // Read until End of Message bit is gone (shouldn't be needed)
    /*if (wifi_card_read_func1_u8(F1_MBOX_FRAME) & 0x10)
    {
        wifi_printlnf("c");
    }
    while (wifi_card_read_func1_u8(F1_MBOX_FRAME) & 0x10)
    {
        wifi_card_read_func1_u8(0xFF);
    }*/
    
    
    
    // Wait for start of data... At least 4 bytes valid
    /*if (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1))
    {
        wifi_printlnf("d");
    }
    while (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1))
    {
        
    }*/
    
    wifi_printf("leave\n");
}

// Idk what this even is, no$ wifiboot does it before every BMI cmd,
// but I suspect it's a workaround for writing too much to FIFOs w/
// block transfers
void wifi_card_bmi_wait_count4(void)
{
    while (wifi_card_read_func1_u8(F1_COUNT4) == 0)
    {
        ioDelay(0x100);
    }
}

void wifi_card_write_mbox0_u32(u32 val)
{
    for (int i = 0; i < 4; i++)
    {
        //while (1)
        {
            wifi_card_write_func1_u8(0xFF, val & 0xFF);
            val >>= 8;

            /*u8 intval = wifi_card_read_func1_u8(F1_ERROR_INT_STATUS);
            if (!(intval & 0x01)) // tx overflow
            {
                break;
            }

            wifi_card_write_func1_u8(F1_ERROR_INT_STATUS, 0x0);*/
            //break;
        }
    }
}

u32 wifi_card_read_mbox0_u32(void)
{
    while (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1));

    u32 val = 0;
    for (int i = 0; i < 4; i++)
    {
        // It seems the entire mailbox range is the same 1-byte register?
        // Maybe 0xFF sends an interrupt, but we're reading bytewise
        // so just use 0xFF
        val |= (wifi_card_read_func1_u8(0xFF) << i*8);
    }

    return val;
}

u32 wifi_card_read_mbox0_u32_timeout()
{
    u32 timeout = 0;
    while (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1) && ++timeout < 10);

    if (!(wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID) & 1))
        return 0xFFFFFFFF;

    u32 val = 0;
    for (int i = 0; i < 4; i++)
    {
        // It seems the entire mailbox range is the same 1-byte register?
        // Maybe 0xFF sends an interrupt, but we're reading bytewise
        // so just use 0xFF
        val |= (wifi_card_read_func1_u8(0xFF) << i*8);
    }

    return val;
}

// TODO: Move this to block transfers.
// This could get tricky since we'd be unable to failsafe on TX overflows.
// Maybe it would be better to make a DMA330 driver specifically
// for this? Or just ignore overflows/find a good way to manage them.
void wifi_card_mbox0_sendbytes(const u8* data, u32 len)
{
    u16 send_addr = 0x4000 - len;

#ifdef SDIO_NO_BLOCKRW
    for (int i = 0; i < len; i++)
    {
        wifi_card_write_func1_u8(send_addr, data[i]);
        send_addr++;

        if (send_addr >= 0x4000)
            send_addr = 0x3F80;
    }
#else
    wifi_card_write_func1_block(send_addr, (void*)data, len);
#endif
    u32 intval = wifi_card_read_func1_u32(F1_HOST_INT_STATUS);
    if (intval & 0x00010000) // tx overflow
    {
        wifi_printlnf("Mbox Full!: %08lx %02x", wifi_card_read_func1_u32(F1_HOST_INT_STATUS), wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID));
        //break;
    }
    
    //wifi_card_bmi_wait_count4();
}

void wifi_card_mbox0_send_packet(u8 type, u8 ack_type, const u8* data, u16 len, u16 idk)
{
    //memset(mbox_out_buffer, 0x0, round_up(len, 0x80));

    mbox_out_buffer[0] = type;
    mbox_out_buffer[1] = ack_type;
    *(u16*)&mbox_out_buffer[2] = len;
    *(u16*)&mbox_out_buffer[4] = idk;
    
    // Truncate to mbox_out_buffer size
    if (len > (MBOX_TMPBUF_SIZE - 0x6))
        len = (MBOX_TMPBUF_SIZE - 0x6);
    
    if (data)
        memcpy(&mbox_out_buffer[6], data, len);
    
    len = len + 6;
    
    // Round to 0x80, block size
    // TODO can this size be reduced?
    len = round_up(len, 0x80);
    
    //hexdump(mbox_out_buffer, 8);
    
    wifi_card_mbox0_sendbytes(mbox_out_buffer, len); // len
}

void data_handle_pkt(u8* pkt_data, u32 len)
{
    struct __attribute__((packed)) {
        s16 rssi;
        u8 dst_bssid[6]; // 3DS MAC
        u8 src_bssid[6]; // AP MAC
        u8 data_len_be[2];
        u8 body[];
        
    } *data_hdr = (void*)pkt_data;
    
    struct __attribute__((packed)) {
        u8 dsap;
        u8 ssap;
        u8 control;
        u8 org[3];
        u8 type_be[2];
        u8 body[];
        
    } *data_snap = (void*)data_hdr->body;
    
    //u16 data_len = getbe16(data_hdr->data_len_be);
    
    if (getbe16(data_snap->type_be) == 0x888E)
    {
        data_handle_auth(data_snap->body, &pkt_data[len] - data_snap->body, data_hdr->dst_bssid, data_hdr->src_bssid);
    }
    else
    {
        // TODO fragment it?
        if (len > DATA_BUF_LEN - 6)
            len = DATA_BUF_LEN - 6;

        //memcpy(ip_data_out_buf, pkt_data, len);
        
        //wifi_printf("%x %x\n", ip_data_out_buf, pkt_data);
        
        Wifi_FifoMsg msg;
        msg.cmd = WIFI_IPCINT_PKTDATA;
        msg.pkt_data = pkt_data;//ip_data_out_buf;
        msg.pkt_len = len;
        fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
        
        //while (*(vu32*)(pkt_data-6) != 0xF00FF00F);
        //wifi_printf(""); // HACK force ARM7 to wait for ARM9 to copy packet
#if 0
        data_send_to_lwip(pkt_data, len);
#endif
#if 0
        wifi_card_print_mac("Dst:", data_hdr->dst_bssid);
        wifi_card_print_mac("Src:", data_hdr->src_bssid);
        
        wifi_printlnf("Data Pkt:");
        
        u8* dump_data = data_hdr->body;
        for (int i = 0; i < data_len; i += 8)
        {
            wifi_printlnf("%02x: %02x %02x %02x %02x %02x %02x %02x %02x", i, dump_data[i+0], dump_data[i+1], dump_data[i+2], dump_data[i+3], dump_data[i+4], dump_data[i+5], dump_data[i+6], dump_data[i+7]);
        }
#endif
    }
}

void data_send_pkt(u8* pkt_data, u32 len)
{
    int lock = enterCriticalSection();
    wifi_card_mbox0_send_packet(0x02, MBOXPKT_NOACK, pkt_data, len, 0); // 0x2008 causes broadcast packets?
    leaveCriticalSection(lock);
}

void data_send_pkt_idk(u8* pkt_data, u32 len)
{
    int lock = enterCriticalSection();
    wifi_card_mbox0_send_packet(0x02, MBOXPKT_NOACK, pkt_data, len, 0x2008); // 0x2008 causes broadcast packets? and might be faster?
    leaveCriticalSection(lock);
}

extern u16 wmi_idk;
void wmi_send_pkt(u16 wmi_type, u8 ack_type, const void* data, u16 len)
{
    int lock = enterCriticalSection();
    //memset(mbox_out_buffer, 0, round_up(len, 0x80));
    memset(mbox_out_buffer, 0, 0x8);

    mbox_out_buffer[0] = MBOXPKT_WMI;
    mbox_out_buffer[1] = ack_type;
    *(u16*)&mbox_out_buffer[2] = len+sizeof(u16);
    *(u16*)&mbox_out_buffer[4] = wmi_idk;
    
    *(u16*)&mbox_out_buffer[6+0] = wmi_type;

    // Truncate to mbox_out_buffer size
    if (len > (MBOX_TMPBUF_SIZE - 0x8))
        len = (MBOX_TMPBUF_SIZE - 0x8);

    if (data)
        memcpy(&mbox_out_buffer[6+2], data, len);
    
    len = len + 8;
    
    // Round to 0x80, block size
    // TODO can this size be reduced?
    len = round_up(len, 0x80);

    //hexdump(mbox_out_buffer, 20);
    
    wifi_card_mbox0_sendbytes(mbox_out_buffer, len); // len
    leaveCriticalSection(lock);
}


static bool mbox_has_lookahead = false;
static u32 mbox_lookahead = 0;

void* data_next_buf()
{
#if 0
    //for (int j = 0; j < 1000000; j++)
    while (1)
    {
        for (int i = 0; i < (ip_data_out_buf_totlen / DATA_BUF_LEN); i++)
        {
            void* dst = memUncached(ip_data_out_buf + (DATA_BUF_LEN * i));
            if (*(vu32*)dst == 0xF00FF00F)
            {
                //wifi_printf("ret %u\n", i);
                return dst;
            }
        }
        //wifi_printf("arm9 loop...\n");
    }
    //return ip_data_out_buf;
    return NULL;
#endif

#if 1
    void* ret = ip_data_out_buf + (DATA_BUF_LEN * ip_data_out_buf_idx);

    //wifi_printf("ret %u\n", ip_data_out_buf_idx);

    ip_data_out_buf_idx = (ip_data_out_buf_idx + 1) % (ip_data_out_buf_totlen / DATA_BUF_LEN);
    
    //memset(ret, 0, DATA_BUF_LEN);
    
    return ret;
#endif
}

u16 wifi_card_mbox0_readpkt(void)
{
    //memset(mbox_buffer, 0, MBOX_TMPBUF_SIZE);
    
    // Try and wait for mailbox data to arrive
    int timeout = 100;
    while (timeout--)
    {
        u8 sts = wifi_card_read_func1_u8(F1_HOST_INT_STATUS);
        if (sts & 1) break; // RX FIFO is not empty
    }
    
    // Timed out
    if (!timeout)
    {
        return 0;
    }
    
    u32 header = 0;
    
    if (mbox_has_lookahead) {
        header = mbox_lookahead;
        mbox_has_lookahead = false;
    }
    else {
        header = wifi_card_read_func1_u32(F1_RX_LOOKAHEAD0); // read lookahead
    }
    
    u8* read_buffer = mbox_buffer;
    
    u8 pkt_type = header & 0xFF;
    u8 ack_present = (header >> 8) & 0xFF;
    u16 len = header >> 16;
    u16 full_len = round_up(len+6, 0x80);
    
    if (ip_data_out_buf && (pkt_type == 2 || pkt_type == 3 || pkt_type == 4 || pkt_type == 5)) {
        //read_buffer = ip_data_out_buf + (ip_data_out_buf_idx * DATA_BUF_LEN);
        //ip_data_out_buf_idx = (ip_data_out_buf_idx + 1) % (ip_data_out_buf_totlen / DATA_BUF_LEN);
        read_buffer = data_next_buf();
        
        //while (*(vu32*)ip_data_out_buf != 0xF00FF00F);
    }
    
    // On the off chance that a packet gets parsed incorrectly (full_len off-by-one, etc)
    // just discard in chunks of 4 and be loud about it.
    if (len > 0x2000)
    {
        u16 actual_len = 0;
        for (int i = 0; i < 4; i++)
        {
            if (!(wifi_card_read_func1_u8(F1_HOST_INT_STATUS) & 1)) break;
            wifi_card_read_func1_u8(0xFF);
            actual_len++;
        }
        wifi_printlnf("?? Pkt len %x %lx, %x %lx", len, header, actual_len, wifi_card_read_func1_u32(F1_RX_LOOKAHEAD0));
        return 0;
    }

#ifdef SDIO_NO_BLOCKRW
    // Read out all data that we can
    //int end_cnt = 0;
    u16 actual_len = 0;
    while (1) // Read until we see the EOM bit thrice
    {
        while (!(wifi_card_read_func1_u8(F1_HOST_INT_STATUS) & 1)); // We don't have data...

        u8 val = wifi_card_read_func1_u8(0xFF); // (0x1000 - full_len)+actual_len
        
        if (actual_len < MBOX_TMPBUF_SIZE)
            read_buffer[actual_len] = val;

        actual_len++;
        
        if (actual_len >= full_len) break;
        
        // We've reached the last few bytes of the packet
        /*if (wifi_card_read_func1_u8(F1_MBOX_FRAME) & 0x10)
            end_cnt++;
        
        if (end_cnt > 3)
            break;*/
    }
#else
    u16 actual_len = full_len;
    u16 send_addr = 0x4000 - full_len;
    wifi_card_read_func1_block(send_addr, read_buffer, full_len);
#endif

    if (!actual_len) { return 0; }
    
    // Short packet? Shouldn't happen.
    if (actual_len < 6) {
        wifi_printlnf("Packet too short?? %x", actual_len);
        return actual_len;
    }

#ifndef SDIO_NO_BLOCKRW
    wifi_ndma_wait();
#endif
    
    u8 ack_len = read_buffer[4];
	if (!ack_present) {
		// ack_len can be set to 0xFF sometimes when an ack is not present, resulting in erroneous data...!
		ack_len = 0;
	}
    u16 len_pkt = len - ack_len;
    u16 pkt_cmd = *(u16*)&read_buffer[6];
    u8* pkt_data = &read_buffer[8];
    u8* ack_data = &read_buffer[6 + len_pkt];
    
    if (pkt_type == MBOXPKT_HTC)
    {
        htc_handle_pkt(pkt_cmd, pkt_data, len_pkt - 2, ack_len);
    }
    else if (pkt_type == MBOXPKT_WMI)
    {
        wmi_handle_pkt(pkt_cmd, pkt_data, len_pkt - 2, ack_len);
    }
    else if (pkt_type == 2 || pkt_type == 3 || pkt_type == 4 || pkt_type == 5) // one of my routers sends 0x04 for some reason
    {
        data_handle_pkt(pkt_data - 2, len_pkt);
    }
    else
    {
        wifi_printlnf("wat %02x %02x %02x %02x %02x %02x %02x %02x", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3], read_buffer[4], read_buffer[5], read_buffer[6], read_buffer[7]);
        wifi_printlnf("wat %02x %02x %02x %02x %02x %02x %02x %02x", read_buffer[8+0], read_buffer[8+1], read_buffer[8+2], read_buffer[8+3], read_buffer[8+4], read_buffer[8+5], read_buffer[8+6], read_buffer[8+7]);
    }
    
    // We can avoid costly CMD52s by using the ack block's lookahead
    mbox_has_lookahead = false;
    u16 ack_idx = 0;
    while (ack_idx < ack_len)
    {
        u8 type = ack_data[ack_idx++];
        u8 len = ack_data[ack_idx++];

        //if (type == 1 || type == 2)
        //    wifi_printlnf("%x %x", type, len);
        
        // Lookahead item
        if (type == 2 && len == 6 && !mbox_has_lookahead)
        {
            if (ack_data[ack_idx] == 0xAA && ack_data[ack_idx+5] == 0x55)
            {
                mbox_has_lookahead = true;
                mbox_lookahead = getle32(&ack_data[ack_idx+1]);

                //hexdump(&ack_data[ack_idx], 6);
            }
        }
        ack_idx += len;
    }
    
    if (ack_present != MBOXPKT_RETACK)
    {
        //wifi_printlnf("%02x %02x %04x %04x", pkt_type, ack_present, len, actual_len);
        //wifi_printlnf("%02x %02x %02x %02x %02x %02x %02x %02x", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3], read_buffer[4], read_buffer[5], read_buffer[6], read_buffer[7]);
        //wifi_printlnf("%02x %02x %02x %02x %02x %02x %02x %02x", read_buffer[8+0], read_buffer[8+1], read_buffer[8+2], read_buffer[8+3], read_buffer[8+4], read_buffer[8+5], read_buffer[8+6], read_buffer[8+7]);
    }

    
    return len;
}

//
// BMI
//

void wifi_card_bmi_start_firmware(void)
{
    //wifi_card_bmi_wait_count4();
    wifi_card_write_mbox0_u32(BMI_DONE);
}

void wifi_card_bmi_cmd_read_memory(u32 addr, u32 len, u8* out)
{
    // Possibly faster, does the same thing.
    if (len == 4)
    {
        *(u32*)&out[0] = wifi_card_read_intern_word_fast(addr);
        return;
    }

    wifi_card_write_mbox0_u32(BMI_READ_MEMORY);
    wifi_card_write_mbox0_u32(addr);
    wifi_card_write_mbox0_u32(len + (len % 4));

    for (int i = 0; i < len; i++)
    {
        out[i] = wifi_card_read_func1_u8(0xFF);
    }

     // Data must be u32 aligned
    for (int i = 0; i < len % 4; i++)
    {
        wifi_card_read_func1_u8(0xFF);
    }
}

void wifi_card_bmi_cmd_write_memory(u32 addr, u8* data, u32 len)
{
    // Possibly faster, does the same thing.
    if (len == 4)
    {
        wifi_card_write_intern_word(addr, *(u32*)&data[0]);
        return;
    }

    wifi_card_write_mbox0_u32(BMI_WRITE_MEMORY);
    wifi_card_write_mbox0_u32(addr);
    wifi_card_write_mbox0_u32(len + (len % 4));

    for (int i = 0; i < len; i++)
    {
        wifi_card_write_func1_u8(0xFF, data[i]);
    }

    for (int i = 0; i < len % 4; i++)
    {
        wifi_card_write_func1_u8(0xFF, 0);
    }
}

u32 wifi_card_bmi_execute(u32 addr, u32 arg)
{
    wifi_card_write_mbox0_u32(BMI_EXECUTE);
    wifi_card_write_mbox0_u32(addr);
    wifi_card_write_mbox0_u32(arg);

    return wifi_card_read_mbox0_u32();
}

u32 wifi_card_bmi_read_register(u32 addr)
{
    wifi_card_write_mbox0_u32(BMI_READ_SOC_REGISTER);
    wifi_card_write_mbox0_u32(addr);

    return wifi_card_read_mbox0_u32();
}

void wifi_card_bmi_write_register(u32 addr, u32 val)
{
    wifi_card_write_mbox0_u32(BMI_WRITE_SOC_REGISTER);
    wifi_card_write_mbox0_u32(addr);
    wifi_card_write_mbox0_u32(val);
}

u32 wifi_card_bmi_get_version()
{
    wifi_card_write_mbox0_u32(BMI_GET_TARGET_ID);

    u32 ret = wifi_card_read_mbox0_u32();
    if (ret == 0xFFFFFFFF) // Extended
    {
        u32 len = wifi_card_read_mbox0_u32(); // len
        if (len != 0xFFFFFFFF)
        {
            ret = wifi_card_read_mbox0_u32(); // version
            for (int i = 0; i < (len/4)-2; i++)
            {
                wifi_card_read_mbox0_u32();
            }
        }
    }

    return ret;
}

void wifi_card_bmi_lz_start(u32 addr)
{
    wifi_card_write_mbox0_u32(BMI_LZ_STREAM_START);
    wifi_card_write_mbox0_u32(addr);
}

void wifi_card_bmi_lz_data(const u8* data, u32 len)
{
    wifi_card_write_mbox0_u32(BMI_LZ_DATA);
    wifi_card_write_mbox0_u32(len + len % 4);

    //u32 len_aligned = len + len % 4;
    u32 len_bulk = len - (len % 0x80);
    
    wifi_card_mbox0_sendbytes(data, len_bulk);

    for (int i = len_bulk; i < len; i++)
    {
        wifi_card_write_func1_u8(0xFF, data[i]);
    }

     // Data must be u32 aligned
    for (int i = 0; i < len % 4; i++)
    {
        wifi_card_write_func1_u8(0xFF, 0);
    }
}

// BMI helpers

void wifi_card_bmi_write_memory(u32 addr, u8* data, u32 len)
{
    u32 chunk_size = 0x1F0;
    for (int i = 0; i < len; i += chunk_size)
    {
        u32 frag_size = i+chunk_size > len ? len-i : chunk_size;
        wifi_card_bmi_cmd_write_memory(addr + i, &data[i], frag_size);
    }
}

void wifi_card_bmi_lz_upload(u32 addr, const u8* data, u32 len)
{
    wifi_card_bmi_lz_start(addr);

    int chunk_size = 0x1F8;
    for (int i = 0; i < len; i += chunk_size)
    {
        u32 frag_size = i+chunk_size > len ? len-i : chunk_size;
        //wifi_printlnf("decomp lz: %08x %08x", i, frag_size);
        wifi_card_bmi_lz_data((u8*)&data[i], frag_size);
    }
}

static void wifi_card_handleMsg(int len, void* user_data)
{
    Wifi_FifoMsg msg;
    
    if (len < 4)
    {
        //wifi_printf("Bad msg len %x\n", len);
        return;
    }
    
    fifoGetDatamsg(FIFO_DSWIFI, len, (u8*)&msg);
    
    u32 cmd = msg.cmd;
    if (cmd == WIFI_IPCCMD_INIT_IOP)
    {
        wifi_printf_allowed = 1;
        //wifi_printf("iop val %x\n", cmd);
        wifi_card_device_init(wifi_card_dev_wlan);
    }
    else if (cmd == WIFI_IPCCMD_INITBUFS)
    {
        void* data = msg.pkt_data;
        u32 len = msg.pkt_len;
        
        ip_data_out_buf = data;
        ip_data_out_buf_totlen = len;
    }
    else if (cmd == WIFI_IPCCMD_SENDPKT)
    {
        void* data = msg.pkt_data;
        u32 len = msg.pkt_len;
        
        data_send_pkt_idk(data, len);
        
        *(vu32*)data = 0xF00FF00F; // mark packet processed
        
        // Craft and send msg
        //msg.cmd = WIFI_IPCINT_PKTSENT;
        //fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
    }
    else if (cmd == WIFI_IPCCMD_GET_DEVICE_MAC)
    {
        Wifi_FifoMsg msg;
        
        // Get MAC
        u8* mac = wmi_get_mac();
        
        // Craft and send msg
        msg.cmd = WIFI_IPCINT_DEVICE_MAC;
        memcpy(msg.mac_addr, mac, 6);
        fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
    }
    else if (cmd == WIFI_IPCCMD_GET_AP_MAC)
    {
        Wifi_FifoMsg msg;
        
        // Get MAC
        u8* mac = wmi_get_ap_mac();
        
        // Craft and send msg
        msg.cmd = WIFI_IPCINT_AP_MAC;
        memcpy(msg.mac_addr, mac, 6);
        fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
    }
    else
    {
        wifi_printf("iop val %x\n", cmd);
    }
}

// SDIO main functions
void wifi_card_init(void)
{
    mbox_buffer = memalign(16, MBOX_TMPBUF_SIZE);
    mbox_out_buffer = memalign(16, MBOX_TMPBUF_SIZE);

    // Read NVRAM settings
    u32 end_addr = 0x1FE00;
    
    readFirmware(0x20, &end_addr, sizeof(u32));
    end_addr *= 0x8;
    
    readFirmware(end_addr - 0x400, (void*)wifi_card_nvram_wep_configs, sizeof(wifi_card_nvram_wep_configs));
    readFirmware(NVRAM_ADDR_WIFICFG, (void*)wifi_card_nvram_configs, sizeof(wifi_card_nvram_configs));

    wifi_ndma_init();
    wifi_sdio_controller_init(REG_SDIO_BASE);

    fifoSetDatamsgHandler(FIFO_DSWIFI, wifi_card_handleMsg, 0);
    //wifi_card_device_init(wifi_card_dev_wlan);

}

void wifi_card_send_command(wifi_sdio_command cmd, u32 args)
{
    if(!device_curctx) return;
    device_curctx->tmio.buffer = NULL;
    wifi_sdio_send_command(&device_curctx->tmio, cmd, args);
}

void wifi_card_send_command_alt(wifi_sdio_command cmd, u32 args)
{
    if(!device_curctx) return;
    wifi_sdio_send_command(&device_curctx->tmio, cmd, args);
}

int wifi_card_device_init(wifi_card_device device)
{
    wifi_card_ctx* ctx = wifi_card_get_context(device);
    if(!ctx) return -1;

    memset(ctx, 0, sizeof(wifi_card_ctx));

    ctx->device = device;

    ctx->tmio.controller = REG_SDIO_BASE;
    ctx->tmio.clk_cnt = 0; // HCLK divider, 7=512 (largest possible) 0=2
    ctx->tmio.bus_width = 1;

    switch(device)
    {
        case wifi_card_dev_wlan:
            return wifi_card_wlan_init(ctx);
    }

    return -3;
}

void wifi_card_process_pkts()
{
    // Ack the interrupts
    u32 int_sts = wifi_card_read_func1_u32(F1_HOST_INT_STATUS);
    wifi_card_write_func1_u32(F1_HOST_INT_STATUS, int_sts);
    
    wifi_card_mbox0_readpkt();
}

void wifi_card_irq(void)
{
    wifi_card_process_pkts();
    
    //wmi_tick(); // TODO
}

void wifi_card_send_ready()
{
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCINT_READY;
    
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}

void wifi_card_send_connect()
{
    Wifi_FifoMsg msg;
    msg.cmd = WIFI_IPCINT_CONNECT;
    
    fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
}

void wifi_card_tick(void)
{
    if (!wifi_card_bInitted) return;

    //wifi_card_process_pkts();
    
    wmi_tick();

    //wifi_printlnf("tick end");
}

int wifi_card_wlan_init(wifi_card_ctx* ctx)
{
    if(!ctx) return -1;
    if(ctx->device != wifi_card_dev_wlan) return -1;
    
    if (!mbox_out_buffer || !mbox_buffer)
    {
        wifi_printf("bad mbox alloc %x %x\n", mbox_buffer, mbox_out_buffer);
        while (1);
    }
    
    device_curctx = ctx;

    ctx->tmio.port = 0;
    ctx->tmio.address = ctx->tmio.port;

    ctx->tmio.debug = false;
    ctx->tmio.break_early = false;
    ctx->tmio.block_size = 128;

    // TODO figure out if I should just put 3DS in another repo
#if 0
    // These are pre-TWL-firmlaunch settings
    REG_PDN_WLAN_CNT = 1; // Enable MP registers
    REG_GPIO3_DAT = 0; // select 3DS wifi? (maybe this refers to the wireless LED activity?)
    REG_GPIO5_DAT = 1; // deassert reset on Atheros

    MCU_SetWirelessLedState(true);
#endif

    *(vu16*)0x4004C04 &= ~0x100; // Select Atheros
    //*(vu16*)0x4004C04 |= 0x100; // Select legacy wireless
    
    u8 command[2];
    command[0] = WRITE_FOUT_1;
    command[1] = 0x80;
    rtcTransaction(command, 2, 0, 0);
    command[0] = WRITE_FOUT_2;
    command[1] = 0x00;
    rtcTransaction(command, 2, 0, 0);
    i2cWriteRegister(I2C_PM, 0x30, 0x13);
    *(vu16*)0x4004020 = 0x1; // SCFG_WL on?
    
    ctx->tmio.bus_width = 4;
    wifi_card_switch_device(ctx);
    ioDelay(0xF000);

    // Read register 0x00 (Revision) as a test
    wifi_card_read_func0_u8(0x00);
    if(ctx->tmio.status & 4) {
        ctx->tmio.bus_width = 1;
        wifi_card_switch_device(ctx);
        wifi_printlnf("Failed to read revision, assuming 1bit");
    }

    //if(is_firstboot)
    {
        wifi_printlnf("Resetting SDIO...");
        wifi_sdio_stop(ctx->tmio.controller);

        // TODO maybe toggle REG_GPIO5_DAT for a hardware reset

        // Operating Conditions Register.
        u32 ocr = 0;

        // CMD5 - IO_SEND_OP_COND
        wifi_sdio_command cmd5 = {
            .cmd = 5,
            .response_type = wifi_sdio_resp_48bit_ocr_no_crc7
        };

        /*
         * Note that this will run at least twice. Once with the OCR parameter
         * as zero, which will read the OCR from the card. In the state following
         * that, the card will not be ready to operate as no operating voltage has
         * been agreed.
         *
         * On the second successful iteration, the OCR parameter is set, which
         * currently just returns the range of voltages that the card gave us (the
         * ones it can support itself) back to the card. Really, we should check to
         * see what operating voltages NWM allows the card to run at.
         *
         * Following that command, the card will be initialized for I/O operations.
         */
        while(true)
        {
            while(true)
            {
                // CMD5 - IO_SEND_OP_COND
                wifi_card_send_command(cmd5, ocr);
                if(ctx->tmio.status & 4) {
                    wifi_printf("Skip opcond...\n");
                    goto skip_opcond;
                }

                if(ctx->tmio.status & 1) break;
            }

            // TODO: Probably should find out what NWM specifies as the voltage range.
            ocr = ctx->tmio.resp[0] & 0xFFFFFF;

            // Card ready to operate (initialized).
            if (ctx->tmio.resp[0] & 0x80000000)
                break;
        }

        // CMD3 - SEND_RELATIVE_ADDR (assign relative address to card).
        wifi_sdio_command cmd3 = {
            .cmd = 3,
            .response_type = wifi_sdio_resp_48bit
        };
        wifi_card_send_command(cmd3, 0);
        if(ctx->tmio.status & 4) return -1;
        ctx->tmio.address = ctx->tmio.resp[0] >> 16;

        // CMD7 - (DE)SELECT_CARD
        wifi_sdio_command cmd7 = {
            .cmd = 7,
            .response_type = wifi_sdio_resp_48bit_busy
        };
        wifi_card_send_command(cmd7, (u32)(ctx->tmio.address) << 16);
        if(ctx->tmio.status & 4) return -1;

skip_opcond:

        //wifi_card_write_func0_u8(0x6, 0x0);
        wifi_card_write_func0_u8(0x2, 0x0); // adding delay after this one wipes the loaded firmware??
        wifi_card_write_func0_u8(0x2, 0x2);
        ioDelay(0xF000);

        // Read register 0x07 (Bus Interface Control) of the CCCR.
        wifi_card_send_command(cmd52, 0x07 << 9);
        if(ctx->tmio.status & 4) return -1;
        u8 bus_interface_control = ctx->tmio.resp[0] & 0xFF;

        // Enable 4-bit mode and card detect
        bus_interface_control |= 0x82;

        // Write to the Bus Interface Control.
        wifi_card_write_func0_u8(0x07, bus_interface_control);
        if(ctx->tmio.status & 4) return -1;
        ctx->tmio.bus_width = 4;

        // Apply new bus width
        wifi_card_switch_device(ctx);

        // Write to the Power Control.
        wifi_card_write_func0_u8(0x12, 0x02);
        if(ctx->tmio.status & 4) return -1;

        // Set block size for func1 (lsb)
        wifi_card_write_func0_u8(0x110, ctx->tmio.block_size & 0xFF);
        if(ctx->tmio.status & 4) return -1;

        // Set block size for func1 (msb)
        wifi_card_write_func0_u8(0x111, ctx->tmio.block_size >> 8);
        if(ctx->tmio.status & 4) return -1;
        
        // Set block size for func0 (lsb)
        wifi_card_write_func0_u8(0x10, ctx->tmio.block_size & 0xFF);
        if(ctx->tmio.status & 4) return -1;

        // Set block size for func0 (msb)
        wifi_card_write_func0_u8(0x11, ctx->tmio.block_size >> 8);
        if(ctx->tmio.status & 4) return -1;
    }

    // Required?
    ioDelay(0xF000);

    // Read register 0x00 (Revision)
    u8 revision = wifi_card_read_func0_u8(0x00);
    if(ctx->tmio.status & 4) return -1;
    wifi_printf("Rev: %02x\n", revision);

    // Set function1 enable
    wifi_card_write_func0_u8(0x02, 0x02);
    //if(ctx->tmio.status & 4) return -1;

    while (true)
    {
        // Read register 0x02 (function enable) hibyte until it's ready
        if (wifi_card_read_func0_u8(0x3) == 0x02)
            break;
        //if(wifi_sdio_critical_fail) return -1;
    }

    //wifi_printlnf("Int status: %08x %02x", wifi_card_read_func1_u32(F1_HOST_INT_STATUS), wifi_card_read_func1_u8(F1_RX_LOOKAHEAD_VALID));

    // Get manufacturer
    device_manufacturer = wifi_card_read_func0_u32(0x1007);
    device_chip_id = wifi_card_read_intern_word(0x000040ec);
    if(ctx->tmio.status & 4) {
        wifi_printf("Error status reading manufacturer\n");
        //return -1;
    }
    wifi_printf("Mfg %08lx Cid %08lx (%s)\n", device_manufacturer, device_chip_id, wifi_card_get_chip_str());

    // TODO detect this?
    device_host_interest_addr = AR601x_HOST_INTEREST_ADDRESS;
    if (device_chip_id == CHIP_ID_AR6002)
        device_host_interest_addr = AR6002_HOST_INTEREST_ADDRESS;
    
    u32 is_uploaded = wifi_card_read_intern_word( device_host_interest_addr+0x58);

    if (!is_uploaded)
    {
        wifi_printf("%s needs firmware upload %lx.\n", wifi_card_get_chip_str(), is_uploaded);
    }

    // Reset into bootloader
    wifi_card_write_intern_word(0x4000, 0x00000100);
    ioDelay(0x10000);

    wifi_printf("Reset cause: %08lx\n", wifi_card_read_intern_word(0x40C0)); // RESET_CAUSE, expecting 0x02

    // FIFOs are weird after reset?
    //wifi_card_bmi_wait_count4();

    // Begin talking to bootloader
    wifi_printlnf("BMI version: %08lx", wifi_card_bmi_get_version());

    u32 mem_write32 = 0x2;
    wifi_card_bmi_cmd_write_memory(device_host_interest_addr, (u8*)&mem_write32, sizeof(u32));

    // TODO is this needed?
    u32 scratch0 = wifi_card_bmi_read_register(0x0180C0);
    wifi_card_bmi_write_register(0x0180C0, scratch0 | 0x8);

    // TODO is this needed?
    u32 wlan_system_sleep = wifi_card_bmi_read_register(0x0040C4); // WLAN_SYSTEM_SLEEP
    wifi_card_bmi_write_register(0x0040C4, wlan_system_sleep | 1);

    wifi_card_bmi_write_register(0x004028, 0x5); // WLAN_CLOCK_CONTROL
    wifi_card_bmi_write_register(0x004020, 0x0);

#if 0
    // All the part's addresses
    u32 parta_dst = 0x524C00;
    u32 partb_dst = 0x53FE18;
    u32 partc_dst = 0x527000;
    u32 partd_dst = 0x524C00;

    // TODO: Source AR6014 bins from SAFE_FIRM nwm?
    // Or just leave them because they're only 4KiB
    if (!is_uploaded)
    {
        // Upload and run D and C
        wifi_card_bmi_write_memory(partd_dst, (u8*)ar6014_partd_bin, ar6014_partd_bin_size);
        wifi_card_bmi_write_memory(partc_dst, (u8*)ar6014_partc_bin, ar6014_partc_bin_size);

        // This executes partC prior to partA (bootrom patches?)
        wifi_card_bmi_execute(partc_dst + 0x400000, partc_dst);

#if 0
        // Load PartA (decompressed
        wifi_card_bmi_write_memory(parta_dst, (u8*)ar6014_parta_bin, ar6014_parta_bin_size);
#endif

#if 1
        // Decompress A
        //wifi_card_bmi_lz_upload(parta_dst, ar6014_parta_bin, ar6014_parta_bin_size);
        wifi_card_bmi_lz_upload(parta_dst, nwm_ar6014_softap_bin, nwm_ar6014_softap_bin_size);
#endif

        // Upload D and B (is D needed?)
        wifi_card_bmi_write_memory(partd_dst, (u8*)ar6014_partd_bin, ar6014_partd_bin_size);
        wifi_card_bmi_write_memory(partb_dst, (u8*)ar6014_partb_bin, ar6014_partb_bin_size);

        // Store database addr somewhere
        mem_write32 = partb_dst;
        wifi_card_bmi_cmd_write_memory(device_host_interest_addr+0x18, (u8*)&mem_write32, sizeof(u32));
    }
#endif
    
    /*u16 new_country = 0x0;//0x8348 US, 0x8188 JP, 0x0 debug?
    device_eeprom_addr = wifi_card_read_intern_word(device_host_interest_addr+0x54);
    u32 test_regdom = wifi_card_read_intern_word(device_eeprom_addr+0x08);
    u32 new_regdom = (test_regdom & ~0xFFFF) | new_country;
    wifi_card_write_intern_word(device_eeprom_addr+0x08, new_regdom);
    u32 fix_check = wifi_card_read_intern_word(device_eeprom_addr+0x04);
    fix_check ^= (new_regdom ^ test_regdom);
    wifi_card_write_intern_word(device_eeprom_addr+0x04, fix_check);*/

    // BMI finish
    wifi_printlnf("BMI finishing...");

    // TODO: What was this about...? Is it just a warm boot thing?
    // seems to hang here, but no$'s wifiboot doesn't have any loops
    // to check if FIFOs are actually receiving...

    //wifi_card_bmi_write_register(0x0040C4, wlan_system_sleep & ~1);
    //wifi_printlnf("BMI finishing... aa");
    //wifi_card_bmi_write_register(0x0180C0, scratch0);

    //wifi_printlnf("BMI finishing... a");

    mem_write32 = 0x80;
    wifi_card_bmi_cmd_write_memory(device_host_interest_addr+0x6C, (u8*)&mem_write32, sizeof(u32));

    mem_write32 = 0x63;
    wifi_card_bmi_cmd_write_memory(device_host_interest_addr+0x74, (u8*)&mem_write32, sizeof(u32));

    // Launch firmware
    wifi_printlnf("Launching!");
    wifi_card_bmi_start_firmware();

    while (1)
    {
        u32 is_ready = wifi_card_read_intern_word(device_host_interest_addr+0x58);

        if (is_ready == 1) break;
        ioDelay(0x1000);
    }

    device_eeprom_addr = wifi_card_read_intern_word(device_host_interest_addr+0x54);
    device_eeprom_version = wifi_card_read_intern_word(device_eeprom_addr+0x10); // version, 609C0202

    wifi_printlnf("Firmware %08lx ready, handshaking...", device_eeprom_version);
    
    wifi_card_bInitted = true;
    
    // Scan for APs    
    //wmi_scan();
    
    // Enable IRQs
    irqSetAUX(IRQ_WIFI_SDIO_CARDIRQ, wifi_card_irq);
    irqEnableAUX(IRQ_WIFI_SDIO_CARDIRQ);
    wifi_sdio_enable_cardirq(REG_SDIO_BASE, true); // MelonDS seems to have trouble with IRQs? Comment out for MelonDS.
    
    wifi_card_write_func1_u32(F1_INT_STATUS_ENABLE, 0x010300D1); // INT_STATUS_ENABLE (or 0x1?)
    wifi_card_write_func0_u8(0x4, 0x3); // CCCR irq_enable, master+func1
    
    // 100ms timer
    timerStart(3, ClockDivider_1024, TIMER_FREQ_1024(1000 / SDIO_TICK_INTERVAL_MS), wifi_card_tick); //((u64)TIMER_CLOCK * SDIO_TICK_INTERVAL_MS) / 1000

    wifi_printlnf("wait ready");
    while (!wmi_is_ready())
    {
        //wifi_card_mbox0_readpkt(); // MelonDS seems to have trouble with IRQs? Uncomment for MelonDS.
    }

    wifi_printlnf("%s fully initialized!", wifi_card_get_chip_str());
    
    wmi_scan();

    return 0;
}

void wifi_card_deinit(void)
{
    wifi_sdio_enable_cardirq(REG_SDIO_BASE, false);
    
    irqDisableAUX(IRQ_WIFI_SDIO_CARDIRQ);
    irqDisable(IRQ_TIMER3);

    if (wifi_card_bInitted)
    {
        wifi_card_write_func1_u32(F1_INT_STATUS_ENABLE, 0x0); // INT_STATUS_ENABLE
        wifi_card_write_func0_u8(0x4, 0x0); // CCCR irq_enable
    }
    
    wifi_card_bInitted = false;
    
    wifi_printlnf("%s deinitted", wifi_card_get_chip_str());
}

bool wifi_card_initted()
{
    return wifi_card_bInitted && wmi_is_ready();
}

u32 wifi_card_host_interest_addr()
{
    return device_host_interest_addr;
}

wifi_card_ctx* wifi_card_get_context(wifi_card_device device)
{
    switch(device)
    {
        case wifi_card_dev_wlan:
            return &wlan_ctx;

        default: return NULL;
    }
}

void wifi_card_switch_device(wifi_card_ctx* ctx)
{
    if(!ctx) return;
    wifi_sdio_switch_device(&ctx->tmio);
    
    device_curctx = ctx;
}

void wifi_card_setclk(u32 data)
{
    wifi_sdio_setclk(REG_SDIO_BASE, data);
}

void wifi_card_stop(void)
{
    wifi_sdio_stop(REG_SDIO_BASE);
}
