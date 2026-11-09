/*!
 * @file Adafruit_STHS34PF80.h
 *
 * This is a library for the STHS34PF80 infrared sensor
 *
 * Designed specifically to work with the Adafruit STHS34PF80 breakout:
 *   https://www.adafruit.com/product/6426
 *
 * These sensors use I2C to communicate, 2 pins are required to interface.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Ladyada for Adafruit Industries.
 *
 * MIT license, all text here must be included in any redistribution.
 *
 */

#ifndef __ADAFRUIT_STHS34PF80_H__
#define __ADAFRUIT_STHS34PF80_H__

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Wire.h>

#include "Arduino.h"

#define STHS34PF80_DEFAULT_ADDR 0x5A ///< Default I2C address for the STHS34PF80

#define STHS34PF80_REG_LPF1 0x0C ///< Low-pass filter configuration 1 register
#define STHS34PF80_REG_LPF2 0x0D ///< Low-pass filter configuration 2 register
#define STHS34PF80_REG_WHO_AM_I 0x0F  ///< Device identification register
#define STHS34PF80_REG_AVG_TRIM 0x10  ///< Averaging configuration register
#define STHS34PF80_REG_CTRL0 0x17     ///< Control register 0 (gain mode)
#define STHS34PF80_REG_SENS_DATA 0x1D ///< Sensitivity data register
#define STHS34PF80_REG_CTRL1 0x20 ///< Control register 1 (ODR configuration)
#define STHS34PF80_REG_CTRL2 \
  0x21 ///< Control register 2 (boot, function access, one-shot)
#define STHS34PF80_REG_CTRL3 \
  0x22 ///< Control register 3 (interrupt configuration)
#define STHS34PF80_REG_STATUS 0x23      ///< Status register
#define STHS34PF80_REG_FUNC_STATUS 0x25 ///< Function status register
#define STHS34PF80_REG_TOBJECT_L 0x26   ///< Object temperature LSB register
#define STHS34PF80_REG_TOBJECT_H 0x27   ///< Object temperature MSB register
#define STHS34PF80_REG_TAMBIENT_L 0x28  ///< Ambient temperature LSB register
#define STHS34PF80_REG_TAMBIENT_H 0x29  ///< Ambient temperature MSB register
#define STHS34PF80_REG_TOBJ_COMP_L \
  0x38 ///< Compensated object temperature LSB register
#define STHS34PF80_REG_TOBJ_COMP_H \
  0x39 ///< Compensated object temperature MSB register
#define STHS34PF80_REG_TPRESENCE_L 0x3A ///< Presence detection LSB register
#define STHS34PF80_REG_TPRESENCE_H 0x3B ///< Presence detection MSB register
#define STHS34PF80_REG_TMOTION_L 0x3C   ///< Motion detection LSB register
#define STHS34PF80_REG_TMOTION_H 0x3D   ///< Motion detection MSB register
#define STHS34PF80_REG_TAMB_SHOCK_L \
  0x3E ///< Ambient shock detection LSB register
#define STHS34PF80_REG_TAMB_SHOCK_H \
  0x3F ///< Ambient shock detection MSB register

#define STHS34PF80_REG_FUNC_CFG_ADDR \
  0x08 ///< Embedded function configuration address register
#define STHS34PF80_REG_FUNC_CFG_DATA \
  0x09 ///< Embedded function configuration data register
#define STHS34PF80_REG_PAGE_RW 0x11 ///< Page read/write control register

#define STHS34PF80_EMBEDDED_RESET_ALGO \
  0x2A ///< Embedded function RESET_ALGO register address

#define STHS34PF80_PRES_FLAG 0x04       ///< Presence detection flag
#define STHS34PF80_MOT_FLAG 0x02        ///< Motion detection flag
#define STHS34PF80_TAMB_SHOCK_FLAG 0x01 ///< Ambient temperature shock flag

/*!
 * @brief Low-pass filter configuration options
 */
typedef enum {
  STHS34PF80_LPF_ODR_DIV_9 = 0x00,   ///< ODR/9
  STHS34PF80_LPF_ODR_DIV_20 = 0x01,  ///< ODR/20
  STHS34PF80_LPF_ODR_DIV_50 = 0x02,  ///< ODR/50
  STHS34PF80_LPF_ODR_DIV_100 = 0x03, ///< ODR/100
  STHS34PF80_LPF_ODR_DIV_200 = 0x04, ///< ODR/200
  STHS34PF80_LPF_ODR_DIV_400 = 0x05, ///< ODR/400
  STHS34PF80_LPF_ODR_DIV_800 = 0x06, ///< ODR/800
} sths34pf80_lpf_config_t;

/*!
 * @brief Ambient temperature averaging options
 */
typedef enum {
  STHS34PF80_AVG_T_8 = 0x00, ///< 8 samples (default)
  STHS34PF80_AVG_T_4 = 0x01, ///< 4 samples
  STHS34PF80_AVG_T_2 = 0x02, ///< 2 samples
  STHS34PF80_AVG_T_1 = 0x03, ///< 1 sample
} sths34pf80_avg_t_t;

/*!
 * @brief Object temperature averaging options
 */
typedef enum {
  STHS34PF80_AVG_TMOS_2 = 0x00,    ///< 2 samples
  STHS34PF80_AVG_TMOS_8 = 0x01,    ///< 8 samples
  STHS34PF80_AVG_TMOS_32 = 0x02,   ///< 32 samples
  STHS34PF80_AVG_TMOS_128 = 0x03,  ///< 128 samples (default)
  STHS34PF80_AVG_TMOS_256 = 0x04,  ///< 256 samples
  STHS34PF80_AVG_TMOS_512 = 0x05,  ///< 512 samples
  STHS34PF80_AVG_TMOS_1024 = 0x06, ///< 1024 samples
  STHS34PF80_AVG_TMOS_2048 = 0x07, ///< 2048 samples
} sths34pf80_avg_tmos_t;

/*!
 * @brief Output data rate options
 */
typedef enum {
  STHS34PF80_ODR_POWER_DOWN = 0x00, ///< Power-down mode
  STHS34PF80_ODR_0_25_HZ = 0x01,    ///< 0.25 Hz
  STHS34PF80_ODR_0_5_HZ = 0x02,     ///< 0.5 Hz
  STHS34PF80_ODR_1_HZ = 0x03,       ///< 1 Hz
  STHS34PF80_ODR_2_HZ = 0x04,       ///< 2 Hz
  STHS34PF80_ODR_4_HZ = 0x05,       ///< 4 Hz
  STHS34PF80_ODR_8_HZ = 0x06,       ///< 8 Hz
  STHS34PF80_ODR_15_HZ = 0x07,      ///< 15 Hz
  STHS34PF80_ODR_30_HZ = 0x08,      ///< 30 Hz (1xxx)
} sths34pf80_odr_t;

/*!
 * @brief Interrupt enable signal options
 */
typedef enum {
  STHS34PF80_INT_HIGH_Z = 0x00, ///< High-Z (disabled)
  STHS34PF80_INT_DRDY = 0x01,   ///< Data ready
  STHS34PF80_INT_OR = 0x02,     ///< INT_OR (function flags)
} sths34pf80_int_signal_t;

/*!
 * @brief Class that stores state and functions for interacting with the
 * STHS34PF80
 */
class Adafruit_STHS34PF80 {
 public:
  Adafruit_STHS34PF80();
  ~Adafruit_STHS34PF80();

  bool begin(uint8_t i2c_addr = STHS34PF80_DEFAULT_ADDR, TwoWire* wire = &Wire);
  bool isConnected();
  bool reset();

  bool setMotionLowPassFilter(sths34pf80_lpf_config_t config);
  sths34pf80_lpf_config_t getMotionLowPassFilter();
  bool setMotionPresenceLowPassFilter(sths34pf80_lpf_config_t config);
  sths34pf80_lpf_config_t getMotionPresenceLowPassFilter();
  bool setPresenceLowPassFilter(sths34pf80_lpf_config_t config);
  sths34pf80_lpf_config_t getPresenceLowPassFilter();
  bool setTemperatureLowPassFilter(sths34pf80_lpf_config_t config);
  sths34pf80_lpf_config_t getTemperatureLowPassFilter();

  bool setAmbTempAveraging(sths34pf80_avg_t_t config);
  sths34pf80_avg_t_t getAmbTempAveraging();
  bool setObjAveraging(sths34pf80_avg_tmos_t config);
  sths34pf80_avg_tmos_t getObjAveraging();

  bool setWideGainMode(bool wide_mode);
  bool getWideGainMode();

  bool setSensitivity(int8_t sensitivity);
  int8_t getSensitivity();

  bool setBlockDataUpdate(bool enable);
  bool getBlockDataUpdate();
  bool setOutputDataRate(sths34pf80_odr_t odr);
  sths34pf80_odr_t getOutputDataRate();

  bool rebootOTPmemory();
  bool enableEmbeddedFuncPage(bool enable);
  bool triggerOneshot();

  bool writeEmbeddedFunction(uint8_t addr, uint8_t* data, uint8_t len);

  bool setIntPolarity(bool active_low);
  bool setIntOpenDrain(bool open_drain);
  bool setIntLatched(bool latched);
  bool setIntMask(uint8_t mask);
  uint8_t getIntMask();
  bool setIntSignal(sths34pf80_int_signal_t signal);
  sths34pf80_int_signal_t getIntSignal();

  bool isDataReady();
  bool isPresence();
  bool isMotion();
  bool isTempShock();

  
  struct CheckedReading {
    int16_t objectRaw, presenceRaw, motionRaw, compensatedRaw, shockRaw;
    float ambientC;
    uint8_t flags;
  };
  int readChecked(CheckedReading &out); 

  int16_t readObjectTemperature();
  float readAmbientTemperature();
  int16_t readCompensatedObjectTemperature();
  int16_t readPresence();
  int16_t readMotion();
  int16_t readTempShock();

 private:
  Adafruit_I2CDevice* i2c_dev = nullptr;
  bool reinitPending = false;
  bool safeSetOutputDataRate(sths34pf80_odr_t current_odr,
                             sths34pf80_odr_t new_odr);
  bool algorithmReset(); // TODO: Implement algorithm reset procedure
};

#endif
