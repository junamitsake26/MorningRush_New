/*!
 * @file Adafruit_STHS34PF80.cpp
 *
 * @mainpage Adafruit STHS34PF80 infrared sensor library
 *
 * @section intro_sec Introduction
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
 * @section author Author
 *
 * Written by Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * MIT license, all text here must be included in any redistribution
 *
 */

#include "Adafruit_STHS34PF80.h"

/*!
 * @brief Instantiates a new STHS34PF80 class
 */
Adafruit_STHS34PF80::Adafruit_STHS34PF80() {}

/*!
 * @brief Cleans up the STHS34PF80
 */
Adafruit_STHS34PF80::~Adafruit_STHS34PF80() {
  if (i2c_dev) {
    delete i2c_dev;
  }
}

/*!
 * @brief Initializes the hardware and detects a valid STHS34PF80
 * @param i2c_addr I2C address to use
 * @param wire The Wire object to be used for I2C connections
 * @return True if initialization was successful, otherwise false
 */
bool Adafruit_STHS34PF80::begin(uint8_t i2c_addr, TwoWire* wire) {
  if (i2c_dev) {
    delete i2c_dev;
  }

  i2c_dev = new Adafruit_I2CDevice(i2c_addr, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  if (!isConnected()) {
    return false;
  }

  
  
  
  Adafruit_BusIO_Register ctrl1(i2c_dev, STHS34PF80_REG_CTRL1, 1);
  Adafruit_BusIO_Register status(i2c_dev, STHS34PF80_REG_STATUS, 1);
  Adafruit_BusIO_Register flags(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);
  uint8_t c = 0, s = 0, f = 0;
  if (!ctrl1.read(&c, 1)) return false;
  if (c & 0x0F) {
    if (!reinitPending) {
      if (!flags.read(&f, 1)) return false;
      reinitPending = true;
      return false;
    }
    if (!status.read(&s, 1) || !(s & 4)) return false;
    if (!ctrl1.write(c & 0xF0) || !flags.read(&f, 1)) return false;
  }
  reinitPending = false;

  if (!reset()) {
    return false;
  }

  // Apply recommended default settings
  if (!setObjAveraging(STHS34PF80_AVG_TMOS_32)) {
    return false;
  }

  if (!setAmbTempAveraging(STHS34PF80_AVG_T_8)) {
    return false;
  }

  if (!setBlockDataUpdate(true)) {
    return false;
  }

  if (!setOutputDataRate(STHS34PF80_ODR_1_HZ)) {
    return false;
  }

  return true;
}

/*!
 * @brief Check if the sensor is connected by reading device ID
 * @return True if device ID matches expected value (0xD3), false otherwise
 */
bool Adafruit_STHS34PF80::isConnected() {
  if (!i2c_dev) {
    return false;
  }

  Adafruit_BusIO_Register chip_id =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_WHO_AM_I, 1);

  return chip_id.read() == 0xD3;
}

/*!
 * @brief Reset the sensor completely
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::reset() {
  // Reboot OTP memory
  if (!rebootOTPmemory()) {
    return false;
  }

  // Wait for sensor reset to complete
  delay(5);

  // Reset the internal algorithm
  if (!algorithmReset()) {
    return false;
  }

  return true;
}

/*!
 * @brief Set the motion detection low-pass filter configuration
 * @param config The LPF configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setMotionLowPassFilter(
    sths34pf80_lpf_config_t config) {
  Adafruit_BusIO_Register lpf1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF1, 1);

  Adafruit_BusIO_RegisterBits lpf_m_bits =
      Adafruit_BusIO_RegisterBits(&lpf1_reg, 3, 0);

  return lpf_m_bits.write(config);
}

/*!
 * @brief Get the motion detection low-pass filter configuration
 * @return The current LPF configuration value
 */
sths34pf80_lpf_config_t Adafruit_STHS34PF80::getMotionLowPassFilter() {
  Adafruit_BusIO_Register lpf1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF1, 1);

  Adafruit_BusIO_RegisterBits lpf_m_bits =
      Adafruit_BusIO_RegisterBits(&lpf1_reg, 3, 0);

  return (sths34pf80_lpf_config_t)lpf_m_bits.read();
}

/*!
 * @brief Set the motion and presence detection low-pass filter configuration
 * @param config The LPF configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setMotionPresenceLowPassFilter(
    sths34pf80_lpf_config_t config) {
  Adafruit_BusIO_Register lpf1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF1, 1);

  Adafruit_BusIO_RegisterBits lpf_p_m_bits =
      Adafruit_BusIO_RegisterBits(&lpf1_reg, 3, 3);

  return lpf_p_m_bits.write(config);
}

/*!
 * @brief Get the motion and presence detection low-pass filter configuration
 * @return The current LPF configuration value
 */
sths34pf80_lpf_config_t Adafruit_STHS34PF80::getMotionPresenceLowPassFilter() {
  Adafruit_BusIO_Register lpf1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF1, 1);

  Adafruit_BusIO_RegisterBits lpf_p_m_bits =
      Adafruit_BusIO_RegisterBits(&lpf1_reg, 3, 3);

  return (sths34pf80_lpf_config_t)lpf_p_m_bits.read();
}

/*!
 * @brief Set the presence detection low-pass filter configuration
 * @param config The LPF configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setPresenceLowPassFilter(
    sths34pf80_lpf_config_t config) {
  Adafruit_BusIO_Register lpf2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF2, 1);

  Adafruit_BusIO_RegisterBits lpf_p_bits =
      Adafruit_BusIO_RegisterBits(&lpf2_reg, 3, 3);

  return lpf_p_bits.write(config);
}

/*!
 * @brief Get the presence detection low-pass filter configuration
 * @return The current LPF configuration value
 */
sths34pf80_lpf_config_t Adafruit_STHS34PF80::getPresenceLowPassFilter() {
  Adafruit_BusIO_Register lpf2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF2, 1);

  Adafruit_BusIO_RegisterBits lpf_p_bits =
      Adafruit_BusIO_RegisterBits(&lpf2_reg, 3, 3);

  return (sths34pf80_lpf_config_t)lpf_p_bits.read();
}

/*!
 * @brief Set the ambient temperature shock detection low-pass filter
 * configuration
 * @param config The LPF configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setTemperatureLowPassFilter(
    sths34pf80_lpf_config_t config) {
  Adafruit_BusIO_Register lpf2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF2, 1);

  Adafruit_BusIO_RegisterBits lpf_a_t_bits =
      Adafruit_BusIO_RegisterBits(&lpf2_reg, 3, 0);

  return lpf_a_t_bits.write(config);
}

/*!
 * @brief Get the ambient temperature shock detection low-pass filter
 * configuration
 * @return The current LPF configuration value
 */
sths34pf80_lpf_config_t Adafruit_STHS34PF80::getTemperatureLowPassFilter() {
  Adafruit_BusIO_Register lpf2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_LPF2, 1);

  Adafruit_BusIO_RegisterBits lpf_a_t_bits =
      Adafruit_BusIO_RegisterBits(&lpf2_reg, 3, 0);

  return (sths34pf80_lpf_config_t)lpf_a_t_bits.read();
}

/*!
 * @brief Set ambient temperature averaging configuration
 * @param config The averaging configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setAmbTempAveraging(sths34pf80_avg_t_t config) {
  Adafruit_BusIO_Register avg_trim_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_AVG_TRIM, 1);

  Adafruit_BusIO_RegisterBits avg_t_bits =
      Adafruit_BusIO_RegisterBits(&avg_trim_reg, 2, 4);

  return avg_t_bits.write(config);
}

/*!
 * @brief Get ambient temperature averaging configuration
 * @return The current averaging configuration value
 */
sths34pf80_avg_t_t Adafruit_STHS34PF80::getAmbTempAveraging() {
  Adafruit_BusIO_Register avg_trim_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_AVG_TRIM, 1);

  Adafruit_BusIO_RegisterBits avg_t_bits =
      Adafruit_BusIO_RegisterBits(&avg_trim_reg, 2, 4);

  return (sths34pf80_avg_t_t)avg_t_bits.read();
}

/*!
 * @brief Set object temperature averaging configuration
 * @param config The averaging configuration value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setObjAveraging(sths34pf80_avg_tmos_t config) {
  Adafruit_BusIO_Register avg_trim_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_AVG_TRIM, 1);

  Adafruit_BusIO_RegisterBits avg_tmos_bits =
      Adafruit_BusIO_RegisterBits(&avg_trim_reg, 3, 0);

  return avg_tmos_bits.write(config);
}

/*!
 * @brief Get object temperature averaging configuration
 * @return The current averaging configuration value
 */
sths34pf80_avg_tmos_t Adafruit_STHS34PF80::getObjAveraging() {
  Adafruit_BusIO_Register avg_trim_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_AVG_TRIM, 1);

  Adafruit_BusIO_RegisterBits avg_tmos_bits =
      Adafruit_BusIO_RegisterBits(&avg_trim_reg, 3, 0);

  return (sths34pf80_avg_tmos_t)avg_tmos_bits.read();
}

/*!
 * @brief Set wide gain mode configuration
 * @param wide_mode True for wide mode, false for default gain mode
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setWideGainMode(bool wide_mode) {
  Adafruit_BusIO_Register ctrl0_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL0, 1);

  Adafruit_BusIO_RegisterBits gain_bits =
      Adafruit_BusIO_RegisterBits(&ctrl0_reg, 3, 4);

  return gain_bits.write(wide_mode ? 0x00 : 0x07);
}

/*!
 * @brief Get wide gain mode configuration
 * @return True if in wide mode, false if in default gain mode
 */
bool Adafruit_STHS34PF80::getWideGainMode() {
  Adafruit_BusIO_Register ctrl0_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL0, 1);

  Adafruit_BusIO_RegisterBits gain_bits =
      Adafruit_BusIO_RegisterBits(&ctrl0_reg, 3, 4);

  return gain_bits.read() == 0x00;
}

/*!
 * @brief Set sensitivity value for ambient temperature compensation
 * @param sensitivity Signed 8-bit sensitivity value (two's complement)
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setSensitivity(int8_t sensitivity) {
  Adafruit_BusIO_Register sens_data_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_SENS_DATA, 1);

  return sens_data_reg.write((uint8_t)sensitivity);
}

/*!
 * @brief Get sensitivity value for ambient temperature compensation
 * @return Signed 8-bit sensitivity value (two's complement)
 */
int8_t Adafruit_STHS34PF80::getSensitivity() {
  Adafruit_BusIO_Register sens_data_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_SENS_DATA, 1);

  return (int8_t)sens_data_reg.read();
}

/*!
 * @brief Set block data update configuration
 * @param enable True to enable block data update, false to disable
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setBlockDataUpdate(bool enable) {
  Adafruit_BusIO_Register ctrl1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL1, 1);

  Adafruit_BusIO_RegisterBits bdu_bit =
      Adafruit_BusIO_RegisterBits(&ctrl1_reg, 1, 4);

  return bdu_bit.write(enable ? 1 : 0);
}

/*!
 * @brief Get block data update configuration
 * @return True if block data update is enabled, false if disabled
 */
bool Adafruit_STHS34PF80::getBlockDataUpdate() {
  Adafruit_BusIO_Register ctrl1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL1, 1);

  Adafruit_BusIO_RegisterBits bdu_bit =
      Adafruit_BusIO_RegisterBits(&ctrl1_reg, 1, 4);

  return bdu_bit.read() == 1;
}

/*!
 * @brief Set output data rate configuration with validation
 * Ported from: sths34pf80_tmos_odr_set
 * @param odr The output data rate value
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setOutputDataRate(sths34pf80_odr_t odr) {
  // sths34pf80_ctrl1_t ctrl1;
  // sths34pf80_avg_trim_t avg_trim;
  // sths34pf80_tmos_odr_t max_odr = STHS34PF80_TMOS_ODR_AT_30Hz;
  // int32_t ret;
  if (!i2c_dev) {
    return false;
  }

  sths34pf80_odr_t max_odr = STHS34PF80_ODR_30_HZ;
  sths34pf80_odr_t current_odr = getOutputDataRate();

  // ret = sths34pf80_read_reg(ctx, STHS34PF80_CTRL1, (uint8_t *)&ctrl1, 1);
  // if (ret == 0)
  // {
  //   ret = sths34pf80_read_reg(ctx, STHS34PF80_AVG_TRIM, (uint8_t *)&avg_trim,
  //   1);
  sths34pf80_avg_tmos_t avg_tmos = getObjAveraging();

  //   switch(avg_trim.avg_tmos)
  //   {
  switch (avg_tmos) {
    //     default:
    //     case STHS34PF80_AVG_TMOS_2:
    //     case STHS34PF80_AVG_TMOS_8:
    //     case STHS34PF80_AVG_TMOS_32:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_30Hz;
    //       break;
    default:
    case STHS34PF80_AVG_TMOS_2:
    case STHS34PF80_AVG_TMOS_8:
    case STHS34PF80_AVG_TMOS_32:
      max_odr = STHS34PF80_ODR_30_HZ;
      break;
    //     case STHS34PF80_AVG_TMOS_128:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_8Hz;
    //       break;
    case STHS34PF80_AVG_TMOS_128:
      max_odr = STHS34PF80_ODR_8_HZ;
      break;
    //     case STHS34PF80_AVG_TMOS_256:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_4Hz;
    //       break;
    case STHS34PF80_AVG_TMOS_256:
      max_odr = STHS34PF80_ODR_4_HZ;
      break;
    //     case STHS34PF80_AVG_TMOS_512:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_2Hz;
    //       break;
    case STHS34PF80_AVG_TMOS_512:
      max_odr = STHS34PF80_ODR_2_HZ;
      break;
    //     case STHS34PF80_AVG_TMOS_1024:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_1Hz;
    //       break;
    case STHS34PF80_AVG_TMOS_1024:
      max_odr = STHS34PF80_ODR_1_HZ;
      break;
    //     case STHS34PF80_AVG_TMOS_2048:
    //       max_odr = STHS34PF80_TMOS_ODR_AT_0Hz50;
    //       break;
    case STHS34PF80_AVG_TMOS_2048:
      max_odr = STHS34PF80_ODR_0_5_HZ;
      break;
  }

  // if (ret == 0)
  // {
  //   if (val > max_odr)
  //   {
  //     return -1;
  //   }
  if (odr > max_odr) {
    return false; // Requested ODR exceeds maximum for current averaging setting
  }

  //   ret = sths34pf80_tmos_odr_check_safe_set(ctx, ctrl1, (uint8_t)val);
  // }
  return safeSetOutputDataRate(current_odr, odr);
}

/*!
 * @brief Get output data rate configuration
 * @return The current output data rate value
 */
sths34pf80_odr_t Adafruit_STHS34PF80::getOutputDataRate() {
  Adafruit_BusIO_Register ctrl1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL1, 1);

  Adafruit_BusIO_RegisterBits odr_bits =
      Adafruit_BusIO_RegisterBits(&ctrl1_reg, 4, 0);

  return (sths34pf80_odr_t)odr_bits.read();
}

/*!
 * @brief Reboot OTP memory
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::rebootOTPmemory() {
  Adafruit_BusIO_Register ctrl2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL2, 1);

  Adafruit_BusIO_RegisterBits boot_bit =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 1, 7);

  return boot_bit.write(1);
}

/*!
 * @brief Enable or disable embedded function page access
 * @param enable True to enable embedded function page, false to disable
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::enableEmbeddedFuncPage(bool enable) {
  Adafruit_BusIO_Register ctrl2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL2, 1);

  Adafruit_BusIO_RegisterBits func_cfg_access_bit =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 1, 4);

  return func_cfg_access_bit.write(enable ? 1 : 0);
}

/*!
 * @brief Trigger one-shot measurement
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::triggerOneshot() {
  Adafruit_BusIO_Register ctrl2_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL2, 1);

  Adafruit_BusIO_RegisterBits oneshot_bit =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 1, 0);

  return oneshot_bit.write(1);
}

/*!
 * @brief Set interrupt polarity
 * @param active_low True for active low (default), false for active high
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setIntPolarity(bool active_low) {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits int_h_l_bit =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 1, 7);

  return int_h_l_bit.write(active_low ? 1 : 0);
}

/*!
 * @brief Set interrupt output type
 * @param open_drain True for open drain, false for push-pull (default)
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setIntOpenDrain(bool open_drain) {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits pp_od_bit =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 1, 6);

  return pp_od_bit.write(open_drain ? 1 : 0);
}

/*!
 * @brief Set interrupt latched mode
 * @param latched True for latched mode, false for pulsed mode (default)
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setIntLatched(bool latched) {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits int_latched_bit =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 1, 2);

  return int_latched_bit.write(latched ? 1 : 0);
}

/*!
 * @brief Set interrupt mask for function status flags
 * @param mask Interrupt mask (bits 2:0 for PRES_FLAG, MOT_FLAG,
 * TAMB_SHOCK_FLAG)
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setIntMask(uint8_t mask) {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits int_mask_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 3, 3);

  return int_mask_bits.write(mask & 0x07);
}

/*!
 * @brief Get interrupt mask for function status flags
 * @return Current interrupt mask value (bits 2:0 for PRES_FLAG, MOT_FLAG,
 * TAMB_SHOCK_FLAG)
 */
uint8_t Adafruit_STHS34PF80::getIntMask() {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits int_mask_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 3, 3);

  return int_mask_bits.read();
}

/*!
 * @brief Set interrupt signal type
 * @param signal Interrupt signal type (HIGH_Z, DRDY, or INT_OR)
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::setIntSignal(sths34pf80_int_signal_t signal) {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits ien_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 2, 0);

  return ien_bits.write(signal);
}

/*!
 * @brief Get interrupt signal type
 * @return Current interrupt signal type
 */
sths34pf80_int_signal_t Adafruit_STHS34PF80::getIntSignal() {
  Adafruit_BusIO_Register ctrl3_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits ien_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 2, 0);

  return (sths34pf80_int_signal_t)ien_bits.read();
}

/*!
 * @brief Check if new data is ready
 * @return True if new data is available, false otherwise
 */
bool Adafruit_STHS34PF80::isDataReady() {
  Adafruit_BusIO_Register status_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_STATUS, 1);

  Adafruit_BusIO_RegisterBits drdy_bit =
      Adafruit_BusIO_RegisterBits(&status_reg, 1, 2);

  return drdy_bit.read() == 1;
}

/*!
 * @brief Check if presence detection flag is set
 * @return True if presence is detected, false otherwise
 */
bool Adafruit_STHS34PF80::isPresence() {
  Adafruit_BusIO_Register func_status_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);

  Adafruit_BusIO_RegisterBits pres_flag_bit =
      Adafruit_BusIO_RegisterBits(&func_status_reg, 1, 2);

  return pres_flag_bit.read() == 1;
}

/*!
 * @brief Check if motion detection flag is set
 * @return True if motion is detected, false otherwise
 */
bool Adafruit_STHS34PF80::isMotion() {
  Adafruit_BusIO_Register func_status_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);

  Adafruit_BusIO_RegisterBits mot_flag_bit =
      Adafruit_BusIO_RegisterBits(&func_status_reg, 1, 1);

  return mot_flag_bit.read() == 1;
}

/*!
 * @brief Check if ambient temperature shock flag is set
 * @return True if temperature shock is detected, false otherwise
 */
bool Adafruit_STHS34PF80::isTempShock() {
  Adafruit_BusIO_Register func_status_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);

  Adafruit_BusIO_RegisterBits tamb_shock_flag_bit =
      Adafruit_BusIO_RegisterBits(&func_status_reg, 1, 0);

  return tamb_shock_flag_bit.read() == 1;
}

/*!
 * @brief Read object temperature raw value
 * @return 16-bit signed object temperature value
 */
int16_t Adafruit_STHS34PF80::readObjectTemperature() {
  Adafruit_BusIO_Register tobj_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_TOBJECT_L, 2, LSBFIRST);

  return (int16_t)tobj_reg.read();
}

/*!
 * @brief Read ambient temperature in degrees Celsius
 * @return Ambient temperature in degrees Celsius
 */
float Adafruit_STHS34PF80::readAmbientTemperature() {
  Adafruit_BusIO_Register tamb_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_TAMBIENT_L, 2, LSBFIRST);

  int16_t raw_temp = (int16_t)tamb_reg.read();
  return raw_temp / 100.0f;
}

/*!
 * @brief Read compensated object temperature raw value
 * @return 16-bit signed compensated object temperature value
 */
int16_t Adafruit_STHS34PF80::readCompensatedObjectTemperature() {
  Adafruit_BusIO_Register tobj_comp_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_TOBJ_COMP_L, 2, LSBFIRST);

  return (int16_t)tobj_comp_reg.read();
}

/*!
 * @brief Read presence detection raw value
 * @return 16-bit signed presence detection value
 */
int16_t Adafruit_STHS34PF80::readPresence() {
  Adafruit_BusIO_Register tpres_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_TPRESENCE_L, 2, LSBFIRST);

  return (int16_t)tpres_reg.read();
}

/*!
 * @brief Read motion detection raw value
 * @return 16-bit signed motion detection value
 */
int16_t Adafruit_STHS34PF80::readMotion() {
  Adafruit_BusIO_Register tmot_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_TMOTION_L, 2, LSBFIRST);

  return (int16_t)tmot_reg.read();
}

/*!
 * @brief Read ambient temperature shock detection raw value
 * @return 16-bit signed ambient temperature shock detection value
 */
int16_t Adafruit_STHS34PF80::readTempShock() {
  Adafruit_BusIO_Register tamb_shock_reg = Adafruit_BusIO_Register(
      i2c_dev, STHS34PF80_REG_TAMB_SHOCK_L, 2, LSBFIRST);

  return (int16_t)tamb_shock_reg.read();
}

/*!
 * @brief Write data to embedded function registers
 * Ported from: sths34pf80_func_cfg_write
 * @param addr Embedded function register address
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::writeEmbeddedFunction(uint8_t addr, uint8_t* data,
                                                uint8_t len) {
  // sths34pf80_ctrl1_t ctrl1;
  // uint8_t odr;
  // sths34pf80_page_rw_t page_rw = {0};
  // int32_t ret;
  // uint8_t i;
  if (!i2c_dev) {
    return false;
  }

  // /* Save current odr and enter PD mode */
  // ret = sths34pf80_read_reg(ctx, STHS34PF80_CTRL1, (uint8_t *)&ctrl1, 1);
  // odr = ctrl1.odr;
  // ret += sths34pf80_tmos_odr_check_safe_set(ctx, ctrl1, 0);
  sths34pf80_odr_t current_odr = getOutputDataRate();
  if (!safeSetOutputDataRate(current_odr, STHS34PF80_ODR_POWER_DOWN)) {
    return false;
  }

  // /* Enable access to embedded functions register */
  // ret += sths34pf80_mem_bank_set(ctx, STHS34PF80_EMBED_FUNC_MEM_BANK);
  if (!enableEmbeddedFuncPage(true)) {
    return false;
  }

  // /* Enable write mode */
  // page_rw.func_cfg_write = 1;
  // ret += sths34pf80_write_reg(ctx, STHS34PF80_PAGE_RW, (uint8_t *)&page_rw,
  // 1);
  Adafruit_BusIO_Register page_rw_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_PAGE_RW, 1);
  Adafruit_BusIO_RegisterBits func_cfg_write_bit =
      Adafruit_BusIO_RegisterBits(&page_rw_reg, 1, 6);
  if (!func_cfg_write_bit.write(1)) {
    enableEmbeddedFuncPage(false);
    safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr);
    return false;
  }

  // /* Select register address (it will autoincrement after each write) */
  // ret += sths34pf80_write_reg(ctx, STHS34PF80_FUNC_CFG_ADDR, &addr, 1);
  Adafruit_BusIO_Register func_cfg_addr_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_CFG_ADDR, 1);
  if (!func_cfg_addr_reg.write(addr)) {
    func_cfg_write_bit.write(0);
    enableEmbeddedFuncPage(false);
    safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr);
    return false;
  }

  // /* Write data */
  // for (i = 0; i < len; i++) {
  //   ret += sths34pf80_write_reg(ctx, STHS34PF80_FUNC_CFG_DATA, &data[i], 1);
  // }
  Adafruit_BusIO_Register func_cfg_data_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_CFG_DATA, 1);
  for (uint8_t i = 0; i < len; i++) {
    if (!func_cfg_data_reg.write(data[i])) {
      func_cfg_write_bit.write(0);
      enableEmbeddedFuncPage(false);
      safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr);
      return false;
    }
  }

  // /* Disable write mode */
  // page_rw.func_cfg_write = 0;
  // ret += sths34pf80_write_reg(ctx, STHS34PF80_PAGE_RW, (uint8_t *)&page_rw,
  // 1);
  if (!func_cfg_write_bit.write(0)) {
    enableEmbeddedFuncPage(false);
    safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr);
    return false;
  }

  // /* Disable access to embedded functions register */
  // ret += sths34pf80_mem_bank_set(ctx, STHS34PF80_MAIN_MEM_BANK);
  if (!enableEmbeddedFuncPage(false)) {
    safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr);
    return false;
  }

  // /* Restore odr */
  // ret += sths34pf80_tmos_odr_check_safe_set(ctx, ctrl1, odr);
  if (!safeSetOutputDataRate(STHS34PF80_ODR_POWER_DOWN, current_odr)) {
    return false;
  }

  // return ret;
  return true;
}

/*!
 * @brief Algorithm reset procedure
 * Ported from: sths34pf80_algo_reset
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::algorithmReset() {
  // tmp = 1;
  // ret = sths34pf80_func_cfg_write(ctx, STHS34PF80_RESET_ALGO, &tmp, 1);
  uint8_t reset_value = 1;
  return writeEmbeddedFunction(STHS34PF80_EMBEDDED_RESET_ALGO, &reset_value, 1);
}

/*!
 * @brief Safe ODR setting with proper algorithm reset and power-down procedures
 * Ported from: sths34pf80_tmos_odr_check_safe_set
 * @param current_odr The current output data rate
 * @param new_odr The new output data rate to set
 * @return True if successful, false otherwise
 */
bool Adafruit_STHS34PF80::safeSetOutputDataRate(sths34pf80_odr_t current_odr,
                                                sths34pf80_odr_t new_odr) {
  // sths34pf80_func_status_t func_status;
  // sths34pf80_tmos_drdy_status_t status;
  // int32_t ret = 0;
  if (!i2c_dev) {
    return false;
  }

  Adafruit_BusIO_Register ctrl1_reg =
      Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_CTRL1, 1);

  Adafruit_BusIO_RegisterBits odr_bits =
      Adafruit_BusIO_RegisterBits(&ctrl1_reg, 4, 0);

  // if (odr_new > 0U) {
  if (new_odr > STHS34PF80_ODR_POWER_DOWN) {
    /*
     * Do a clean reset algo procedure everytime odr is changed to an
     * operative state.
     */
    // ctrl1.odr = 0;
    // ret = sths34pf80_write_reg(ctx, STHS34PF80_CTRL1, (uint8_t *)&ctrl1, 1);
    if (!odr_bits.write(STHS34PF80_ODR_POWER_DOWN)) {
      return false;
    }

    // ret += sths34pf80_algo_reset(ctx);
    if (!algorithmReset()) {
      return false;
    }

  } else {
    /* if we need to go to power-down from an operative state
     * perform the safe power-down.
     */
    // if ((uint8_t)ctrl1.odr > 0U) {
    if (current_odr > STHS34PF80_ODR_POWER_DOWN) {
      /* reset the DRDY bit */
      // ret = sths34pf80_read_reg(ctx, STHS34PF80_FUNC_STATUS, (uint8_t
      // *)&func_status, 1);
      Adafruit_BusIO_Register func_status_reg =
          Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);
      func_status_reg.read(); // Reading clears the DRDY bit

      /* wait DRDY bit go to '1' */
      // do {
      //   ret += sths34pf80_tmos_drdy_status_get(ctx, &status);
      // } while (status.drdy != 0U);
      Adafruit_BusIO_Register status_reg =
          Adafruit_BusIO_Register(i2c_dev, STHS34PF80_REG_STATUS, 1);

      Adafruit_BusIO_RegisterBits drdy_bit =
          Adafruit_BusIO_RegisterBits(&status_reg, 1, 2);

      
      if (drdy_bit.read() != 1) return false;

      // Continue even if DRDY timeout occurs
      // if (timeout == 0) {
      //   return false; // Timeout waiting for DRDY
      // }

      /* set ODR to 0 */
      // ctrl1.odr = 0;
      // ret += sths34pf80_write_reg(ctx, STHS34PF80_CTRL1, (uint8_t *)&ctrl1,
      // 1);
      if (!odr_bits.write(STHS34PF80_ODR_POWER_DOWN)) {
        return false;
      }

      /* reset the DRDY bit */
      // ret += sths34pf80_read_reg(ctx, STHS34PF80_FUNC_STATUS, (uint8_t
      // *)&func_status, 1);
      func_status_reg.read(); // Reading clears the DRDY bit again
    }
  }

  // Final ODR set (implied from original function usage context)
  return odr_bits.write(new_odr);
}

int Adafruit_STHS34PF80::readChecked(CheckedReading &out) {
  
  if (!i2c_dev) {
    return -1;
  }

  uint8_t status = 0;
  uint8_t temperatureBytes[4];
  uint8_t algorithmBytes[8];
  uint8_t flags = 0;

  
  Adafruit_BusIO_Register statusRegister(i2c_dev, STHS34PF80_REG_STATUS, 1);
  if (!statusRegister.read(&status, 1)) {
    return -1;
  }
  if ((status & 0x04) == 0) {
    return 0;
  }

  
  Adafruit_BusIO_Register temperatureRegister(i2c_dev, STHS34PF80_REG_TOBJECT_L, 4);
  if (!temperatureRegister.read(temperatureBytes, 4)) {
    return -1;
  }

  
  Adafruit_BusIO_Register algorithmRegister(i2c_dev, STHS34PF80_REG_TOBJ_COMP_L, 8);
  if (!algorithmRegister.read(algorithmBytes, 8)) {
    return -1;
  }

  
  Adafruit_BusIO_Register flagsRegister(i2c_dev, STHS34PF80_REG_FUNC_STATUS, 1);
  if (!flagsRegister.read(&flags, 1)) {
    return -1;
  }

  
  
  out.objectRaw = (int16_t)(temperatureBytes[0] | (uint16_t(temperatureBytes[1]) << 8));
  int16_t ambientRaw = (int16_t)(temperatureBytes[2] | (uint16_t(temperatureBytes[3]) << 8));
  out.ambientC = ambientRaw / 100.0f; 
  out.compensatedRaw = (int16_t)(algorithmBytes[0] | (uint16_t(algorithmBytes[1]) << 8));
  out.presenceRaw = (int16_t)(algorithmBytes[2] | (uint16_t(algorithmBytes[3]) << 8));
  out.motionRaw = (int16_t)(algorithmBytes[4] | (uint16_t(algorithmBytes[5]) << 8));
  out.shockRaw = (int16_t)(algorithmBytes[6] | (uint16_t(algorithmBytes[7]) << 8));
  out.flags = flags;
  return 1;
}
