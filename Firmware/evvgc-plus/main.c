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

#include "hal.h"
#include "board.h"
#include "usbcfg.h"
#include "mpu6050.h"
#include "attitude.h"
#include "pwmio.h"
#include "misc.h"
#include "telemetry.h"
#include "mavlink_handler.h"
#include "storage.h"
#include "shell.h"
#include "test.h"

/* Telemetry operation time out in milliseconds. */
#define TELEMETRY_SLEEP_MS      20

#define MPU6050_LOW_DETECTED    0x00000001
#define EEPROM_24C02_DETECTED   0x00000004

#define WARM_UP_COUNTER_MAX     0x00000BB8

#define STREAM_BUFFER_SIZE      0x20

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)


/**
 * Global variables
 */
/* Status of the board. */
uint32_t g_boardStatus = 0;
/* Main thread termination flag. */
bool g_runMain = TRUE;
/* I2C error info structure. */
I2CErrorStruct g_i2cErrorInfo = {0, 0};
/* Stream data id. */
uint8_t g_streamDataID = 0;
/* Data streaming index. */
uint8_t g_streamIdx = 0;
/* LED B flash req */
bool led_b = false;

/**
 * Local variables
 */
/* I2C2 configuration for I2C driver 2 */
static const I2CConfig i2cfg_d2 = {
  OPMODE_I2C,
  //200000,
  //FAST_DUTY_CYCLE_16_9,
      400000,
    FAST_DUTY_CYCLE_2,
};







#if 1
static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreGetStatusX());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}
#endif

#if 1
static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;

  /*
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%08lx %08lx %4lu %4lu %9s %lu\r\n",
             (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
             (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
             states[tp->p_state]);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
  */
}
#endif

#if 1
static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: test\r\n");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriorityX(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}
#endif 

#if 1

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
  static uint8_t buf[] =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: write\r\n");
    return;
  }

  while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
#if 1
    /* Writing in channel mode.*/
    chnWrite(&SDU1, buf, sizeof buf - 1);
#else
    /* Writing in buffer mode.*/
    (void) obqGetEmptyBufferTimeout(&SDU1.obqueue, TIME_INFINITE);
    memcpy(SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
    obqPostFullBuffer(&SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
#endif
  }
  chprintf(chp, "\r\n\nstopped\r\n");
}



static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},
  {"write", cmd_write},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

#endif

/* Virtual serial port over USB. */
//SerialUSBDriver SDU1;

#if 1
/* Binary semaphore indicating that new data is ready to be processed. */
static binary_semaphore_t bsemIMU1DataReady;
static binary_semaphore_t bsemStreamReady;



/* Data streaming buffer. */
static float dataStream[STREAM_BUFFER_SIZE];
/* Pointer to low or high part of the data streaming buffer. */
float *pStream = NULL;

static void streamUpdateData(PIMUStruct pIMU) {
  switch (g_streamDataID) {
  case 1: /* Accel X; */
    dataStream[g_streamIdx++] = pIMU->accelData[0];
    break;
  case 2: /* Accel Y; */
    dataStream[g_streamIdx++] = pIMU->accelData[1];
    break;
  case 3: /* Accel Z; */
    dataStream[g_streamIdx++] = pIMU->accelData[2];
    break;
  case 4: /* Gyro X;  */
    dataStream[g_streamIdx++] = pIMU->gyroData[0];
    break;
  case 5: /* Gyro Y;  */
    dataStream[g_streamIdx++] = pIMU->gyroData[1];
    break;
  case 6: /* Gyro Z;  */
    dataStream[g_streamIdx++] = pIMU->gyroData[2];
    break;
  case 7: /* Atti X;  */
    dataStream[g_streamIdx++] = pIMU->rpy[0];
    break;
  case 8: /* Atti Y;  */
    dataStream[g_streamIdx++] = pIMU->rpy[1];
    break;
  case 9: /* Atti Z;  */
    dataStream[g_streamIdx++] = pIMU->rpy[2];
    break;
  default:
    g_streamIdx = 0;
    g_streamDataID = 0;
  }

  if (g_streamIdx == (STREAM_BUFFER_SIZE / 2)) {
    pStream = &dataStream[0];
    chBSemSignal(&bsemStreamReady);
  }

  if (g_streamIdx == STREAM_BUFFER_SIZE) {
    g_streamIdx = 0;
    pStream = &dataStream[STREAM_BUFFER_SIZE / 2];
    chBSemSignal(&bsemStreamReady);
  }
}
#endif

/**
 * Green LED blinker thread. Times are in milliseconds.
 */
static THD_WORKING_AREA(waBlinkerThread_A, 64);
static THD_FUNCTION(BlinkerThread_A,arg) {
  (void)arg;
  //while (!chThdShouldTerminateX()) {
  while (true) {
    systime_t time;
#if 0
    if (g_boardStatus & IMU_CALIBRATION_MASK) {
      time = 50;
    } else {
      time = serusbcfg.usbp->state == USB_ACTIVE ? 250 : 500;
    }
#endif
    time = 1000;//ms
    palToggleLedGreen();

    // serial UART:
    //chprintf((BaseSequentialStream *)&SD4, "wo\n");
    //chprintf((BaseSequentialStream *)&SDU1, "di\n");
     //sdWriteTimeout(&SDU1, "000000000", 9, MS2ST(100) );
    //sdWrite(&SDU1, (uint8_t *)"000000000", 9);
    const uint8_t constHexToSend = 0x55;

    //chnWriteTimeout(&SDU1, (uint8_t *)&constHexToSend, 1, TIME_INFINITE);
    //chnWriteTimeout(&SDU1, (uint8_t *)&constHexToSend, 1, TIME_IMMEDIATE);
    //chnWriteTimeout(&SDU1, (uint8_t *)"i\r\n", 3, TIME_IMMEDIATE);
    //chnWriteTimeout(&SD4, (uint8_t *)"i\r\n", 3, TIME_IMMEDIATE);
    
    chThdSleepMilliseconds(time);
  }
  /* This point may be reached if shut down is requested. */
  chThdExit(MSG_OK);
}

/**
 * Red LED blinker thread. Times are in milliseconds.
 */
static THD_WORKING_AREA(waBlinkerThread_B, 64);
static THD_FUNCTION(BlinkerThread_B,arg) {
  (void)arg;
  systime_t time = 20;
  time = 2000; //ms
  while (!chThdShouldTerminateX()) {
    if (led_b) {
    	palToggleLedRed();

    }
    else {
    	palToggleLedRed();
    }
    chThdSleepMilliseconds(time);
  }
  /* This point may be reached if shut down is requested. */
  chThdExit(MSG_OK);
}

#if 1
/**
 * MPU6050 data polling thread. Times are in milliseconds.
 * This thread requests a new data from MPU6050 every 1.5 ms (@666 Hz).
 */
static THD_WORKING_AREA(waPollMPU6050Thread, 128);
static THD_FUNCTION(PollMPU6050Thread,arg) {
  systime_t time;
  uint32_t warmUp = 0;
  (void)arg;
  chprintf((BaseSequentialStream *)&SD4, "i2ccccc\n");
  time = chVTGetSystemTime();
  do {
    if (!mpu6050GetNewData(&g_IMU1)) {
      /* Restart I2C2 bus in case of an error. */
      chprintf((BaseSequentialStream *)&SD4, " restart i2c\n");
      i2cStop(&I2CD2);
      i2cStart(&I2CD2, &i2cfg_d2);
    //  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN); //SCL
    //palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN); //SDA
      palSetPadMode(GPIOB, 10, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);   /* SCL */
palSetPadMode(GPIOB,11, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);   /* SDA */

    }
    /* Wait until the next 1.5 milliseconds passes. */
    chThdSleepUntil(time += US2ST(1500));
  } while (warmUp++ < WARM_UP_COUNTER_MAX);

  while (!chThdShouldTerminateX()) {
    if (mpu6050GetNewData(&g_IMU1)) {
      chBSemSignal(&bsemIMU1DataReady);
    } else {
      /* Restart I2C2 bus in case of an error. */
      i2cStop(&I2CD2);
      i2cStart(&I2CD2, &i2cfg_d2);
    }
    /* Wait until the next 1.5 milliseconds passes. */
    chThdSleepUntil(time += US2ST(1500));
  }
  /* This point may be reached if shut down is requested. */
  chThdExit(MSG_OK);
}
#endif


/**
 * Attitude calculation thread.
 * - This thread works in conjunction with PollMPU6050Thread thread.
 * - This thread is synchronized by PollMPU6050Thread thread.
 * - This thread has the highest priority level.
 */
#if 1
static THD_WORKING_AREA(waAttitudeThread, 2048);
static THD_FUNCTION(AttitudeThread,arg) {
  (void)arg;
  attitudeInit();
  while (!chThdShouldTerminateX()) {
    /* Process IMU1 new data ready event. */
    if (chBSemWait(&bsemIMU1DataReady) == MSG_OK) {
      if (g_boardStatus & IMU1_CALIBRATION_MASK) {
        if (imuCalibrate(&g_IMU1, g_boardStatus & IMU1_CALIBRATE_ACCEL)) {
          g_boardStatus &= ~IMU1_CALIBRATION_MASK;
        }
      } else {
        attitudeUpdate(&g_IMU1);
      }
    }

    if (g_streamDataID) {
      streamUpdateData(&g_IMU1);
    }

    if (g_boardStatus & IMU_CALIBRATION_MASK) {
      pwmOutputDisableAll();
    } else {
      cameraRotationUpdate();
      actuatorsUpdate();
    }
  }
  /* This point may be reached if shut down is requested. */
  chThdExit(MSG_OK);
}
#endif

#if 0
static THD_WORKING_AREA(waMavlinkHandler, 2048);
static THD_FUNCTION(MavlinkHandler,arg) {
  (void) arg;
  while (!chThdShouldTerminateX()) {
    systime_t time;
    time = chVTGetSystemTime();

    handleStream();
    readMavlinkChannel();

    chThdSleepUntil(time += MS2ST(1000/MAX_STREAM_RATE_HZ));  //Max stream rate
  }
  /* This point may be reached if shut down is requested. */
  chThdExit(MSG_OK);
}
#endif

/**
 * @brief   Application entry point.
 * @details
 */
int main(void) {
  thread_t *tpBlinker_A  = NULL;
  thread_t *tpBlinker_B  = NULL;
  thread_t *tpPoller   = NULL;
  thread_t *tpAttitude = NULL;
  thread_t *tpMavlink  = NULL;

  thread_t *shelltp = NULL;
  
  /* System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  osalInit();
  chSysInit();

  //palToggleLedRed();
  //palToggleLedGreen();
  //while(true){
    //}

  /* Initializes a serial-over-USB CDC driver. */
#if 0
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);
  usbStop(serusbcfg.usbp);
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(500);
    usbConnectBus(serusbcfg.usbp);
    usbStart(serusbcfg.usbp, &usbcfg);

    shellInit();

#endif
  
    sdStart(&SD4, NULL);

  /* Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
    rccEnableAHB(RCC_AHBENR_CRCEN,FALSE);
#if 0
  /* Activates the serial driver 4 using the driver's default configuration. */
  //sdStart(&SD4, NULL);

  
  
  /* Activates the I2C driver 2. */
  i2cStart(&I2CD2, &i2cfg_d2);

  /* Enables the CRC peripheral clock. */
  //rccEnableAHB(RCC_AHBENR_CRCEN,FALSE);

  /* Initialize IMU data structure. */
  imuStructureInit(&g_IMU1, FALSE); // IMU1 on low address.
  //imuStructureInit(&g_IMU1, TRUE); 

  /* Loads settings from external EEPROM chip.
     WARNING! If MPU6050 sensor is not connected to the I2C bus, there
     aren't pull-up resistors on SDA and SCL lines, therefore it is
     impossible to communicate with EEPROM without the sensor connected. */

  bool result = loadSettings();
  if(result)
      chprintf((BaseSequentialStream *)&SD4, "pamiec OK\n");
  else 
      chprintf((BaseSequentialStream *)&SD4, "pamiec ZLE\n");
  g_boardStatus |= EEPROM_24C02_DETECTED;

  /* Initializes the MPU6050 sensor1. */
  if (mpu6050Init(g_IMU1.addr)) {
      
    g_boardStatus |= MPU6050_LOW_DETECTED;
    g_boardStatus |= IMU1_CALIBRATE_GYRO;
  }
#endif

//tpPoller = chThdCreateStatic(waPollMPU6050Thread, sizeof(waPollMPU6050Thread),
      //NORMALPRIO + 1, PollMPU6050Thread, NULL);

#if 0
  //if (g_boardStatus & MPU6050_LOW_DETECTED) {
  if (g_boardStatus & MPU6050_LOW_DETECTED) {
    /* Creates a taken binary semaphore. */
    chBSemObjectInit(&bsemIMU1DataReady, TRUE);
    /* Creates a taken binary semaphore. */
    chBSemObjectInit(&bsemStreamReady, TRUE);

    /* Creates the MPU6050 polling thread and attitude calculation thread. */
    tpPoller = chThdCreateStatic(waPollMPU6050Thread, sizeof(waPollMPU6050Thread),
      NORMALPRIO + 1, PollMPU6050Thread, NULL);
    tpAttitude = chThdCreateStatic(waAttitudeThread, sizeof(waAttitudeThread),
      HIGHPRIO, AttitudeThread, NULL);

    /* Starts motor drivers. */
    //pwmOutputStart();

    /* Starts ADC and ICU input drivers. */
    //mixedInputStart();
  }
#endif
  
  // na chwile to zakomentowuje, dopoki problem z USB
  // g_chnp = (BaseChannel *)&SDU1; //Default to USB for GUI
  chprintf((BaseSequentialStream *)&SD4, "wo1\n");
  g_chnp = (BaseChannel *)&SD4; //Default to USB for GUI
  chprintf((BaseSequentialStream *)&SD4, "wo2\n");

  /* Creates the blinker threads. */
  // green led

  //pwmOutputStart();

  tpBlinker_A = chThdCreateStatic(waBlinkerThread_A, sizeof(waBlinkerThread_A),
    LOWPRIO, BlinkerThread_A, NULL);

  //palClearPad(GPIOB, GPIOB_LED_0);

  // red led
  tpBlinker_B = chThdCreateStatic(waBlinkerThread_B, sizeof(waBlinkerThread_B),
    LOWPRIO, BlinkerThread_B, NULL);


//  pwmOutputStart();
#if 0
  while(true){
	  chThdSleepMilliseconds(6000);
  }
#endif

#if 0
  tpMavlink = chThdCreateStatic(waMavlinkHandler, sizeof(waMavlinkHandler),
    NORMALPRIO - 1, MavlinkHandler, NULL);
#endif

  /* Normal main() thread activity. */

  
  
#if 1
  //while(g_runMain){
  while(true){
	  //chThdSleepMilliseconds(60); // works but not perfectly
	  //chThdSleepMilliseconds(84); //
	  //chThdSleepMilliseconds(TELEMETRY_SLEEP_MS); //
	  //chThdSleepMilliseconds(70); //
	  //chThdSleepMilliseconds(140); //
	  // 0 - does not work
	  //chThdSleepMilliseconds(120);
	  chThdSleepMilliseconds(200); // the best so far
	  //chThdSleepMilliseconds(230); //
	  //chThdSleepMilliseconds(1790);
	  //chThdSleepMilliseconds(20); //
	  telemetryReadSerialData();
	  //chThdSleepMilliseconds(20); //
#if 0
	  // this sends only the 'z' messages
	    if ((g_chnp == (BaseChannel *)&SD4) && /* USB only; */
	        (chBSemWaitTimeout(&bsemStreamReady, TIME_IMMEDIATE) == MSG_OK)
	    //true
	    ) {
	      telemetryWriteStream(pStream, sizeof(float) * STREAM_BUFFER_SIZE / 2);
	    }
#endif
#if 0
    if (!shelltp && (SDU1.config->usbp->state == USB_ACTIVE)){
        // wersja oryginalna z demo z testhal w ChibiOS
       shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO+1);
        // ponizej wersja dla maple, o wiele szybsza
//              shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              //"shell", NORMALPRIO + 1,
                                              //shellThread, (void *)&shell_cfg1);
        
        
    } else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
    //chThdSleepMilliseconds(6000);
#endif
  }
#endif

  while (g_runMain) {
#if 0 //MS
	#ifdef BOARD_EVVGC_V1_X
    if ((g_boardStatus & EEPROM_24C02_DETECTED) && eepromIsDataLeft()) {
      eepromContinueSaving();
    }
	#endif
    //telemetryReadSerialDataMS();
#endif    
    telemetryReadSerialData();
    /* Process data stream if ready. */
#if 0 //MS
    if ((g_chnp == (BaseChannel *)&SDU1) && /* USB only; */
        (chBSemWaitTimeout(&bsemStreamReady, TIME_IMMEDIATE) == MSG_OK)) {
      telemetryWriteStream(pStream, sizeof(float) * STREAM_BUFFER_SIZE / 2);
    }
#endif
    //chThdSleepMilliseconds(TELEMETRY_SLEEP_MS);
    chThdSleepMilliseconds(40); // MSz
  }

#if 0
  /* Starting the shut-down sequence.*/
  if (tpAttitude != NULL) {
    chThdTerminate(tpAttitude); /* Requesting termination.                  */
    chThdWait(tpAttitude);      /* Waiting for the actual termination.      */
  }
  if (tpPoller != NULL) {
    chThdTerminate(tpPoller);   /* Requesting termination.                  */
    chThdWait(tpPoller);        /* Waiting for the actual termination.      */
  }
  if (tpBlinker_A != NULL) {
    chThdTerminate(tpBlinker_A);  /* Requesting termination.                  */
    chThdWait(tpBlinker_A);       /* Waiting for the actual termination.      */
  }
  if (tpBlinker_B != NULL) {
    chThdTerminate(tpBlinker_B);  /* Requesting termination.                  */
    chThdWait(tpBlinker_B);       /* Waiting for the actual termination.      */
  }

  mixedInputStop();             /* Stopping mixed input devices.            */
  pwmOutputStop();              /* Stopping pwm output devices.             */
  i2cStop(&I2CD2);              /* Stopping I2C2 device.                    */
  sdStop(&SD4);                 /* Stopping serial port 4.                  */
  usbStop(serusbcfg.usbp);      /* Stopping USB port.                       */
  usbDisconnectBus(serusbcfg.usbp);
  sduStop(&SDU1);               /* Stopping serial-over-USB CDC driver.     */
#endif
  
  osalSysDisable();

  /* Reset of all peripherals. */
  rccResetAPB1(0xFFFFFFFF);
  rccResetAPB2(0xFFFFFFFF);

  NVIC_SystemReset();

  return 0;
}
