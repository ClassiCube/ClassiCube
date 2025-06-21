/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _WIFI_SDIO_H
#define _WIFI_SDIO_H

#include "common.h"

// Toggle for debug features.
#define WIFI_SDIO_DEBUG

// Toggle for DATA32 support.
#define WIFI_SDIO_DATA32

// TMIO register offsets.
#define WIFI_SDIO_OFFS_CMD             (0x000)
#define WIFI_SDIO_OFFS_PORT_SEL        (0x002)

#define WIFI_SDIO_OFFS_CMD_PARAM       (0x004)
#define WIFI_SDIO_OFFS_CMD_PARAM0      (0x004)
#define WIFI_SDIO_OFFS_CMD_PARAM1      (0x006)

#define WIFI_SDIO_OFFS_STOP            (0x008)
#define WIFI_SDIO_OFFS_DATA16_BLK_CNT  (0x00A)

#define WIFI_SDIO_OFFS_RESP0           (0x00C)
#define WIFI_SDIO_OFFS_RESP1           (0x00E)
#define WIFI_SDIO_OFFS_RESP2           (0x010)
#define WIFI_SDIO_OFFS_RESP3           (0x012)
#define WIFI_SDIO_OFFS_RESP4           (0x014)
#define WIFI_SDIO_OFFS_RESP5           (0x016)
#define WIFI_SDIO_OFFS_RESP6           (0x018)
#define WIFI_SDIO_OFFS_RESP7           (0x01A)

#define WIFI_SDIO_OFFS_IRQ_STAT        (0x01C)
#define WIFI_SDIO_OFFS_IRQ_STAT0       (0x01C)
#define WIFI_SDIO_OFFS_IRQ_STAT1       (0x01E)

#define WIFI_SDIO_OFFS_IRQ_MASK        (0x020)
#define WIFI_SDIO_OFFS_IRQ_MASK0       (0x020)
#define WIFI_SDIO_OFFS_IRQ_MASK1       (0x022)

#define WIFI_SDIO_OFFS_CLK_CNT         (0x024)
#define WIFI_SDIO_OFFS_DATA16_BLK_LEN  (0x026)
#define WIFI_SDIO_OFFS_CARD_OPT        (0x028)

#define WIFI_SDIO_OFFS_ERR_DETAIL      (0x02C)
#define WIFI_SDIO_OFFS_ERR_DETAIL0     (0x02C)
#define WIFI_SDIO_OFFS_ERR_DETAIL1     (0x02E)

#define WIFI_SDIO_OFFS_DATA16_FIFO     (0x030)

#define WIFI_SDIO_OFFS_CARDIRQ_CTL     (0x034)
#define WIFI_SDIO_OFFS_CARDIRQ_STAT    (0x036)
#define WIFI_SDIO_OFFS_CARDIRQ_MASK    (0x038)

#define WIFI_SDIO_OFFS_DATA16_CNT      (0x0D8)
#define WIFI_SDIO_OFFS_RESET           (0x0E0)

#define WIFI_SDIO_OFFS_PROTECTED       (0x0F6) // Bit 0 determines if SD is protected or not?

#define WIFI_SDIO_OFFS_IRQ32           (0x100)
#define WIFI_SDIO_OFFS_DATA32_BLK_LEN  (0x104)
#define WIFI_SDIO_OFFS_DATA32_BLK_CNT  (0x108)

#ifdef ARM11
#define WIFI_SDIO_OFFS_DATA32_FIFO     (0x200000u)
#else
#define WIFI_SDIO_OFFS_DATA32_FIFO     (0x10C)
#endif

// WIFI_SDIO_STAT status bits.
#define WIFI_SDIO_STAT0_CMDRESPEND     (0x0001)
#define WIFI_SDIO_STAT0_DATAEND        (0x0004)
#define WIFI_SDIO_STAT0_CARD_REMOVE    (0x0008)
#define WIFI_SDIO_STAT0_CARD_INSERT    (0x0010)
#define WIFI_SDIO_STAT0_SIGSTATE       (0x0020)
#define WIFI_SDIO_STAT0_WRPROTECT      (0x0080)
#define WIFI_SDIO_STAT0_CARD_REMOVE_A  (0x0100)
#define WIFI_SDIO_STAT0_CARD_INSERT_A  (0x0200)
#define WIFI_SDIO_STAT0_SIGSTATE_A     (0x0400)
#define WIFI_SDIO_STAT1_CMD_IDX_ERR    (0x0001)
#define WIFI_SDIO_STAT1_CRCFAIL        (0x0002)
#define WIFI_SDIO_STAT1_STOPBIT_ERR    (0x0004)
#define WIFI_SDIO_STAT1_DATATIMEOUT    (0x0008)
#define WIFI_SDIO_STAT1_RXOVERFLOW     (0x0010)
#define WIFI_SDIO_STAT1_TXUNDERRUN     (0x0020)
#define WIFI_SDIO_STAT1_CMDTIMEOUT     (0x0040)
#define WIFI_SDIO_STAT1_RXRDY          (0x0100)
#define WIFI_SDIO_STAT1_TXRQ           (0x0200)
#define WIFI_SDIO_STAT1_ILL_FUNC       (0x2000)
#define WIFI_SDIO_STAT1_CMD_BUSY       (0x4000)
#define WIFI_SDIO_STAT1_ILL_ACCESS     (0x8000)

#define WIFI_SDIO_MASK_ALL             (0x837F031D)

#define WIFI_SDIO_MASK_READOP          (WIFI_SDIO_STAT1_RXRDY | WIFI_SDIO_STAT1_DATAEND)
#define WIFI_SDIO_MASK_WRITEOP         (WIFI_SDIO_STAT1_TXRQ  | WIFI_SDIO_STAT1_DATAEND)

#define WIFI_SDIO_MASK_ERR             (WIFI_SDIO_STAT1_ILL_ACCESS | WIFI_SDIO_STAT1_CMDTIMEOUT | WIFI_SDIO_STAT1_TXUNDERRUN | WIFI_SDIO_STAT1_RXOVERFLOW | \
                                   WIFI_SDIO_STAT1_DATATIMEOUT | WIFI_SDIO_STAT1_STOPBIT_ERR | WIFI_SDIO_STAT1_CRCFAIL | WIFI_SDIO_STAT1_CMD_IDX_ERR)

typedef struct {
    void* controller;
    u16 address;
    u8 port;
    bool debug;

    u32 clk_cnt;
    u8 bus_width;

    void* buffer;
    size_t size;

    u32 resp[4];
    u32 status;

    u16 stat0;
    u16 stat1;

    u16 err0;
    u16 err1;

    u16 block_size;

    bool break_early;
} wifi_sdio_ctx;

typedef union
{
    struct {
        u32 cmd:6;

        u32 command_type:2;
        u32 response_type:3;

        u32 data_transfer:1;
        u32 data_direction:1;
        u32 data_length:1;

        u32 secure:1;
        u32 unknown:1;
    };

    u16 raw;
} wifi_sdio_command;

typedef enum
{
    wifi_sdio_single_block = 0,
    wifi_sdio_multiple_block = 1
} wifi_sdio_data_length;

typedef enum
{
    wifi_sdio_data_write = 0,
    wifi_sdio_data_read = 1
} wifi_sdio_data_direction;

typedef enum
{
    wifi_sdio_cmd = 0,
    wifi_sdio_acmd = 1
} wifi_sdio_command_type;

typedef enum
{
    /*
     * Response size - the TMIO controller does some pre-processing on commands and their
     * responses. The response returned by the controller following its processing has had
     * the CRC7 bits (the controller asserts an IRQ with a CRC7 bit set in IRQ_STAT if invalid),
     * end bit, start bit, direction bit and command index removed. This (usually) amounts to
     * 16 bits, which gets us our returned (from the controller) 32-bit word for commands with
     * 48-bit responses.
     *
     * Commands with 136-bit responses (such as SEND_CSD and ALL_SEND_CID) have 8 bits
     * removed, this includes the start bit, transmission bit, end bit and the 6 reserved
     * bits, which leaves us with the raw 128-bit (4 32-bit response words) CID or CSD register.
     * These registers are actually meant to have an internal CRC7, but this is also removed
     * (actually cleared to zero) by the controller, since we need the SHA of the CID in
     * a 120-bit form padded to 128-bits with zeroes in order to decrypt the eMMC partitions,
     * and we've been doing that for months via the ALL_SEND_CID command (which would give us
     * an incorrect result if the CRC7 bits were preserved).
     */
    wifi_sdio_resp_none = 3,
    wifi_sdio_resp_48bit = 4,
    wifi_sdio_resp_48bit_busy = 5,
    wifi_sdio_resp_136bit = 6,
    wifi_sdio_resp_48bit_ocr_no_crc7 = 7
} wifi_sdio_response_type;

void wifi_sdio_controller_init(void* controller);
void wifi_sdio_switch_device(wifi_sdio_ctx* ctx);
void wifi_sdio_send_command(wifi_sdio_ctx* ctx, wifi_sdio_command cmd, u32 args);
void wifi_sdio_send_command_alt(wifi_sdio_ctx* ctx, wifi_sdio_command cmd, u32 args);

u16 wifi_sdio_read16(void* controller, u32 reg);
u32 wifi_sdio_read32(void* controller, u32 reg);
void wifi_sdio_write16(void* controller, u32 reg, u16 val);
void wifi_sdio_write32(void* controller, u32 reg, u32 val);
void wifi_sdio_mask16(void* controller, u32 reg, u16 clear, u16 set);

u32 wifi_sdio_read32(void* controller, u32 reg);
void wifi_sdio_write32(void* controller, u32 reg, u32 val);
void wifi_sdio_mask32(void* controller, u32 reg, u32 clear, u32 set);

void wifi_sdio_stop(void* controller);
void wifi_sdio_setclk(void* controller, u32 data);

void wifi_sdio_enable_cardirq(void* controller, bool en);

#endif

