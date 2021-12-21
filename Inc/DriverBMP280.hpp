/*
	Projekt ARM

	Autorzy:
	Mateusz Bielecki
	Grzegorz Mrozek
	Lukasz Plech
*/

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

#define RESET_VALUE 0xB6

// adresy rejestrow
#define REG_CALIB 0x88
#define REG_CALIB_HUM 0xA1
#define REG_ID 0xD0
#define REG_RESET 0xE0
#define REG_CTRL_HUM 0xF2
#define REG_STATUS 0xF3
#define REG_CONFIG 0xF5
#define REG_CTRL 0xF4
#define REG_PRESS_MSB 0xF7

// stale konfiguracyjne
#define MODE_NORMAL 3
#define FILTER_OFF 0
#define STANDARD 3
#define STANDBY_250 3

class DriverBMP280
{
public:

	// konstruktor sluzacy do zapisania wskaznika na handler i2c oraz adresu urzadzenia
    DriverBMP280(I2C_HandleTypeDef* hi2c, uint16_t deviceAddress);

	// Metoda sluzaca do inicjalizacji czujnika (reset, pobranie danych kalibracyjnych oraz ustawienie parametrow)
    bool Init();

	// Metoda sluzaca do odczytu temperatury, cisnienia oraz wilgotnosci
    bool ReadValues(float& temperature, float& pressure, float& humidity);

private:

	// Metoda sluzaca do odczytu pamieci czujnika
    bool ReadData(uint8_t registerAddress, uint8_t* value, size_t size);

	// Metoda sluzaca do odczytu pamieci czujnika
    bool WriteData(uint8_t registerAddress, uint8_t value, size_t size);

	// Metoda pobierajaca dane kalibracyjne z czujnika 
    bool ReadAndSaveCalibrationData();

	// obliczenia uzyte w metodzie zostaly zaczerpeniete z dokumentacji czujnika
    int32_t CompensateTemperature(int32_t adc_temp, int32_t& t_fine);

	// obliczenia uzyte w metodzie zostaly zaczerpeniete z dokumentacji czujnika
    uint32_t CompensatePressure(int32_t adc_press, int32_t t_fine);

	// obliczenia uzyte w metodzie zostaly zaczerpeniete z dokumentacji czujnika i innych zrodel
    uint32_t CompensateHumidity(int32_t adc_hum, int32_t t_fine);

	// struktura do zapisania danych kalibracyjnych
	struct BMP280_Calibration_Data
	{
		// rejestry kompensacji temperatury
		uint16_t dig_T1;
		int16_t dig_T2;
		int16_t dig_T3;

		// rejestry kompensacji cisnienia
		uint16_t dig_P1;
		int16_t dig_P2;
		int16_t dig_P3;
		int16_t dig_P4;
		int16_t dig_P5;
		int16_t dig_P6;
		int16_t dig_P7;
		int16_t dig_P8;
		int16_t dig_P9;

		// rejestry kompensacji wilgotnosci
		uint8_t dig_H1;
		int16_t dig_H2;
		uint8_t dig_H3;
		int16_t dig_H4;
		int16_t dig_H5;
		int8_t dig_H6;
	};
 
	// struktura zawierajaca dane kalibracyjne
    BMP280_Calibration_Data m_calibrationData;

	// handler i2c
    I2C_HandleTypeDef *p_hi2c;

	// adres urzadzenia
	uint16_t m_DeviceAddress;
};
