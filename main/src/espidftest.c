#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "weather.h"
#include "buttontest.h"

void app_main() {
    //xTaskCreate(weather_run, "weather_run", 2048, NULL, 1, NULL);
    xTaskCreate(buttontest_lcd_task, "buttontest_lcd_task", 2048, NULL, 5, NULL);
    xTaskCreate(buttontest_button_task, "buttontest_button_task", 2048, NULL, 5, NULL);
}

