/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "lmic.h"

#include "screen_api.h"

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#include "esp_lcd_sh1107.h"
#else
#include "esp_lcd_panel_vendor.h"
#endif

static const char *TAG = "example";

#define I2C_HOST  0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA 4
#define EXAMPLE_PIN_NUM_SCL 15
#define EXAMPLE_PIN_NUM_RST 16
#define EXAMPLE_I2C_HW_ADDR 0x3C

// The pixel number in horizontal and vertical
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#define EXAMPLE_LCD_H_RES              64
#define EXAMPLE_LCD_V_RES              128
#endif
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

extern void example_lvgl_demo_ui(lv_disp_t *disp);

/* The LVGL port component calls esp_lcd_panel_draw_bitmap API for send data to the screen. There must be called
lvgl_port_flush_ready(disp) after each transaction to display. The best way is to use on_color_trans_done
callback from esp_lcd IO config structure. In IDF 5.1 and higher, it is solved inside LVGL port component. */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t * disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rst = 14,
    .dio = {26, 34, 35},
    .spi = {/* MISO = */ 19, /* MOSI = */ 27, /* SCK = */ 5},
    .rxtx = LMIC_UNUSED_PIN,
};

static const u1_t APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy(buf, APPEUI, 8);}

static const u1_t DEVEUI[8]={ 0xF9, 0x76, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui (u1_t* buf) { memcpy(buf, DEVEUI, 8);}

static const u1_t APPKEY[16] = { 0xB2, 0x70, 0x1C, 0x80, 0x4A, 0x94, 0x65, 0x3C, 0x0B, 0xA0, 0xE4, 0xAA, 0xA0, 0x58, 0xB0, 0x1D };
void os_getDevKey (u1_t* buf) { memcpy(buf, APPKEY, 16);}


// handle of send task
static TaskHandle_t send_task_handle = NULL;

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            ESP_LOGI(TAG, "EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            ESP_LOGI(TAG, "EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            ESP_LOGI(TAG, "EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            ESP_LOGI(TAG, "EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            ESP_LOGI(TAG, "EV_JOINING");
            screen_add_event("EV_JOINING");
            break;
        case EV_JOINED:
            ESP_LOGI(TAG, "EV_JOINED");
            screen_add_event("EV_JOINED");
            xTaskNotifyGive(send_task_handle);
            {
                ESP_LOGI(TAG, "netid: %ld", LMIC.netid);
                ESP_LOGI(TAG, "devaddr: %lx", LMIC.devaddr);
            //   u4_t netid = 0;
            //   devaddr_t devaddr = 0;
            //   u1_t nwkKey[16];
            //   u1_t artKey[16];
            //   LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
            //   ESP_LOGI(TAG, "netid: %ld", netid);
            //   ESP_LOGI(TAG, "devaddr: %ulx", devaddr);
            //   ESP_LOGI(TAG, "AppSKey: ");
            //   for (size_t i=0; i<sizeof(artKey); ++i) {
            //     if (i != 0)
            //       ESP_LOGI(TAG, "-");
            //     printHex2(artKey[i]);
            //   }
            //   ESP_LOGI(TAG, "");
            //   ESP_LOGI(TAG, "NwkSKey: ");
            //   for (size_t i=0; i<sizeof(nwkKey); ++i) {
            //           if (i != 0)
            //                   ESP_LOGI(TAG, "-");
            //           printHex2(nwkKey[i]);
            //   }
            //   ESP_LOGI(TAG, "");
            }
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            ESP_LOGI(TAG, "EV_JOIN_FAILED");
            screen_add_event("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            ESP_LOGI(TAG, "EV_REJOIN_FAILED");
            break;
        case EV_TXCOMPLETE:
            ESP_LOGI(TAG, "EV_TXCOMPLETE (includes waiting for RX windows)");
            if (LMIC.txrxFlags & TXRX_ACK)
              ESP_LOGI(TAG, "Received ack");
            if (LMIC.dataLen) {
              ESP_LOGI(TAG, "Received %d bytes of payload", LMIC.dataLen);
            }
            break;
        case EV_LOST_TSYNC:
            ESP_LOGI(TAG, "EV_LOST_TSYNC");
            break;
        case EV_RESET:
            ESP_LOGI(TAG, "EV_RESET");
            break;
        case EV_RXCOMPLETE:
            ESP_LOGI(TAG, "EV_RXCOMPLETE");
            screen_add_event("EV_RXCOMPLETE");
            LMIC.frame[LMIC.dataLen] = 0;
            ESP_LOGI(TAG, "Received %d bytes of payload string: %s", LMIC.dataLen, LMIC.frame);
            break;
        case EV_LINK_DEAD:
            ESP_LOGI(TAG, "EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            ESP_LOGI(TAG, "EV_LINK_ALIVE");
            break;

        default:
            ESP_LOGI(TAG, "Unknown event: %u", (unsigned) ev);
            break;
    }
}

// task send data every 20 seconds but wait to be joined
void send_task(void *pvParameter)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Send task started");
    const char mydata[] = "H, W!";
    while (1) {
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        if (LMIC.devaddr != 0) {
            LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        }
    }
}


void app_main(void)
{

    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
        .dc_bit_offset = 0,                     // According to SH1107 datasheet
        .flags =
        {
            .disable_control_phase = 1,
        }
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);
    /* Register done callback for IO */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    screen_initial_setup(disp);

    os_init();
    ESP_LOGI(TAG, "LMIC initialized");
    LMIC_reset();
    ESP_LOGI(TAG, "LMIC reset");
    LMIC_startJoining();
    ESP_LOGI(TAG, "LMIC start joining");

    xTaskCreate(send_task, "send_task", 2048, NULL, 1, &send_task_handle);
}
