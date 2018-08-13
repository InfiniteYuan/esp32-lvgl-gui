// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ugfx_driver_config.h"

/*C Includes*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*RTOS Includes*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

/*SPI Includes*/
#include "lcd_com.h"
#include "screen_NT35510.h"
#include "lcd_adapter.h"

#include "sdkconfig.h"
#include "esp_log.h"

static lcd_dev_t * lcd_obj = NULL;
static uint16_t* pFrameBuffer = NULL;

lcd_dev_t *LcdNt35510Create(uint16_t x_size, uint16_t y_size);

#if UGFX_DRIVER_AUTO_FLUSH_ENABLE
SemaphoreHandle_t flush_sem = NULL;

void board_lcd_flush_task(void* arg)
{
    portBASE_TYPE res;
    while(1) {
        res = xSemaphoreTake(flush_sem, portMAX_DELAY);
        if (res == pdTRUE) {
            lcd_obj->lcd.LcdDrawBmp((uint16_t*)pFrameBuffer, 0, 0, UGFX_DRIVER_SCREEN_WIDTH, UGFX_DRIVER_SCREEN_HEIGHT);
            vTaskDelay(UGFX_DRIVER_AUTO_FLUSH_INTERVAL / portTICK_RATE_MS);
        }
    }
}
#endif

void board_lcd_init()
{
    nvs_flash_init();

    /*Initialize LCD*/
    if(lcd_obj == NULL) {
        lcd_obj = LcdNt35510Create(UGFX_DRIVER_SCREEN_WIDTH, UGFX_DRIVER_SCREEN_HEIGHT);
    }

#if UGFX_DRIVER_AUTO_FLUSH_ENABLE
    // For framebuffer mode and flush
    if (flush_sem == NULL) {
        flush_sem = xSemaphoreCreateBinary();
    }
    xTaskCreate(board_lcd_flush_task, "flush_task", 1500, NULL, 5, NULL);
#endif
}

void board_lcd_flush(int16_t x, int16_t y, uint16_t* bitmap, int16_t w, int16_t h)
{
#if UGFX_DRIVER_AUTO_FLUSH_ENABLE
    pFrameBuffer = bitmap;
    xSemaphoreGive(flush_sem);
#else
    lcd_obj->lcd.LcdDrawBmp(bitmap, x, y, UGFX_DRIVER_SCREEN_WIDTH, UGFX_DRIVER_SCREEN_HEIGHT);
#endif
}

void board_lcd_write_cmd(uint16_t cmd)
{
    WriteCmd(cmd);
}

void board_lcd_write_data(uint16_t data)
{
    WriteData(data);
}

void board_lcd_write_data_byte(uint8_t data)
{
    WriteData(data);
}

void board_lcd_write_datas(uint16_t data, uint16_t x, uint16_t y)
{
    lcd_obj->lcd.LcdFillArea(data, x, y);
}

void board_lcd_write_cmddata(uint16_t cmd, uint32_t data)
{
    WriteCmdData(cmd, data);
}

void board_lcd_set_backlight(uint16_t data)
{
    /* Code here*/
}

#if CONFIG_LVGL_USE_CUSTOM_DRIVER

/* lvgl include */
#include "lvgl_disp_config.h"
#include "iot_lvgl.h"

/*Write the internal buffer (VDB) to the display. 'lv_flush_ready()' has to be called when finished*/
void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    lcd_obj->lcd.LcdDrawBmp((uint16_t *)color_p, (uint16_t)x1, (uint16_t)y1, (uint16_t)(x2-x1+1), (uint16_t)(y2-y1+1));
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_flush_ready();
}

/*Fill an area with a color on the display*/
void ex_disp_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
    lcd_obj->lcd.LcdFillRet((uint16_t)color.full, (uint16_t)x1, (uint16_t)y1, (uint16_t)(x2-x1+1), (uint16_t)(y2-y1+1));
}

/*Write pixel map (e.g. image) to the display*/
void ex_disp_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    lcd_obj->lcd.LcdDrawBmp((uint16_t *)color_p, (uint16_t)x1, (uint16_t)y1, (uint16_t)(x2-x1+1), (uint16_t)(y2-y1+1));
}

void lvgl_lcd_display_init()
{
    /*Initialize LCD*/
    if(lcd_obj == NULL) {
        lcd_obj = LcdNt35510Create(UGFX_DRIVER_SCREEN_WIDTH, UGFX_DRIVER_SCREEN_HEIGHT);
    }

    lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    switch(CONFIG_LVGL_DISP_ROTATE){
        default:
        case 0:
            WriteCmd(0x3600);
            WriteData(0x00|0x00);
            break;
        case 1:
            WriteCmd(0x3600);
            WriteData(0xA0|0x00);
            break;
        case 2:
            WriteCmd(0x3600);
            WriteData(0xC0|0x00);
            break;
        case 3:
            WriteCmd(0x3600);
            WriteData(0x60|0x00);
            break;
    }

    /* Set up the functions to access to your display */
    if (LV_VDB_SIZE != 0) {
        disp_drv.disp_flush = ex_disp_flush;            /*Used in buffered mode (LV_VDB_SIZE != 0  in lv_conf.h)*/
    } else if (LV_VDB_SIZE == 0) {
        disp_drv.disp_fill = ex_disp_fill;              /*Used in unbuffered mode (LV_VDB_SIZE == 0  in lv_conf.h)*/
        disp_drv.disp_map = ex_disp_map;                /*Used in unbuffered mode (LV_VDB_SIZE == 0  in lv_conf.h)*/
    }

    /* Finally register the driver */
    lv_disp_drv_register(&disp_drv);
}

#endif
