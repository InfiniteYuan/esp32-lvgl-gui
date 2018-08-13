#ifndef _LVGL_LCD_CONFIG_H
#define _LVGL_LCD_CONFIG_H

#define LCD_COLORSPACE_R5G6B5
#define TYPE_UCOLOR r5g6b5_t

#if CONFIG_LVGL_USE_CUSTOM_DRIVER

/* lvgl include */
#include "iot_lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif
	
/* Display interface 
   Initialize your display*/
void lvgl_lcd_display_init();

/*
void lvgl_lcd_display_init() {
	/*Initialize lcd*/
    lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    
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
*/

#ifdef __cplusplus
}
#endif

#endif

#endif /* _LVGL_LCD_CONFIG_H */
