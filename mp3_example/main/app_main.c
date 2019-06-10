// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
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

/* component includes */
#include <stdio.h>

/* freertos includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_freertos_hooks.h"

#include "sdkconfig.h"

/* lvgl includes */
#include "iot_lvgl.h"

/* lvgl calibration includes */
#include "lv_calibration.h"

/* esp includes */
#include "esp_log.h"
#include "esp_err.h"
#include "ff.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

/* Param Include */
#include "iot_param.h"
#include "nvs_flash.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "fatfs_stream.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"

// #define LVGL_TEST_THEME
#define LVGL_EXAMPLE

#define CONTROL_CURRENT -1
#define CONTROL_NEXT    -2
#define CONTROL_PREV    -3

static TimerHandle_t lvgl_timer;
static TimerHandle_t lvgl_tick_timer;

static char directory[20][100];
static uint8_t filecount = 0;

#ifdef LVGL_EXAMPLE
static const char *TAG = "example_lvgl";
#endif

static audio_pipeline_handle_t pipeline;
static audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader;
static lv_obj_t *current_music;
static lv_obj_t *button[3];
static lv_obj_t *img[3];
static lv_obj_t *list_music[20];
void *img_src[] = {SYMBOL_PREV, SYMBOL_PLAY, SYMBOL_NEXT, SYMBOL_PAUSE};
        
//lv_task_handler should be called periodically around 10ms
static void IRAM_ATTR lvgl_task_time_callback(TimerHandle_t xTimer)
{
    lv_task_handler();
}

static void IRAM_ATTR lv_tick_task_callback(TimerHandle_t xTimer)
{
    lv_tick_inc(1);
}

static FILE *get_file(int next_file)
{
    static FILE *file;
    static int file_index = 1;

    if (next_file != CONTROL_CURRENT){
        if (next_file == CONTROL_NEXT){
            // advance to the next file
            if (++file_index > filecount - 1) {
                file_index = 0;
            }
        } else if (next_file == CONTROL_PREV){
            // advance to the prev file
            if (--file_index < 0) {
                file_index = filecount - 1;
            }
        } else if (next_file >=0 && next_file < filecount){
            file_index = next_file;
        }
        if (file != NULL) {
            fclose(file);
            file = NULL;
        }
        ESP_LOGI(TAG, "[ * ] File index %d", file_index);
    }
    // return a handle to the current file
    if (file == NULL) {
        lv_label_set_text(current_music, strstr(directory[file_index], "d/") + 2);
        file = fopen(directory[file_index], "r");
        if (!file) {
            ESP_LOGE(TAG, "Error opening file");
            return NULL;
        }
    }
    return file;
}

#ifdef LVGL_EXAMPLE
static lv_res_t audio_next_prev(lv_obj_t *obj)
{
    if (obj == button[0]) { // prev song
        ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
        // audio_pipeline_stop(pipeline);
        // audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);
        ESP_LOGI(TAG, "[ * ] Stopped, advancing to the prev song");
        get_file(CONTROL_PREV);
            // audio_element_set_uri(fatfs_stream_reader, directory[0]);
        audio_pipeline_run(pipeline);
        lv_img_set_src(img[1], img_src[3]);
    } else if (obj == button[1]) {

    } else if (obj == button[2]) { // next song
        ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
        // audio_pipeline_stop(pipeline);
        // audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);
        ESP_LOGI(TAG, "[ * ] Stopped, advancing to the next song");
            // audio_element_set_uri(fatfs_stream_reader, directory[1]);
        get_file(CONTROL_NEXT);
        audio_pipeline_run(pipeline);
        lv_img_set_src(img[1], img_src[3]);
    }
    ESP_LOGI(TAG, "Hello");
    return LV_RES_OK;
}

static lv_res_t audio_control(lv_obj_t *obj)
{
    audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
    switch (el_state) {
        case AEL_STATE_INIT :
            lv_img_set_src(img[1], img_src[3]);
            audio_pipeline_run(pipeline);
            break;
        case AEL_STATE_RUNNING :
            lv_img_set_src(img[1], img_src[1]);
            audio_pipeline_pause(pipeline);
            break;
        case AEL_STATE_PAUSED :
            lv_img_set_src(img[1], img_src[3]);
            audio_pipeline_resume(pipeline);
            break;
        default :
            break;
    }
    return LV_RES_OK;
}

//"Night theme\nAlien theme\nMaterial theme\nZen theme\nMono theme\nNemo theme"
static lv_res_t theme_change_action(lv_obj_t * roller)
{
    lv_theme_t *th;
    switch (lv_ddlist_get_selected(roller))
    {
        case 0:
            th = lv_theme_night_init(100, NULL);
            break;
    
        case 1:
            th = lv_theme_alien_init(100, NULL);
            break;
    
        case 2:
            th = lv_theme_material_init(100, NULL);
            break;
    
        case 3:
            th = lv_theme_zen_init(100, NULL);
            break;
    
        case 4:
            th = lv_theme_mono_init(100, NULL);
            break;
    
        case 5:
            th = lv_theme_nemo_init(100, NULL);
            break;
    
        default:
            th = lv_theme_default_init(100, NULL);
            break;
    }
    lv_theme_set_current(th);
    return LV_RES_OK;
}

static lv_res_t play_list(lv_obj_t *obj)
{
    for (uint8_t i = 0; i < 20; i++){
        if (obj == list_music[i]) {
            audio_pipeline_terminate(pipeline);
            get_file(i);
            audio_pipeline_run(pipeline);
            lv_img_set_src(img[1], img_src[3]);

            break;
        }
    }
    return LV_RES_OK;
}

static void littlevgl_demo(void)
{
    lv_obj_t *scr = lv_obj_create(NULL, NULL);
    lv_scr_load(scr);

    lv_theme_t *th = lv_theme_zen_init(100, NULL);
    // lv_theme_t *th = lv_theme_mono_init(100, NULL);
    lv_theme_set_current(th);

    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), NULL);

    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, SYMBOL_AUDIO);
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, SYMBOL_LIST);
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, SYMBOL_SETTINGS);
    lv_tabview_set_tab_act(tabview, 0, false);

    // lv_theme_t *th1 = lv_theme_get_current();
    // lv_style_t *cont_style = th1->cont;
    // cont_style->body.border.width = 0;
    // lv_obj_set_style(cont, &cont_style);

    lv_obj_t *cont = lv_cont_create(tab1, NULL);
    current_music = lv_label_create(cont, NULL);
    lv_label_set_long_mode(current_music, LV_LABEL_LONG_ROLL);
    lv_obj_set_width(current_music, 200); 
    lv_obj_align(current_music, cont, LV_ALIGN_IN_TOP_MID, 0, 20);      /*Align to center*/
    lv_label_set_text(current_music, strstr(directory[1], "d/") + 2);
    lv_obj_set_pos(current_music, 50, 20);

    lv_obj_set_size(cont, LV_HOR_RES - 20, LV_VER_RES - 85);
    lv_cont_set_fit(cont, false, false);
    for (uint8_t i = 0; i < 3; i++) {
        button[i] = lv_btn_create(cont, NULL);
        lv_obj_set_size(button[i], 50, 50);
        img[i] = lv_img_create(button[i], NULL);
        lv_img_set_src(img[i], img_src[i]);
    }
    lv_obj_align(button[0], cont, LV_ALIGN_IN_LEFT_MID, 35, 20);
    // lv_obj_set_pos(button[0], 35, 80);
    for (uint8_t i = 1; i < 3; i++) {
        lv_obj_align(button[i], button[i - 1], LV_ALIGN_OUT_RIGHT_MID, 40, 0);
    }
    lv_btn_set_action(button[0], LV_BTN_ACTION_CLICK, audio_next_prev);
    lv_btn_set_action(button[1], LV_BTN_ACTION_CLICK, audio_control);
    lv_btn_set_action(button[2], LV_BTN_ACTION_CLICK, audio_next_prev);

    lv_obj_t *list = lv_list_create(tab2, NULL);
    lv_obj_set_size(list, LV_HOR_RES - 20, LV_VER_RES - 85);
    for(uint8_t i = 0; i < filecount; i++) {
        list_music[i] = lv_list_add(list, SYMBOL_AUDIO, strstr(directory[i], "d/") + 2, play_list);
    }

    lv_obj_t *theme_label = lv_label_create(tab3, NULL);
    lv_label_set_text(theme_label, "Theme:");

    lv_obj_t *theme_roller = lv_roller_create(tab3, NULL);
    lv_obj_align(theme_roller, theme_label, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_roller_set_options(theme_roller, "Night theme\nAlien theme\nMaterial theme\nZen theme\nMono theme\nNemo theme");
    lv_roller_set_selected(theme_roller, 1, false);
    lv_roller_set_visible_row_count(theme_roller, 3);
    lv_ddlist_set_action(theme_roller, theme_change_action);
    ESP_LOGI("LvGL", "app_main last: %d", esp_get_free_heap_size());
}

static void user_task(void *pvParameter)
{
    while (1) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
#endif

// extern "C" {

    sdmmc_card_t* card;

    void sdmmc_init()
    {
        ESP_LOGI(TAG, "Initializing SD card");
        ESP_LOGI(TAG, "Using SDMMC peripheral");
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();

        // To use 1-line SD mode, uncomment the following line:
        // host.flags = SDMMC_HOST_FLAG_1BIT;

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        slot_config.gpio_cd = (gpio_num_t)34;

        // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
        // Internal pull-ups are not sufficient. However, enabling internal pull-ups
        // does make a difference some boards, so we do that here.
        gpio_set_pull_mode((gpio_num_t)15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
        gpio_set_pull_mode((gpio_num_t)2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
        gpio_set_pull_mode((gpio_num_t)4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
        gpio_set_pull_mode((gpio_num_t)12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
        gpio_set_pull_mode((gpio_num_t)13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

        // Options for mounting the filesystem.
        // If format_if_mount_failed is set to true, SD card will be partitioned and
        // formatted in case when mounting fails.
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
        };

        // Use settings defined above to initialize SD card and mount FAT filesystem.
        // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
        // Please check its source code and implement error recovery when developing
        // production applications.
        esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set format_if_mount_failed = true.");
            } else {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            }
            return;
        }

        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);
    }

    void read_content(const char* path, uint8_t *filecount)
    {
        char nextpath[100];
        struct dirent* de;
        DIR* dir = opendir(path);
        while (1) {
            de = readdir(dir);
            if(!de) {
                // printf("error\n");
                return;
            } else if (de->d_type == DT_REG) {
                // printf("f:%s\n",de->d_name);
                // if(strcasecmp(de->d_name, "hello.txt") == 0){
                //     printf("equal\n");
                // } else {
                //     printf("not equal\n");
                // }
                if (strstr(de->d_name, ".mp3") || strstr(de->d_name, ".MP3") ) {
                    sprintf(directory[*filecount], "%s/%s", path, de->d_name);
                    printf("%s\n", directory[*filecount]);
                    (*filecount)++;
                }
            } else if (de->d_type == DT_DIR) {
                // printf("%s\n",de->d_name);
                sprintf(nextpath, "%s/%s", path, de->d_name);
                // printf("d:%s\n",nextpath);
                // sprintf(directory[*filecount], "%s", de->d_name);
                // (*filecount)++;
                read_content(nextpath, filecount);
            }
        }
        closedir(dir);
    }

    /*
    To embed it in the app binary, the mp3 file is named
    in the component.mk COMPONENT_EMBED_TXTFILES variable.
    */

    extern const uint8_t adf_music_mp3_start[] asm("_binary_adf_music_mp3_start");
    extern const uint8_t adf_music_mp3_end[]   asm("_binary_adf_music_mp3_end");

    int mp3_music_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
    {
        static int mp3_pos;
        int read_size = adf_music_mp3_end - adf_music_mp3_start - mp3_pos;
        if (read_size == 0) {
            return AEL_IO_DONE;
        } else if (len < read_size) {
            read_size = len;
        }
        memcpy(buf, adf_music_mp3_start + mp3_pos, read_size);
        mp3_pos += read_size;
        return read_size;
    }

    void audio_embeeded_task(void *para)
    {
        audio_pipeline_handle_t pipeline;
        audio_element_handle_t i2s_stream_writer, mp3_decoder;

        esp_log_level_set("*", ESP_LOG_WARN);
        esp_log_level_set(TAG, ESP_LOG_INFO);

        ESP_LOGI(TAG, "[ 1 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
        audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        pipeline = audio_pipeline_init(&pipeline_cfg);
        mem_assert(pipeline);

        ESP_LOGI(TAG, "[1.1] Create mp3 decoder to decode mp3 file and set custom read callback");
        mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
        mp3_decoder = mp3_decoder_init(&mp3_cfg);
        audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);

        ESP_LOGI(TAG, "[1.2] Create i2s stream to write data to ESP32 internal DAC");
        i2s_stream_cfg_t i2s_cfg = I2S_STREAM_INTERNAL_DAC_CFG_DEFAULT();
        i2s_cfg.type = AUDIO_STREAM_WRITER;
        i2s_stream_writer = i2s_stream_init(&i2s_cfg);

        ESP_LOGI(TAG, "[1.3] Register all elements to audio pipeline");
        audio_pipeline_register(pipeline, mp3_decoder, "mp3");
        audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

        ESP_LOGI(TAG, "[1.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[ESP32 DAC]");
        audio_pipeline_link(pipeline, (const char *[]) {"mp3", "i2s"}, 2);

        ESP_LOGI(TAG, "[ 2 ] Setup event listener");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

        ESP_LOGI(TAG, "[2.1] Listening event from all elements of pipeline");
        audio_pipeline_set_listener(pipeline, evt);

        ESP_LOGI(TAG, "[ 3 ] Start audio_pipeline");
        audio_pipeline_run(pipeline);

        while (1) {
            audio_event_iface_msg_t msg;
            esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);

                ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                    music_info.sample_rates, music_info.bits, music_info.channels);

                audio_element_setinfo(i2s_stream_writer, &music_info);
                i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }
            /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
                    && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
                break;
            }
        }

        ESP_LOGI(TAG, "[ 4 ] Stop audio_pipeline");
        audio_pipeline_terminate(pipeline);

        /* Terminate the pipeline before removing the listener */
        audio_pipeline_remove_listener(pipeline);

        /* Make sure audio_pipeline_remove_listener is called before destroying event_iface */
        audio_event_iface_destroy(evt);

        /* Release all resources */
        audio_pipeline_deinit(pipeline);
        audio_element_deinit(i2s_stream_writer);
        audio_element_deinit(mp3_decoder);
        vTaskDelete(NULL);
    }

    /*
    * Callback function to feed audio data stream from sdcard to mp3 decoder element
    */
    static int my_sdcard_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
    {
        int read_len = fread(buf, 1, len, get_file(CONTROL_CURRENT));
        if (read_len == 0) {
            read_len = AEL_IO_DONE;
        }
        return read_len;
    }

    void audio_sdcard_task(void *para)
    {
        // esp_log_level_set("*", ESP_LOG_WARN);
        // esp_log_level_set(TAG, ESP_LOG_INFO);

        ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
        // Initialize peripherals management
        esp_periph_config_t periph_cfg = { 0 };
        esp_periph_init(&periph_cfg);

        // Initialize SD Card peripheral
        periph_sdcard_cfg_t sdcard_cfg = {
            .root = "/sdcard",
            .card_detect_pin = 34, //GPIO_NUM_34
        };
        esp_periph_handle_t sdcard_handle = periph_sdcard_init(&sdcard_cfg);
        // Start sdcard & button peripheral
        esp_periph_start(sdcard_handle);

        // Wait until sdcard was mounted
        while (!periph_sdcard_is_mounted(sdcard_handle)) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // ESP_LOGI(TAG, "[ 2 ] Start codec chip not use");
        // audio_hal_codec_config_t audio_hal_codec_cfg =  AUDIO_HAL_ES8388_DEFAULT();
        // audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
        // audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 0);
        // audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

        ESP_LOGI(TAG, "[3.0] Create audio pipeline for playback");
        audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        pipeline = audio_pipeline_init(&pipeline_cfg);
        mem_assert(pipeline);

        // ESP_LOGI(TAG, "[3.1] Create fatfs stream to read data from sdcard not use");
        // fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
        // fatfs_cfg.type = AUDIO_STREAM_READER;
        // fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

        // ESP_LOGI(TAG, "[3.2] Create i2s stream to write data to codec chip");
        ESP_LOGI(TAG, "[3.2] Create i2s stream to write data to ESP32 internal DAC");
        i2s_stream_cfg_t i2s_cfg = I2S_STREAM_INTERNAL_DAC_CFG_DEFAULT();
        i2s_cfg.type = AUDIO_STREAM_WRITER;
        i2s_stream_writer = i2s_stream_init(&i2s_cfg);

        ESP_LOGI(TAG, "[3.3] Create mp3 decoder to decode mp3 file");
        mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
        mp3_decoder = mp3_decoder_init(&mp3_cfg);
        audio_element_set_read_cb(mp3_decoder, my_sdcard_read_cb, NULL);

        ESP_LOGI(TAG, "[3.4] Register all elements to audio pipeline");
        // audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
        audio_pipeline_register(pipeline, mp3_decoder, "mp3");
        audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

        // ESP_LOGI(TAG, "[3.5] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->i2s_stream-->[ESP32 DAC]");
        // audio_pipeline_link(pipeline, (const char *[]) {"file", "mp3", "i2s"}, 3);
        ESP_LOGI(TAG, "[3.4] Link it together [my_sdcard_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
        audio_pipeline_link(pipeline, (const char *[]) {"mp3", "i2s"}, 2);

        // ESP_LOGI(TAG, "[3.6] Setup uri (file as fatfs_stream, mp3 as mp3 decoder, and default output is i2s)");
        // audio_element_set_uri(fatfs_stream_reader, "/sdcard/seeyou.mp3");

        ESP_LOGI(TAG, "[ 4 ] Setup event listener");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

        ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
        audio_pipeline_set_listener(pipeline, evt);

        ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
        audio_event_iface_set_listener(esp_periph_get_event_iface(), evt);


        // ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
        // audio_pipeline_run(pipeline);

        ESP_LOGI(TAG, "[ 6 ] Listen for all pipeline events");
        while (1) {
            audio_event_iface_msg_t msg;
            esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);

                ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                    music_info.sample_rates, music_info.bits, music_info.channels);

                audio_element_setinfo(i2s_stream_writer, &music_info);
                i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }

            // Advance to the next song when previous finishes
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED) {
                    ESP_LOGI(TAG, "[ * ] Finished, advancing to the next song");
                    audio_pipeline_stop(pipeline);
                    audio_pipeline_wait_for_stop(pipeline);
                    get_file(CONTROL_NEXT);
                    audio_pipeline_run(pipeline);
                }
                continue;
            }

            /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
                    && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
                ESP_LOGW(TAG, "[ * ] Stop event received");
                break;
            }
        }

        ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline");
        audio_pipeline_terminate(pipeline);

        /* Terminal the pipeline before removing the listener */
        audio_pipeline_remove_listener(pipeline);

        /* Stop all periph before removing the listener */
        esp_periph_stop_all();
        audio_event_iface_remove_listener(esp_periph_get_event_iface(), evt);

        /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
        audio_event_iface_destroy(evt);

        /* Release all resources */
        audio_pipeline_deinit(pipeline);
        // audio_element_deinit(fatfs_stream_reader);
        audio_element_deinit(i2s_stream_writer);
        audio_element_deinit(mp3_decoder);
        esp_periph_destroy();
        vTaskDelete(NULL);
    }

// }
/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
// extern "C" 
void app_main()
{
    ESP_LOGI("LvGL", "before init: %d", esp_get_free_heap_size());
    sdmmc_init();
    TaskHandle_t task;
    filecount = 0;
    read_content("/sdcard", &filecount);
    esp_vfs_fat_sdmmc_unmount();
    printf("%d\n", filecount);

    gpio_set_pull_mode((gpio_num_t)15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode((gpio_num_t)2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode((gpio_num_t)4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode((gpio_num_t)12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode((gpio_num_t)13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes
    xTaskCreate(audio_sdcard_task, "audio_sdcard_task", 1024*5, NULL, 4, NULL);

    /* Initialize LittlevGL */
    lv_init();

    /* Tick interface， Initialize a Timer for 1 ms period and in its interrupt call*/
    // esp_register_freertos_tick_hook(lv_tick_task_callback);
    lvgl_tick_timer = xTimerCreate(
                        "lv_tickinc_task",
                        1 / portTICK_PERIOD_MS, //period time
                        pdTRUE,                 //auto load
                        (void *)NULL,           //timer parameter
                        lv_tick_task_callback); //timer callback
    xTimerStart(lvgl_tick_timer, 0);

    /* Display interface */
    lvgl_lcd_display_init(); /*Initialize your display*/

    /* Input device interface */
    lv_indev_drv_t indevdrv = lvgl_indev_init(); /*Initialize your indev*/

    lvgl_timer = xTimerCreate(
                    "lv_task",
                    10 / portTICK_PERIOD_MS,  //period time
                    pdTRUE,                   //auto load
                    (void *)NULL,             //timer parameter
                    lvgl_task_time_callback); //timer callback
    xTimerStart(lvgl_timer, 0);

    vTaskDelay(20 / portTICK_PERIOD_MS);    // wait for execute lv_task_handler, avoid 'error'

    lvgl_calibrate_mouse(indevdrv, false);

#ifdef LVGL_TEST_THEME
    // lv_theme_t *th = lv_theme_alien_init(100, NULL);
    lv_theme_t *th = lv_theme_material_init(100, NULL);
    // lv_theme_t *th = lv_theme_mono_init(100, NULL);
    // lv_theme_t *th = lv_theme_nemo_init(100, NULL);
    // lv_theme_t *th = lv_theme_night_init(100, NULL);    
    // lv_theme_t *th = lv_theme_zen_init(100, NULL);    
    // lv_theme_t *th = lv_theme_night_init(100, NULL);    
    lv_test_theme_1(th);
    // lv_tutorial_animations();
    // lv_tutorial_styles();
    // lv_tutorial_responsive();
    // lv_tutorial_keyboard(NULL);
    // demo_create();
    // benchmark_create();
    // benchmark_start();
    // sysmon_create();
    // lv_obj_t * terminal = terminal_create();
    // terminal_add("123\n123\n456\n7890\n");
#else
    littlevgl_demo();
    ESP_LOGI("LvGL", "app_main last: %d", esp_get_free_heap_size());

    // xTaskCreate(
    //     user_task,   //Task Function
    //     "user_task", //Task Name
    //     1024,        //Stack Depth
    //     NULL,        //Parameters
    //     1,           //Priority
    //     NULL);       //Task Handler
#endif

    // xTaskCreate(audio_embeeded_task, "audio_embeeded_task", 1024*5, NULL, 4, NULL);

    ESP_LOGI("LvGL", "app_main last: %d", esp_get_free_heap_size());
}
