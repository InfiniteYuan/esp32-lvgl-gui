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

/* lvgl includes */
#include "iot_lvgl.h"

/* esp includes */
#include "esp_log.h"

/* Game include */
#include "game.h"

#define GRID_BODDER 5
static uint16_t num_matrix[5][5] = {0};
static uint32_t score_num = 0;
static lv_obj_t *lable[4][4];
static lv_obj_t *score_num_label;
static lv_obj_t *mbox1;
static int space[17][2];
static int tmp[5];
static bool over = false;

/*Declare the "source code image" which is stored in the flash*/
LV_IMG_DECLARE(background);
LV_IMG_DECLARE(esp32_logo);
LV_IMG_DECLARE(lvgl_logo);
LV_IMG_DECLARE(esp_logo);

void refresh_num()
{
    char num_string[5] = {0};
    for (uint8_t i = 0; i < 4; i++)
        for (uint8_t j = 0; j < 4; j++)
        {
            if (num_matrix[i + 1][j + 1] != 0)
                sprintf(num_string, "%4d", num_matrix[i + 1][j + 1]);
            else
                sprintf(num_string, " ");
            lv_label_set_text(lable[i][j], num_string);
        }
}

void draw_score_num()
{
    char score_num_string[5] = {0};
    sprintf(score_num_string, "%4d", score_num);
    lv_label_set_text(score_num_label, score_num_string);
}

void game_init_draw(uint8_t width)
{
    // Step 1: draw ground
    lv_obj_t *bg = lv_img_create(lv_scr_act(), NULL);
    // lv_img_set_src(bg, &background);
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_protect(bg, LV_PROTECT_POS);

    lv_obj_t *bg1 = lv_img_create(bg, NULL);
    lv_img_set_src(bg1, &esp32_logo);
    lv_obj_set_pos(bg1, 0, 320);
    lv_obj_set_size(bg1, LV_HOR_RES, LV_HOR_RES);
    lv_obj_set_protect(bg1, LV_PROTECT_POS);

    lv_obj_t *bg2 = lv_img_create(bg, NULL);
    lv_img_set_src(bg2, &esp_logo);
    lv_obj_set_pos(bg2, 0, 0);
    lv_obj_set_size(bg2, 480, 120);
    lv_obj_set_protect(bg2, LV_PROTECT_POS);

    lv_obj_t *bg3 = lv_img_create(bg, NULL);
    lv_img_set_src(bg3, &lvgl_logo);
    lv_obj_set_pos(bg3, 0, 120);
    lv_obj_set_size(bg3, 480, 90);
    lv_obj_set_protect(bg3, LV_PROTECT_POS);

    // Step 2: draw score
    /*Create an array for score*/
    static lv_style_t score_style;
    lv_style_copy(&score_style, &lv_style_plain);
    score_style.text.font = &lv_font_dejavu_40; /*Unicode and symbol fonts already assigned by the library*/
    score_style.text.color = LV_COLOR_MAKE(0xFF, 0x00, 0x00);

    lv_obj_t *score_label = lv_label_create(bg, NULL);
    score_num_label = lv_label_create(bg, NULL);
    lv_label_set_long_mode(score_label, LV_LABEL_LONG_EXPAND);
    lv_label_set_long_mode(score_num_label, LV_LABEL_LONG_EXPAND);
    lv_label_set_style(score_label, &score_style);
    lv_label_set_style(score_num_label, &score_style);
    lv_label_set_text(score_label, "Score: ");
    lv_label_set_text(score_num_label, "0");
    lv_obj_align(score_label, bg, LV_ALIGN_IN_TOP_MID, 0, 250);
    lv_obj_align(score_num_label, score_label, LV_ALIGN_OUT_RIGHT_MID, 30, 0);

    // Step 3: draw grid
    /*Create an array for the points of the line*/
    uint8_t cell_width = (width - 2 * GRID_BODDER) / 4;

    static lv_point_t raw_line1_points[] = {{5, 0}, {480 - 5, 0}};
    static lv_point_t column_line1_points[] = {{5, 0}, {5, 4 * 117}};

    /*Create new style for line*/
    static lv_style_t style_line;
    lv_style_copy(&style_line, &lv_style_plain);
    style_line.line.color = LV_COLOR_MAKE(0x5d, 0x5d, 0x5d);
    style_line.line.width = 5;

    lv_obj_t *raw_line[5];
    raw_line[0] = lv_line_create(bg, NULL);
    lv_line_set_points(raw_line[0], raw_line1_points, 2); /*Set the points*/
    lv_line_set_style(raw_line[0], &style_line);
    lv_obj_align(raw_line[0], bg, LV_ALIGN_IN_TOP_LEFT, 0, 800 - 5 - 4 * 117);
    for (uint8_t i = 1; i < 5; i++)
    {
        raw_line[i] = lv_line_create(bg, raw_line[i - 1]);
        lv_obj_align(raw_line[i], raw_line[i - 1], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 113);
        lv_line_set_style(raw_line[i], &style_line);
    }

    lv_obj_t *column_line[5];
    column_line[0] = lv_line_create(bg, NULL);
    lv_line_set_points(column_line[0], column_line1_points, 2); /*Set the points*/
    lv_line_set_style(column_line[0], &style_line);
    lv_obj_align(column_line[0], bg, LV_ALIGN_IN_TOP_LEFT, 0, 800 - 5 - 4 * 117);
    for (uint8_t i = 1; i < 5; i++)
    {
        column_line[i] = lv_line_create(bg, column_line[i - 1]);
        lv_obj_align(column_line[i], column_line[i - 1], LV_ALIGN_OUT_RIGHT_TOP, 108, 0);
        lv_line_set_style(column_line[i], &style_line);
    }

    // Step 4: draw cell
    /*Create an array for cell*/
    static lv_style_t font_style;
    lv_style_copy(&font_style, &lv_style_plain);
    font_style.text.font = &lv_font_dejavu_40; /*Unicode and symbol fonts already assigned by the library*/
    font_style.text.color = LV_COLOR_MAKE(0xFF, 0x00, 0x00);

    for (uint8_t i = 0; i < 4; i++)
        for (uint8_t j = 0; j < 4; j++)
        {
            lable[i][j] = lv_label_create(bg, NULL);
            lv_label_set_text(lable[i][j], " ");
            lv_label_set_style(lable[i][j], &font_style);
            lv_label_set_long_mode(lable[i][j], LV_LABEL_LONG_BREAK); /*Break the long lines*/
            lv_obj_set_size(lable[i][j], 110, 110);
            lv_obj_align(lable[i][j], raw_line[i], LV_ALIGN_OUT_BOTTOM_LEFT, j * 112 + 5, 30);
            lv_label_set_align(lable[i][j], LV_LABEL_ALIGN_CENTER);
        }

    // Step 5: num init
    num_matrix[esp_random() % 4 + 1][esp_random() % 4 + 1] = 2;
    num_matrix[esp_random() % 4 + 1][esp_random() % 4 + 1] = 2;

    // Step 6: init grid
    refresh_num();
}

void game_init(uint16_t width)
{
    game_init_draw(width);
}

/*Called when a button is clicked*/
static lv_res_t mbox_apply_action(lv_obj_t *mbox, const char *txt)
{
    if (strcmp(txt, "Replay") == 0)
    {
        memset(num_matrix, 0, 25 * 2);
        memset(space, 0, 34);
        score_num = 0;
        game_init(480);
        over = false;
    }
    else if (strcmp(txt, "Close") == 0)
    {
        lv_obj_del(mbox1);
    }
    return LV_RES_OK; /*Return OK if the message box is not deleted*/
}

void game_over(void)
{
    over = true;

    static lv_style_t score_style;
    lv_style_copy(&score_style, &lv_style_scr);
    score_style.text.font = &lv_font_dejavu_40; /*Unicode and symbol fonts already assigned by the library*/
    score_style.text.color = LV_COLOR_MAKE(0x00, 0x00, 0x00);
    char score_num_string[30] = {0};
    sprintf(score_num_string, "Game Over\n Your score: %4d", score_num);
    mbox1 = lv_mbox_create(lv_scr_act(), NULL);
    // lv_mbox_set_style(mbox1, LV_MBOX_STYLE_BG, &score_style);
    lv_mbox_set_text(mbox1, score_num_string); /*Set the text*/

    /*Add two buttons*/
    static const char *btns[] = {"\221Replay", "\221Close", ""}; /*Button description. '\221' lv_btnm like control char*/
    lv_mbox_add_btns(mbox1, btns, mbox_apply_action);
    lv_obj_set_size(mbox1, 300, 100);
    lv_obj_align(mbox1, NULL, LV_ALIGN_IN_TOP_LEFT, 90, 300); /*Align to the corner*/
    while (over)
    {
        vTaskDelay(20);
    }
}
void gen_num(void)
{
    uint8_t x, y;
    uint8_t i, j, k, t;
    uint16_t nax, nin;
    k = 0;
    nax = 0;
    nin = 2049;
    for (i = 1; i <= 4; ++i)
    {
        for (j = 1; j <= 4; ++j)
        {
            if (num_matrix[i][j] > nax)
                nax = num_matrix[i][j];
            if (num_matrix[i][j] < nin)
                nin = num_matrix[i][j];
            if (num_matrix[i][j] == 0)
            {
                k++;
                space[k][0] = i;
                space[k][1] = j;
            }
        }
    }
    if (k == 0)
    {
        t = 1;
        for (i = 1; i <= 4; ++i)
            for (j = 1; j <= 3; ++j)
                if (num_matrix[i][j] == num_matrix[i][j + 1])
                    t = 2;
        for (i = 1; i <= 3; ++i)
            for (j = 1; j <= 4; ++j)
                if (num_matrix[i][j] == num_matrix[i + 1][j])
                    t = 2;
        if (t == 1)
            game_over();
        return;
    }

    i = rand() % k + 1;
    x = space[i][0];
    y = space[i][1];

    j = rand() % 100 + 1;
    if (j <= 85)
    {
        num_matrix[x][y] = 2;
        // draw_new_num(x, y, 2);
    }
    else if (j <= 95)
    {
        num_matrix[x][y] = 4;
        // draw_new_num(x, y, 4);
    }
    else if (j <= 100)
    {
        num_matrix[x][y] = 8;
        // draw_new_num(x, y, 8);
    }
    refresh_num();
}

void move_up()
{
    int i, j, k, t, nx = -1, ny = 0, nn = 0;
    for (i = 1; i <= 4; ++i)
        tmp[i] = 0;
    for (j = 1; j <= 4; ++j)
    {
        k = 0;
        for (i = 1; i <= 4; ++i)
        {
            if (num_matrix[i][j])
            {
                tmp[++k] = num_matrix[i][j];
                num_matrix[i][j] = 0;
            }
        }
        t = 1;
        while (t <= k - 1)
        {
            if (tmp[t] == tmp[t + 1])
            {
                tmp[t] *= 2;
                tmp[t + 1] = 0;
                score_num += tmp[t];
                if (nx == -1)
                {
                    nx = t;
                    ny = j;
                    nn = tmp[t];
                }
                t += 2;
            }
            else
                t++;
        }
        t = 1;
        for (i = 1; i <= k; ++i)
            if (tmp[i])
                num_matrix[t++][j] = tmp[i];
    }
    // for (i = 1; i <= 4; ++i)
    //     for (j = 1; j <= 4; ++j)
    //         draw_num(i, j, num_matrix[i][j]);
    // if (nx != -1)
    //     draw_sum_num(nx, ny, tmp[nx]);
    draw_score_num(score_num);
    gen_num();
}

void move_down()
{
    int i, j, k, t, nx = -1, ny = 0, nn = 0;
    for (i = 1; i <= 4; ++i)
        tmp[i] = 0;
    for (j = 1; j <= 4; ++j)
    {
        k = 0;
        for (i = 4; i >= 1; --i)
        {
            if (num_matrix[i][j])
            {
                tmp[++k] = num_matrix[i][j];
                num_matrix[i][j] = 0;
            }
        }
        t = 1;
        while (t <= k - 1)
        {
            if (tmp[t] == tmp[t + 1])
            {
                tmp[t] *= 2;
                tmp[t + 1] = 0;
                score_num += tmp[t];
                if (nx == -1)
                {
                    nx = 4 - t;
                    ny = j;
                    nn = tmp[t];
                }
                t += 2;
            }
            else
                t++;
        }
        t = 4;
        for (i = 1; i <= k; ++i)
            if (tmp[i])
                num_matrix[t--][j] = tmp[i];
    }
    // for (i = 1; i <= 4; ++i)
    //     for (j = 1; j <= 4; ++j)
    //         draw_num(i, j, num_matrix[i][j]);
    // if (nx != -1)
    //     draw_sum_num(nx, ny, nn);
    draw_score_num(score_num);
    gen_num();
}

void move_left()
{
    int i, j, k, t, nx = -1, ny = 0, nn = 0;
    for (i = 1; i <= 4; ++i)
        tmp[i] = 0;
    for (i = 1; i <= 4; ++i)
    {
        k = 0;
        for (j = 1; j <= 4; ++j)
        {
            if (num_matrix[i][j])
            {
                tmp[++k] = num_matrix[i][j];
                num_matrix[i][j] = 0;
            }
        }
        t = 1;
        while (t <= k - 1)
        {
            if (tmp[t] == tmp[t + 1])
            {
                tmp[t] *= 2;
                tmp[t + 1] = 0;
                score_num += tmp[t];
                if (nx == -1)
                {
                    nx = i;
                    ny = t;
                    nn = tmp[t];
                }
                t += 2;
            }
            else
                t++;
        }
        t = 1;
        for (j = 1; j <= k; ++j)
            if (tmp[j])
                num_matrix[i][t++] = tmp[j];
    }
    // for (i = 1; i <= 4; ++i)
    //     for (j = 1; j <= 4; ++j)
    //         draw_num(i, j, num_matrix[i][j]);
    // if (nx != -1)
    //     draw_sum_num(nx, ny, nn);
    draw_score_num(score_num);
    gen_num();
}

void move_right()
{
    int i, j, k, t, nx = -1, ny = 0, nn = 0;
    for (i = 1; i <= 4; ++i)
        tmp[i] = 0;
    for (i = 1; i <= 4; ++i)
    {
        k = 0;
        for (j = 4; j >= 1; --j)
        {
            if (num_matrix[i][j])
            {
                tmp[++k] = num_matrix[i][j];
                num_matrix[i][j] = 0;
            }
        }
        t = 1;
        while (t <= k - 1)
        {
            if (tmp[t] == tmp[t + 1])
            {
                tmp[t] *= 2;
                tmp[t + 1] = 0;
                score_num += tmp[t];
                if (nx == -1)
                {
                    nx = i;
                    ny = 4 - t;
                    nn = tmp[t];
                }
                t += 2;
            }
            else
                t++;
        }
        t = 4;
        for (j = 1; j <= k; ++j)
            if (tmp[j])
                num_matrix[i][t--] = tmp[j];
    }
    // for (i = 1; i <= 4; ++i)
    //     for (j = 1; j <= 4; ++j)
    //         draw_num(i, j, num_matrix[i][j]);
    // if (nx != -1)
    //     draw_sum_num(nx, ny, nn);
    draw_score_num(score_num);
    gen_num();
}
