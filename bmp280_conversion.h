#ifndef BMP280_CONVERSION_H
#define BMP280_CONVERSION_H

#include <Arduino.h>

// "extern" = tells code that it exists in the .cpp
extern uint16_t dig_T1;
extern int16_t dig_T2;
extern int16_t dig_T3;

extern uint16_t dig_P1;
extern int16_t dig_P2;
extern int16_t dig_P3;
extern int16_t dig_P4;
extern int16_t dig_P5;
extern int16_t dig_P6;
extern int16_t dig_P7;
extern int16_t dig_P8;
extern int16_t dig_P9;

// ensures info stays as un/signed and 32 or 64 bit. 
typedef int32_t  BMP280_S32_t;
typedef uint32_t BMP280_U32_t;
typedef int64_t  BMP280_S64_t;

// decalring value type of each dig vbl. 

BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T);
BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P);

#endif