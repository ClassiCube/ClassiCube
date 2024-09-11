/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wifi_sdio.h"

#ifdef WIFI_SDIO_DEBUG
#include "wifi_debug.h"
#endif

#include "wifi_ndma.h"

#define WIFI_SDIO_NDMA

void wifi_sdio_controller_init(void* controller)
{
    void* c = controller;
    if(!c) return;

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x0800, 0x0000);
    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x1000, 0x0000);
    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x0000, 0x0402);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_DATA16_CNT, 0x0022, 0x0002);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_DATA16_CNT, 0x0020, 0x0000);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA32_BLK_LEN, 128);

    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA32_BLK_CNT, 0x0001);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_RESET, 0x0003, 0x0000);
    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_RESET, 0x0000, 0x0003);

    // Disable all interrupts.
    wifi_sdio_write32(c, WIFI_SDIO_OFFS_IRQ_MASK, 0xFFFFFFFF);

    wifi_sdio_mask16(c, 0x0FC, 0x0000, 0x00DB);
    wifi_sdio_mask16(c, 0x0FE, 0x0000, 0x00DB);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_PORT_SEL, 0b11, 0);

    wifi_sdio_write16(c, WIFI_SDIO_OFFS_CLK_CNT, 0x0020);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_CARD_OPT, 0x40EE);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x8000, 0x0000);
    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x0000, 0x0100);
    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ32, 0x0100, 0x0000);

    wifi_sdio_mask16(c, WIFI_SDIO_OFFS_PORT_SEL, 0b11, 0);

    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA16_BLK_LEN, 128);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_STOP, 0x0100);
}

void wifi_sdio_send_command(wifi_sdio_ctx* ctx, wifi_sdio_command cmd, u32 args)
{
    if(!ctx) return;
    void* c = ctx->controller;
    if(!c) return;

    // Safety fallback
    if (!ctx->block_size)
        ctx->block_size = 512;

    void* buffer = ctx->buffer;
    size_t size = ctx->size;

    ctx->status = 0;
    u16 stat0 = 0, stat1 = 0;
    u16 stat0_completion_flags = 0;

    u16 cnt32 = 0;

    // Are we expecting a response? We need to wait for it.
    if(cmd.response_type != wifi_sdio_resp_none)
        stat0_completion_flags |= WIFI_SDIO_STAT0_CMDRESPEND;

    // Are we doing a data transfer? We need to wait for it to end.
    if(cmd.data_transfer)
        stat0_completion_flags |= WIFI_SDIO_STAT0_DATAEND;

#ifdef WIFI_SDIO_DEBUG
    if(ctx->debug)
        wifi_printlnf("CMD#: 0x%04X (%u) (%X) -> %u:%u",
                cmd.raw, cmd.cmd, stat0_completion_flags, ctx->port, ctx->address);
#endif

    bool buffer32 = false;
    if(buffer && ((u32)buffer & 3) == 0) buffer32 = true;
    
    // Force alignment
    if (buffer && !buffer32) {
        ctx->status |= 4;
        return;
    }

    // Wait until the SDIO controller is not busy.
    while(wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT1) & WIFI_SDIO_STAT1_CMD_BUSY);

    // ACK all interrupts and halt
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ_STAT0, 0);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ_STAT1, 0);
    //wifi_sdio_mask16(c, WIFI_SDIO_OFFS_STOP, 1, 0);
    
    // Write command arguments.
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_CMD_PARAM0, args & 0xFFFF);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_CMD_PARAM1, args >> 16);

    // Set block len and counts
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA16_BLK_LEN, ctx->block_size);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA16_BLK_CNT, size / ctx->block_size);
    
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA32_BLK_LEN, ctx->block_size);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA32_BLK_CNT, size / ctx->block_size);    

    // Data32 mode
    bool is_block = (cmd.data_length == wifi_sdio_multiple_block);
    if (is_block) {
        wifi_sdio_write16(c, WIFI_SDIO_OFFS_DATA16_CNT, 0x0002);
        if(cmd.data_direction == wifi_sdio_data_read)
            wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ32, 0xC02);
        else
            wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ32, 0x1402);
    }

    // Write command.
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_CMD, cmd.raw);

#ifdef WIFI_SDIO_NDMA
    if(cmd.data_direction == wifi_sdio_data_read && is_block && buffer)
    {
        wifi_ndma_read(buffer, size);
        return;
    }
    else if (cmd.data_direction == wifi_sdio_data_write && is_block && buffer)
    {
        wifi_ndma_write(buffer, size);
        return;
    }
#endif

    while(true)
    {
        stat1 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT1);

        cnt32 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ32);

        // Ready to receive data.
        if(cnt32 & 0x100)
        {
            // Are we actually meant to receive data?
            if(cmd.data_direction == wifi_sdio_data_read && buffer)
            {
                // ACK ready state.
                wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ_STAT1, WIFI_SDIO_STAT1_RXRDY, 0);

                if(size > ctx->block_size - 1)
                {
//#ifdef WIFI_SDIO_NDMA
                    //wifi_ndma_read(buffer, ctx->block_size);
                    //buffer += ctx->block_size;
//#else
                    void* buffer_end = buffer + ctx->block_size;
                    
                    while(buffer != buffer_end)
                    {
                        *(u32*)buffer = wifi_sdio_read32(c, WIFI_SDIO_OFFS_DATA32_FIFO);
                        
                        //wifi_printlnf("asdf %x", *(u32*)buffer);
                        
                        buffer += sizeof(u32);
                    }
//#endif
                    size -= ctx->block_size;
                }
            }
        }

        // Data transmission requested.
        //if(!(cnt32 & 0x200))
        if(stat1 & WIFI_SDIO_STAT1_TXRQ)
        {
            // Are we actually meant to write data?
            if(cmd.data_direction == wifi_sdio_data_write && buffer)
            {
                // ACK request.
                wifi_sdio_mask16(c, WIFI_SDIO_OFFS_IRQ_STAT1, WIFI_SDIO_STAT1_TXRQ, 0);

                if(size > ctx->block_size-1)
                {
//#ifdef WIFI_SDIO_NDMA
                    //wifi_ndma_write(buffer, ctx->block_size);
                    //buffer += ctx->block_size;
//#else
                    void* buffer_end = buffer + ctx->block_size;

                    while(buffer != buffer_end)
                    {
                        u32 data = *(u32*)buffer;
                        buffer += sizeof(u32);
                        wifi_sdio_write32(c, WIFI_SDIO_OFFS_DATA32_FIFO, data);
                    }
//#endif
                    size -= ctx->block_size;
                }
            }
        }

        // Has an error been asserted? Exit if so.
        if(stat1 & WIFI_SDIO_MASK_ERR)
        {
#ifdef WIFI_SDIO_DEBUG
            if(ctx->debug)
                wifi_printlnf("ERR#: %04X 0000", stat1 & WIFI_SDIO_MASK_ERR);
#endif
            // Error flag.
            ctx->status |= 4;
            break;
        }
        
        bool end_cond = !(stat1 & WIFI_SDIO_STAT1_CMD_BUSY);
        if (is_block) {
            stat0 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT0);
            end_cond = (stat0 & WIFI_SDIO_STAT0_CMDRESPEND && !size);
        }

        // Not busy...
        if(end_cond)
        {
            stat0 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT0);

            // Set response end flag.
            if(stat0 & WIFI_SDIO_STAT0_CMDRESPEND)
                ctx->status |= 1;

            // Set data end flag.
            if(stat0 & WIFI_SDIO_STAT0_DATAEND)
                ctx->status |= 2;

            // If done (all completion criteria), exit.
            if((stat0 & stat0_completion_flags) == stat0_completion_flags)
                break;
        }

        if(ctx->break_early && !size)
        {
            break;
        }
    }

    ctx->stat0 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT0);
    ctx->stat1 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_IRQ_STAT1);

    ctx->err0 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_ERR_DETAIL0);
    ctx->err1 = wifi_sdio_read16(c, WIFI_SDIO_OFFS_ERR_DETAIL1);

    // ACK all interrupts.
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ_STAT0, 0);
    wifi_sdio_write16(c, WIFI_SDIO_OFFS_IRQ_STAT1, 0);

    // If the command has a response, pull it in to sdmmc_ctx.
    if(cmd.response_type != wifi_sdio_resp_none)
    {
        ctx->resp[0] = wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP0) | (u32)(wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP1) << 16);
        ctx->resp[1] = wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP2) | (u32)(wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP3) << 16);
        ctx->resp[2] = wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP4) | (u32)(wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP5) << 16);
        ctx->resp[3] = wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP6) | (u32)(wifi_sdio_read16(c, WIFI_SDIO_OFFS_RESP7) << 16);
    }

#ifdef WIFI_SDIO_DEBUG
    if(ctx->debug)
    {
        wifi_printlnf("STAT: %04X %04X (%X) INFO: %04X %04X", ctx->stat1, ctx->stat0,
                ctx->status, ctx->err1, ctx->err0);

        if(cmd.response_type != wifi_sdio_resp_none)
        {
            if(cmd.response_type == wifi_sdio_resp_136bit) {
                wifi_printlnf("RESP: %08lX %08lX %08lX %08lX",
                        ctx->resp[0], ctx->resp[1], ctx->resp[2], ctx->resp[3]);
            } else {
                wifi_printlnf("RESP: %08lX", ctx->resp[0]);
            }
        }

        wifi_printlnf("");
    }
#endif
}

void wifi_sdio_switch_device(wifi_sdio_ctx* ctx)
{
    if(!ctx) return;

    // Reconfigure the bus to talk to the new device.
    wifi_sdio_mask16(ctx->controller, WIFI_SDIO_OFFS_PORT_SEL, 0b11, ctx->port);
    wifi_sdio_setclk(ctx->controller, ctx->clk_cnt);

    // WIFI_SDIO_CARD_OPT bit 15: bus width (0 = 4-bit, 1 = 1-bit).
    if(ctx->bus_width == 4)
        wifi_sdio_mask16(ctx->controller, WIFI_SDIO_OFFS_CARD_OPT, BIT(15), 0);
    else
        wifi_sdio_mask16(ctx->controller, WIFI_SDIO_OFFS_CARD_OPT, 0, BIT(15));
}

u16 wifi_sdio_read16(void* controller, u32 reg)
{
    return *(vu16*)(controller + reg);
}

u32 wifi_sdio_read32(void* controller, u32 reg)
{
    return *(vu32*)(controller + reg);
}

void wifi_sdio_write16(void* controller, u32 reg, u16 val)
{
    *(vu16*)(controller + reg) = val;
}

void wifi_sdio_write32(void* controller, u32 reg, u32 val)
{
    *(vu32*)(controller + reg) = val;
}

void wifi_sdio_mask16(void* controller, u32 reg, u16 clear, u16 set)
{
    u16 val = wifi_sdio_read16(controller, reg);
    val &= ~clear;
    val |= set;
    wifi_sdio_write16(controller, reg, val);
}

void wifi_sdio_mask32(void* controller, u32 reg, u32 clear, u32 set)
{
    u32 val = wifi_sdio_read32(controller, reg);
    val &= ~clear;
    val |= set;
    wifi_sdio_write32(controller, reg, val);
}

void wifi_sdio_setclk(void* controller, u32 data)
{
    wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CLK_CNT, 0x100, 0);
    wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CLK_CNT, 0x2FF, data & 0x2FF);
    wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CLK_CNT, 0x0, 0x100);
}

void wifi_sdio_stop(void* controller)
{
    wifi_sdio_write16(controller, WIFI_SDIO_OFFS_STOP, 0x100);
}

void wifi_sdio_enable_cardirq(void* controller, bool en)
{
    if (en)
    {
        wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CARDIRQ_CTL, 0, 0x0001);
        wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CARDIRQ_MASK, 0x0003, 0);
    }
    else
    {
        wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CARDIRQ_CTL, 1, 0);
        wifi_sdio_mask16(controller, WIFI_SDIO_OFFS_CARDIRQ_MASK, 0, 3);
    }
}

u16 wifi_sdio_get_cardirq_stat(void* controller)
{
    return wifi_sdio_read16(controller, WIFI_SDIO_OFFS_CARDIRQ_STAT);
}
