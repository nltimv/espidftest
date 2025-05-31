//
// Created by nltimv on 31-5-25.
//
/* weather.c
   Converted to plain C from an Arduino/C++ example
   Uses u8g2’s C API: https://github.com/olikraus/u8g2
*/
#include <string.h>
#include <u8g2.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <u8g2_esp32_hal.h>
#include "weather.h"

#define PIN_SDA   21
#define PIN_SCL   22

static const char *TAG = "weather";
static uint16_t count = 1;

/* Replace this with your platform's delay function */
static void delay_ms(uint32_t ms) {
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

/*---------------------------------------------------------------------------*/
/* define your display constructor here:                                     */
/*  e.g. for HW‐I2C on a ST7567 128×64 you might use:                        */
/*---------------------------------------------------------------------------*/
static u8g2_t u8g2;
static u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
/* u8g2_Setup_ST7567_JLX12864_F_HW_I2C is a macro from u8g2.h */
void init_display(void) {
  ESP_LOGI(TAG, "Initializing display");
  u8g2_esp32_hal.bus.i2c.sda = PIN_SDA;
  u8g2_esp32_hal.bus.i2c.scl = PIN_SCL;
  u8g2_esp32_hal_init(u8g2_esp32_hal);

  /* rotation = U8G2_R2, I²C address 0x3F<<1 */
  u8g2_Setup_st7567_i2c_jlx12864_f(
    &u8g2,
    U8G2_R2,
    u8g2_esp32_i2c_byte_cb,
    u8g2_esp32_gpio_and_delay_cb);

  u8g2_SetI2CAddress(&u8g2, 0x3F << 1);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);      /* wake up display */
  u8g2_SetContrast(&u8g2, 150);
  ESP_LOGI(TAG, "Display initialized");
}

/*---------------------------------------------------------------------------*/
/* Symbols:                                                                 */
/*---------------------------------------------------------------------------*/
#define SUN        0
#define SUN_CLOUD  1
#define CLOUD      2
#define RAIN       3
#define THUNDER    4

/*---------------------------------------------------------------------------*/
/* Draw one of the iconic weather symbols at (x,y)                           */
/*---------------------------------------------------------------------------*/
static void drawWeatherSymbol(int x, int y, uint8_t symbol) {
  switch(symbol) {
    case SUN:
      u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_6x_t);
      u8g2_DrawGlyph(&u8g2, x, y, 69);
      break;
    case SUN_CLOUD:
      u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_6x_t);
      u8g2_DrawGlyph(&u8g2, x, y, 65);
      break;
    case CLOUD:
      u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_6x_t);
      u8g2_DrawGlyph(&u8g2, x, y, 64);
      break;
    case RAIN:
      u8g2_SetFont(&u8g2, u8g2_font_open_iconic_weather_6x_t);
      u8g2_DrawGlyph(&u8g2, x, y, 67);
      break;
    case THUNDER:
      u8g2_SetFont(&u8g2, u8g2_font_open_iconic_embedded_6x_t);
      u8g2_DrawGlyph(&u8g2, x, y, 67);
      break;
    default:
      break;
  }
}

/*---------------------------------------------------------------------------*/
/* Draw the big weather icon + temperature                                  */
/*---------------------------------------------------------------------------*/
static void drawWeather(int symbol) {
  /* icon */
  drawWeatherSymbol(0, 48, symbol);
  /* numeric temperature */
  u8g2_SetFont(&u8g2, u8g2_font_logisoso32_tf);
  //u8g2_SetCursor(&u8g2, 48 + 3, 42);
  /* print number and °C */

  char buf[8];
  snprintf(buf, sizeof(buf), "%d", count++);
  u8g2_DrawUTF8(&u8g2, 48 + 3, 42, buf);

}

/*---------------------------------------------------------------------------*/
/* Scroll a single‐line ASCII string in the bottom area                     */
/* offset: pixel offset from left (can be negative)                         */
/*---------------------------------------------------------------------------*/
static void drawScrollString(int16_t offset, const char *s) {
  static char buf[64];
  const size_t w = u8g2_GetDisplayWidth(&u8g2);
  size_t len = strlen(s);
  size_t char_off, visible;
  int16_t dx;

  /* erase the scroll‐area (y=49..63) */
  u8g2_SetDrawColor(&u8g2, 0);
  u8g2_DrawBox(&u8g2, 0, 49, w, u8g2_GetDisplayHeight(&u8g2) - 49);
  u8g2_SetDrawColor(&u8g2, 1);

  if (offset < 0) {
    char_off = (-offset) / 8;
    dx = offset + char_off * 8;
    if (char_off >= w/8) return;
    visible = w/8 + 1 - char_off;
    if (visible > len) visible = len;
    memcpy(buf, s, visible);
    buf[visible] = 0;
    u8g2_SetFont(&u8g2, u8g2_font_8x13_mf);
    u8g2_DrawStr(&u8g2, char_off*8 - dx, 62, buf);
  }
  else {
    char_off = offset / 8;
    if (char_off >= len) return;
    dx = offset - char_off * 8;
    visible = len - char_off;
    if (visible > w/8 + 1) visible = w/8 + 1;
    memcpy(buf, s + char_off, visible);
    buf[visible] = 0;
    u8g2_SetFont(&u8g2, u8g2_font_8x13_mf);
    u8g2_DrawStr(&u8g2, -dx, 62, buf);
  }
}

/*---------------------------------------------------------------------------*/
/* A single draw‐and‐scroll routine                                         */
/*---------------------------------------------------------------------------*/
static void draw(const char *s, uint8_t symbol) {
  ESP_LOGI(TAG, "Symbol %d, %d degrees: %s", symbol, count, s);
  size_t len = strlen(s);
  int16_t offset = - (int16_t)u8g2_GetDisplayWidth(&u8g2);

  /* full buffer clear */
  u8g2_ClearBuffer(&u8g2);

  /* draw static parts */
  drawWeather(symbol);

  /* scroll the text */
  while (1) {
    drawScrollString(offset, s);
    u8g2_SendBuffer(&u8g2);
    //delay_ms(20);
    offset += 2;
    /* when text has fully scrolled out of sight, stop */
    if (offset > (int16_t)(len*8 + 1)) break;
  }
}

/*---------------------------------------------------------------------------*/
/* main (or your platform’s init + loop)                                    */
/*---------------------------------------------------------------------------*/
void weather_run() {
  init_display();

  for (;;) {
    draw("What a beautiful day!",   SUN);
    draw("The sun's come out!",     SUN_CLOUD);
    draw("It's raining cats and dogs.", RAIN);
    draw("That sounds like thunder.", THUNDER);
    draw("It's stopped raining",     CLOUD);
  }

}