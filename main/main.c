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

// Task Routines
void sdcard_task( void * pvParameters );
// Task Handles
TaskHandle_t xSDwriter_handle = NULL;

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


  // Delaying for human interpretation
  ESP_LOGW(TAG, "Preparing to start routine, adjust lines/wires now");
  #define STARTUPDELAY 7
  for (int i = STARTUPDELAY; i >= 0; i--) {
    if (i == STARTUPDELAY) {
      printf("Starting in %d seconds...", i);
      fflush(stdout);
    } else { 
      printf("%i.", i);
      fflush(stdout);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Creating seperate task with larger stack to handle FatFS
  if(xSDwriter_handle == NULL){
    BaseType_t xReturned;
    xReturned = xTaskCreate(
      sdcard_task,
      "SD Card",
      16 * 1024, // Stack size in words not bytes
      NULL,
      tskIDLE_PRIORITY + 1,
      &xSDwriter_handle );
    if( xReturned == pdPASS){
      vTaskDelay( pdMS_TO_TICKS( 5000 ) );
    } else {
      ESP_LOGE(TAG, "Failed to create SD card task, error code: %i", xReturned);
    }
  }
}

void sdcard_task( void * pvParameters ) {
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

  // FatFS Configuration
  FATFS   filesystem;
  FIL     file;
  FRESULT fat_ret;              // Status feedback
  UINT    byteswritten;
  BYTE    workspace[FF_MAX_SS]; // Working area, used by f_mkfs
  #define CLUSTERSIZE 4 * 512 // Size of filesystem clusters/allocation size
  fat_ret = f_mkfs("1:", 0, CLUSTERSIZE, workspace, sizeof(workspace));
  if( fat_ret == FR_OK ) { ESP_LOGV(TAG, "Successfully created FatFS filesystem"); }
  else { ESP_LOGE(TAG, "Failed to create FatFS filesystem, error code: %i", fat_ret); }
  fat_ret = f_mount(&filesystem, "1:", 1);
  if( fat_ret == FR_OK ) { ESP_LOGV(TAG, "Successfully mounted FatFS filesystem"); }
  else { ESP_LOGE(TAG, "Failed to mount FatFS filesystem, error code: %i", fat_ret); }
  fat_ret = f_open(&file, "1:TEST.txt", FA_OPEN_APPEND | FA_WRITE | FA_READ);
  if( fat_ret == FR_OK ) { ESP_LOGV(TAG, "Successfully accessed FatFS files"); }
  else { ESP_LOGE(TAG, "Failed to access FatFS file, error code: %i", fat_ret); }
  //f_size()
  //f_expand()
  //f_lseek()
  //f_sync()
  //f_getfree()
  char *testData = "Testing 123\r\n";
  fat_ret = f_write(&file, testData, sizeof(testData), &byteswritten);
  if( fat_ret == FR_OK ) { ESP_LOGV(TAG, "Successfully wrote FatFS file"); }
  else { ESP_LOGE(TAG, "Failed to write FatFS file, error code: %i", fat_ret); }
  if( sizeof(testData) != byteswritten ){ ESP_LOGE(TAG, "Wrote incorrect amount"); }
  fat_ret = f_close(&file);
  if( fat_ret == FR_OK ) { ESP_LOGV(TAG, "Successfully closed FatFS file"); }
  else { ESP_LOGE(TAG, "Failed to close FatFS file, error code: %i", fat_ret); }

  // Delaying for human interpretation
  #define SHUTDOWNDELAY 25
  for (int i = SHUTDOWNDELAY; i >= 0; i--) {
    if (i == SHUTDOWNDELAY) {
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

  // We should never get here
  // This is a non thread/priority safe single task management
  vTaskDelay( pdMS_TO_TICKS( 1000 ) );
  xSDwriter_handle = NULL;
  vTaskDelete( NULL );  // Deleting Self 
}
