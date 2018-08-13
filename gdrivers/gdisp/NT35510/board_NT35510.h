/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "sdkconfig.h"
#include "ugfx_driver_config.h"

#if UGFX_LCD_DRIVER_API_MODE
#include "gos_freertos_priv.h"
#include "gfx.h"

#include "lcd_adapter.h"
#include "lcd_com.h"
#include "NT35510.h"

// Static inline functions
static GFXINLINE void init_board(GDisplay *g)
{
    board_lcd_init();
}

static GFXINLINE void post_init_board(GDisplay *g)
{
    (void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state)
{
    (void) g;
}

static GFXINLINE void acquire_bus(GDisplay *g)
{
    (void) g;
}

static GFXINLINE void release_bus(GDisplay *g)
{
    (void) g;
}

static GFXINLINE void acquire_sem()
{
    /* Code here*/
}

static GFXINLINE void release_sem()
{
    /* Code here*/
}

static GFXINLINE void write_cmd(GDisplay *g, uint16_t cmd)
{
    board_lcd_write_cmd(cmd);
}

static GFXINLINE void write_data(GDisplay *g, uint16_t data)
{
    board_lcd_write_data(data);
}

static GFXINLINE void write_data_repeats(GDisplay *g, uint16_t data, uint16_t x, uint16_t y)
{
    board_lcd_write_datas(data, x, y);
}

static GFXINLINE void write_data_byte(GDisplay *g, uint8_t data)
{
    board_lcd_write_data_byte(data);
}

static GFXINLINE void write_cmddata(GDisplay *g, uint16_t cmd, uint32_t data)
{
    board_lcd_write_cmddata(cmd, data);
}

static GFXINLINE void set_backlight(GDisplay *g, uint16_t data)
{
    (void) g;
}

static GFXINLINE void set_viewport(GDisplay *g)
{
    WriteReg((uint16_t)(NT35510_CASET), (uint16_t)(g->p.x)>>8);
    WriteReg((uint16_t)(NT35510_CASET+1), (uint16_t)(g->p.x) & 0xff);
    WriteReg((uint16_t)(NT35510_CASET+2), (uint16_t)(g->p.x+g->p.cx-1) >> 8);
    WriteReg((uint16_t)(NT35510_CASET+3), (uint16_t)(g->p.x+g->p.cx-1) & 0xff);
    WriteReg((uint16_t)(NT35510_RASET), (uint16_t)(g->p.y)>>8);
    WriteReg((uint16_t)(NT35510_RASET+1), (uint16_t)(g->p.y)&0xff);
    WriteReg((uint16_t)(NT35510_RASET+2), (uint16_t)(g->p.y+g->p.cy-1) >> 8);
    WriteReg((uint16_t)(NT35510_RASET+3), (uint16_t)(g->p.y+g->p.cy-1) & 0xff);
    WriteCmd((uint16_t)NT35510_RAMWR);
}

#endif /* _GDISP_LLD_BOARD_H */

#endif 
