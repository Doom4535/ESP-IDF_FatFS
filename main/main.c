/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

// Supporting includes
#include "esp_log.h"
const char *TAG = "main.c";

// SD includes
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

// FatFS includes
#include "ff.h"

void app_main(void)
{
  printf("Hello world!\n");

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
    CONFIG_IDF_TARGET,
    chip_info.cores,
    (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  // Begin Card testing
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  //slot_config.width = 4;
  gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
  gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

  // Setup SD Interface
  esp_err_t esp_ret;
  sdmmc_card_t card;
  esp_ret = sdmmc_host_init(); 
  if( esp_ret == ESP_OK ) { ESP_LOGV(TAG, "Successfully initialized SDMMC host"); }
  else { ESP_LOGE(TAG, "Failed to initialize SDMMC host, error code: %s", esp_err_to_name(esp_ret)); }
  esp_ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot_config);
  if( esp_ret == ESP_OK ) { ESP_LOGV(TAG, "Successfully initialized SDMMC slot"); }
  else { ESP_LOGE(TAG, "Failed to initialize SDMMC slot, error code: %s", esp_err_to_name(esp_ret)); }
  esp_ret = sdmmc_card_init(&host, &card);
  if( esp_ret == ESP_OK ) { ESP_LOGV(TAG, "Successfully initialized SDMMC card"); }
  else { ESP_LOGE(TAG, "Failed to initialize SDMMC card, error code: %s", esp_err_to_name(esp_ret)); }

  // Delaying for human interpretation
  #define delayCounter 25
  for (int i = 25; i >= 0; i--) {
    if (i == delayCounter) {
      printf("Restarting in %d seconds...", i);
      fflush(stdout);
    } else { 
      printf("%i.", i);
      fflush(stdout);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  printf("Restarting now.\n");
  fflush(stdout);
  esp_restart();
}
