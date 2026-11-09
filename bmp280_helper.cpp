#include "bmp280_helper.h"
#include <Wire.h>
#include "bmp280_conversion.h" // .h = middle man b/w datasheet .cpp and .ino
#include "config.h"

namespace BMP280 {
const int address_bmp280 = Config::BMP_ADDRESS;

bool begin() {
  // check the sensor ID, load calibration values and start normal readings

  // main sketch has already started the shared I2C bus
  
  //4.3.1 Register 0xD0 “id” - checking I2C connection with BMP280
  Wire.beginTransmission(address_bmp280); // ALWAYS NEED TO DO TO START CONNECTION W ADDRESS
  Wire.write(0xD0);               // id request register
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(address_bmp280, 1); //asks to verify itself

  if (Wire.available()) {
      uint8_t chip_id = Wire.read(); // always def. vbl. type. 
      //"uint8_t" = Unsigned INTeger 8bits w/_T standard anmind condition
      if (chip_id == 0x58) {
      } else {
        return false;
    } } else {
      return false;
    }


  // read the temperature and pressure calibration values

  Wire.beginTransmission(address_bmp280); // Start communication with device
  Wire.write(0x88);                  // "Data readout is done by starting a burst read from 0xF7 to 0xFC"
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(address_bmp280, 24);
  if (Wire.available() == 24) {

    uint8_t msb_dig_T1, lsb_dig_T1, msb_dig_T2, lsb_dig_T2, msb_dig_T3, lsb_dig_T3, 
    msb_dig_P1, lsb_dig_P1, msb_dig_P2, lsb_dig_P2, msb_dig_P3, lsb_dig_P3, 
    msb_dig_P4, lsb_dig_P4, msb_dig_P5, lsb_dig_P5, msb_dig_P6, lsb_dig_P6, 
    msb_dig_P7, lsb_dig_P7, msb_dig_P8, lsb_dig_P8, msb_dig_P9, lsb_dig_P9;

    // Temperature coefficients
    lsb_dig_T1 = Wire.read();
    msb_dig_T1 = Wire.read();
    dig_T1 = ((uint16_t) msb_dig_T1 << 8) | lsb_dig_T1 ;

    lsb_dig_T2 = Wire.read(); // 0x8A
    msb_dig_T2 = Wire.read(); // 0x8B
    dig_T2 = (int16_t)(((uint16_t)msb_dig_T2 << 8) | lsb_dig_T2);

    lsb_dig_T3 = Wire.read(); // 0x8C
    msb_dig_T3 = Wire.read(); // 0x8D
    dig_T3 = (int16_t)(((uint16_t)msb_dig_T3 << 8) | lsb_dig_T3);

    // Pressure coefficients
    lsb_dig_P1 = Wire.read(); // 0x8E
    msb_dig_P1 = Wire.read(); // 0x8F
    dig_P1 = ((uint16_t)msb_dig_P1 << 8) | lsb_dig_P1;

    lsb_dig_P2 = Wire.read(); // 0x90
    msb_dig_P2 = Wire.read(); // 0x91
    dig_P2 = (int16_t)(((uint16_t)msb_dig_P2 << 8) | lsb_dig_P2);

    lsb_dig_P3 = Wire.read(); // 0x92
    msb_dig_P3 = Wire.read(); // 0x93x
    dig_P3 = (int16_t)(((uint16_t)msb_dig_P3 << 8) | lsb_dig_P3);

    lsb_dig_P4 = Wire.read(); // 0x94
    msb_dig_P4 = Wire.read(); // 0x95
    dig_P4 = (int16_t)(((uint16_t)msb_dig_P4 << 8) | lsb_dig_P4);

    lsb_dig_P5 = Wire.read(); // 0x96
    msb_dig_P5 = Wire.read(); // 0x97
    dig_P5 = (int16_t)(((uint16_t)msb_dig_P5 << 8) | lsb_dig_P5);

    lsb_dig_P6 = Wire.read(); // 0x98
    msb_dig_P6 = Wire.read(); // 0x99
    dig_P6 = (int16_t)(((uint16_t)msb_dig_P6 << 8) | lsb_dig_P6);

    lsb_dig_P7 = Wire.read(); // 0x9A
    msb_dig_P7 = Wire.read(); // 0x9B
    dig_P7 = (int16_t)(((uint16_t)msb_dig_P7 << 8) | lsb_dig_P7);

    lsb_dig_P8 = Wire.read(); // 0x9C
    msb_dig_P8 = Wire.read(); // 0x9D
    dig_P8 = (int16_t)(((uint16_t)msb_dig_P8 << 8) | lsb_dig_P8);

    lsb_dig_P9 = Wire.read(); // 0x9E
    msb_dig_P9 = Wire.read(); // 0x9F
    dig_P9 = (int16_t)(((uint16_t)msb_dig_P9 << 8) | lsb_dig_P9);
  } else {
    return false;
  }



  //4.3.4 Register 0xF4 “ctrl_meas” - setting up sensor res.
  Wire.beginTransmission(address_bmp280); 
  Wire.write(0xF4);   // address to set measure range
  Wire.write(0b00100111);   // 001 001 11 = lowest res T, lowest res P, normal mode
  return Wire.endTransmission() == 0; 

}


bool read(float &temperatureC, float &pressureHpa) {
  // read raw pressure and temperature, then run the datasheet conversions
  Wire.beginTransmission(address_bmp280); // Start communication with device
  Wire.write(0xF7);                  // "Data readout is done by starting a burst read from 0xF7 to 0xFC"
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(address_bmp280,6); // requests the 6 bytes after 0xF7 AKA P & T bytes
  if (Wire.available() == 6) { // verifies they are there
    uint8_t press_msb = Wire.read(); // 0xF7
    uint8_t press_lsb = Wire.read(); // 0xF8
    uint8_t press_xlsb = Wire.read(); // 0xF9
    uint8_t temp_msb = Wire.read(); // 0xFA
    uint8_t temp_lsb = Wire.read(); // 0xFB
    uint8_t temp_xlsb = Wire.read(); // 0xFC

    // combine the incoming bytes into raw pressure and temperature values
    BMP280_S32_t adc_P =
        ((BMP280_S32_t)press_msb << 12) |
        ((BMP280_S32_t)press_lsb << 4) |
        ((BMP280_S32_t)press_xlsb >> 4);

    BMP280_S32_t adc_T =
        ((BMP280_S32_t)temp_msb << 12) |
        ((BMP280_S32_t)temp_lsb << 4) |
        ((BMP280_S32_t)temp_xlsb >> 4);

    if (adc_T == 0x80000 || adc_P == 0x80000 || dig_P1 == 0) return false;
    // convert temperature first because pressure needs its correction value
    BMP280_S32_t temperature100 =
        bmp280_compensate_T_int32(adc_T);

    BMP280_U32_t pressureQ24_8 =
        bmp280_compensate_P_int64(adc_P);
    float temp_C = temperature100 / 100.0f; 
    temperatureC = temp_C;
    pressureHpa = pressureQ24_8 / 25600.0f;
    return pressureQ24_8 != 0;

  }
  return false; // No complete sensor reading.
}

} 

