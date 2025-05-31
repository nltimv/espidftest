//
// Created by nltimv on 29-5-25.
//

#include "u8g2_esp32_hal_ai.h"
#include "u8g2.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"

#define TAG    "u8g2_hal"
#define I2C_PORT_NUM  I2C_NUM_0
#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define I2C_FREQ_HZ   400000

// This is our "byte" callback for hardware I2C:
uint8_t u8x8_byte_esp32_i2c(u8x8_t *u8x8,
                           uint8_t msg,
                           uint8_t arg_int,
                           void *arg_ptr)
{
  switch(msg) {
    case U8X8_MSG_BYTE_INIT:
      // configure and install the i2c driver
      {
        i2c_config_t conf = {
          .mode = I2C_MODE_MASTER,
          .sda_io_num = I2C_SDA_PIN,
          .scl_io_num = I2C_SCL_PIN,
          .sda_pullup_en = GPIO_PULLUP_ENABLE,
          .scl_pullup_en = GPIO_PULLUP_ENABLE,
          .master.clk_speed = I2C_FREQ_HZ
        };
        i2c_param_config(I2C_PORT_NUM, &conf);
        i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
      }
      break;

    case U8X8_MSG_BYTE_START_TRANSFER:
      // nothing special on ESP-IDF
      break;

    case U8X8_MSG_BYTE_SEND:
      {
        // arg_ptr points at the bytes to send, arg_int is the length
        uint8_t *data = (uint8_t*)arg_ptr;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        // 0x3C << 1 = 0x78 is the 8-bit I2C address of the SSD1306
        i2c_master_write_byte(cmd, (0x3C << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, data, arg_int, true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
      }
      break;

    case U8X8_MSG_BYTE_END_TRANSFER:
      // nothing to do
      break;

    default:
      return 0;
  }
  return 1;
}

// This is our "gpio_and_delay" callback:
uint8_t u8x8_gpio_and_delay_esp32(u8x8_t *u8x8,
                                  uint8_t msg,
                                  uint8_t arg_int,
                                  void *arg_ptr)
{
  switch(msg) {
    case U8X8_MSG_DELAY_MILLI:
      vTaskDelay(pdMS_TO_TICKS(arg_int));
      break;

    case U8X8_MSG_GPIO_RESET:
      // Reset line if you have one on e.g. GPIO-16
      gpio_set_level(16, arg_int);
      break;

    // we are using hardware I²C so do not handle SCL/SDA here
    default:
      break;
  }
  return 1;
}