/*
    EvvGC-PLUS - Copyright (C) 2013-2015 Sarunas Vaitekonis

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/**
 * ATTENTION!!!
 * - EEPROM chip is accessible only if MPU6050 sensor is connected to the I2C bus.
 *   Otherwise pull-up resistors are missing on SDA and SCL lines, therefore
 *   communication with EEPROM chip is impossible.
 */

#include "hal.h"

#include "mpu6050.h"
#include "pwmio.h"
#include "telemetry.h"
#include "misc.h"
#include "attitude.h"

#include "flashconfig.h"

#include "flash/flash.h"
#include "storage.h"

/* C libraries: */
#include <string.h>

#define FLASH_PARAMS_PAGE	125

typedef struct tagDataStruct {
  PIDSettings pidSettings[3];       /*  9 bytes */
  PWMOutputStruct pwmOutput[3];     /* 12 bytes */
  MixedInputStruct mixedInput[3];   /* 21 byte  */
  InputModeStruct modeSettings[3];  /* 24 bytes */
  uint8_t sensorSettings[3];        /*  3 bytes */
  uint16_t cfSettings[2];           /*  4 bytes */
  float accelBias[3];               /* 12 bytes */
  float gyroBias[3];                /* 12 bytes */
  uint32_t crc32;                   /*  4 bytes */
/* TOTAL:                             101 bytes */

} __attribute__((packed)) ParamsDataStruct, *PParamsDataStruct;


/**
 * Local variables
 */
static ParamsDataStruct paramsData;


/**
 * @brief  Loads all user defined settings from external EEPROM chip.
 * @return 1 - if write operation is successful;
 *         0 - if write operation fails.
 */
uint8_t loadSettings(void) {

  memcpy((void *)&paramsData, (void *)FLASH_ADDRESS_OF_PAGE(FLASH_PARAMS_PAGE), sizeof(paramsData));

  if (paramsData.crc32 != crcCRC32((uint32_t *)&paramsData, sizeof(paramsData) / sizeof(uint32_t) - 1)) {
    /* Fill with default settings. */
    return saveSettings();
  } else {
    pidSettingsUpdate(paramsData.pidSettings);
    pwmOutputSettingsUpdate(paramsData.pwmOutput);
    mixedInputSettingsUpdate(paramsData.mixedInput);
    inputModeSettingsUpdate(paramsData.modeSettings);
    sensorSettingsUpdate(paramsData.sensorSettings);
    cfSettingsUpdate(paramsData.cfSettings);
    accelBiasUpdate(&g_IMU1, paramsData.accelBias);
    gyroBiasUpdate(&g_IMU1, paramsData.gyroBias);
  }

  return 1;
}

/**
 * @brief  Starts saving all user defined settings to external EEPROM chip.
 * @return 1 - if write operation is successful;
 *         0 - if write operation fails.
 */
uint8_t saveSettings(void) {
  memcpy((void *)paramsData.pidSettings, (void *)g_pidSettings, sizeof(g_pidSettings));
  memcpy((void *)paramsData.pwmOutput, (void *)g_pwmOutput, sizeof(g_pwmOutput));
  memcpy((void *)paramsData.mixedInput, (void *)g_mixedInput, sizeof(g_mixedInput));
  memcpy((void *)paramsData.modeSettings, (void *)g_modeSettings, sizeof(g_modeSettings));
  memcpy((void *)paramsData.sensorSettings, (void *)g_sensorSettings, sizeof(g_sensorSettings));
  memcpy((void *)paramsData.cfSettings, (void *)g_cfSettings, sizeof(g_cfSettings));
  memcpy((void *)paramsData.accelBias, (void *)g_IMU1.accelBias, sizeof(g_IMU1.accelBias));
  memcpy((void *)paramsData.gyroBias, (void *)g_IMU1.gyroBias, sizeof(g_IMU1.gyroBias));
  paramsData.crc32 = crcCRC32((uint32_t *)&paramsData, sizeof(paramsData) / sizeof(uint32_t) - 1);

  flashPageErase(FLASH_PARAMS_PAGE);
  return flashWrite((flashaddr_t) FLASH_ADDRESS_OF_PAGE(FLASH_PARAMS_PAGE), (char*) &paramsData, sizeof(paramsData));
}


