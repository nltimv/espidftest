//
// Created by nltimv on 31-5-25.
//

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "u8g2.h"

#include "buttontest.h"

#include "esp_log.h"
#include "u8g2_esp32_hal.h"

#define TAG "buttontest"

// —— User-configurable pins ——
#define PIN_SDA   4
#define PIN_SCL   5

#define BTN1_GPIO                  39
#define BTN2_GPIO                  40
#define BTN3_GPIO                  41
#define BTN4_GPIO                  42

// Shared button state array
static bool button_state[4]      = { false };  // current state
static bool button_prev_state[4] = { false };  // last state

// U8g2 handle
static u8g2_t u8g2;

// Button scanning task
void buttontest_button_task(void *pvParameters)
{
    // configure inputs with pull-down
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL<<BTN1_GPIO) | (1ULL<<BTN2_GPIO) |
                        (1ULL<<BTN3_GPIO) | (1ULL<<BTN4_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);

    while (1) {
        // read each button
        bool new_state;
        for (int i = 0; i < 4; i++) {
            int gpio = (i==0 ? BTN1_GPIO :
                        i==1 ? BTN2_GPIO :
                        i==2 ? BTN3_GPIO :
                                BTN4_GPIO);
            new_state = gpio_get_level(gpio);

            // if changed, log it
            if (new_state != button_prev_state[i]) {
                button_state[i]      = new_state;
                button_prev_state[i] = new_state;
                ESP_LOGI(TAG, "BTN %d %s", i+1,
                         new_state ? "PRESSED" : "RELEASED");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Display update task
void buttontest_lcd_task(void *pvParameters)
{
    u8g2_hal_config_t u8g2_hal_config = U8G2_HAL_CONFIG_DEFAULT;
    u8g2_hal_config.i2c.scl = PIN_SCL;
    u8g2_hal_config.i2c.sda = PIN_SDA;
    u8g2_hal_init(&u8g2_hal_config);

    // initialize the display
    u8g2_Setup_st7567_i2c_jlx12864_f(
        &u8g2,
        U8G2_R2,
        u8g2_esp32_hal_i2c_byte_cb,
        u8g2_esp32_hal_gpio_and_delay_cb
    );
    // assign the I2C bus and address
    u8g2_SetI2CAddress(&u8g2, 0x3F<<1);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetContrast(&u8g2, 140);
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    while(1) {
        u8g2_ClearBuffer(&u8g2);

        for(int i = 0; i < 4; i++) {
            char line[32];
            snprintf(line, sizeof(line), "BTN %d: %s",
                     i+1,
                     button_state[i] ? "Pressed" : "Released");
            u8g2_DrawStr(&u8g2, 0, 12*(i+1), line);
        }

        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}