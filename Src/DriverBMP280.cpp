/*
	Projekt ARM

	Autorzy:
	Mateusz Bielecki
	Grzegorz Mrozek
	Lukasz Plech
*/

#include "DriverBMP280.hpp"

DriverBMP280::DriverBMP280(I2C_HandleTypeDef* hi2c, uint16_t deviceAddress) : p_hi2c(hi2c), m_DeviceAddress(deviceAddress << 1) {}

bool DriverBMP280::Init()
{
	if (!WriteData(REG_RESET, RESET_VALUE, 1))
	{
		return false;
	}

	while (1) // czekamy az dane z NVM zostana skopiowane do rejestrow
	{
		uint8_t status;
		if (ReadData(REG_STATUS, &status, 1) && !(status & 1))
			break;
	}

	if (!ReadAndSaveCalibrationData() ||
		!WriteData(REG_CONFIG, (STANDBY_250 << 5 | FILTER_OFF << 2), 1) ||
		!WriteData(REG_CTRL_HUM, STANDARD, 1) ||
		!WriteData(REG_CTRL, (MODE_NORMAL | STANDARD << 5 | STANDARD << 2), 1))
	{
		return false;
	}

	return true;
}

int32_t DriverBMP280::CompensateTemperature(int32_t adc_temp, int32_t& t_fine)
{
	int32_t var1, var2;

	var1 = ((((adc_temp >> 3) - ((int32_t)m_calibrationData.dig_T1 << 1))) * (int32_t)m_calibrationData.dig_T2) >> 11;
	var2 = (((((adc_temp >> 4) - (int32_t)m_calibrationData.dig_T1) * ((adc_temp >> 4) - (int32_t)m_calibrationData.dig_T1)) >> 12) * (int32_t)m_calibrationData.dig_T3) >> 14;

	t_fine = var1 + var2;
	return (t_fine * 5 + 128) >> 8;
}

uint32_t DriverBMP280::CompensatePressure(int32_t adc_press, int32_t t_fine)
{
	int64_t var1, var2, p;

	var1 = (int64_t)t_fine - 128000;
	var2 = var1 * var1 * (int64_t)m_calibrationData.dig_P6;
	var2 = var2 + ((var1 * (int64_t)m_calibrationData.dig_P5) << 17);
	var2 = var2 + (((int64_t)m_calibrationData.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)m_calibrationData.dig_P3) >> 8) + ((var1 * (int64_t)m_calibrationData.dig_P2) << 12);
	var1 = (((int64_t)1 << 47) + var1) * ((int64_t)m_calibrationData.dig_P1) >> 33;

	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}

	p = 1048576 - adc_press;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = ((int64_t)m_calibrationData.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = ((int64_t)m_calibrationData.dig_P8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + ((int64_t)m_calibrationData.dig_P7 << 4);
	return p;
}

uint32_t DriverBMP280::CompensateHumidity(int32_t adc_hum, int32_t t_fine)
{
	int32_t var;

	var = t_fine - (int32_t)76800;
	var = ((((adc_hum << 14) - ((int32_t)m_calibrationData.dig_H4 << 20) - ((int32_t)m_calibrationData.dig_H5 * var)) + (int32_t)16384) >> 15) * (((((((var * (int32_t)m_calibrationData.dig_H6) >> 10) * (((var * (int32_t)m_calibrationData.dig_H3) >> 11) + (int32_t)32768)) >> 10) + (int32_t)2097152) * (int32_t)m_calibrationData.dig_H2 + 8192) >> 14);
	var = var - (((((var >> 15) * (var >> 15)) >> 7) * (int32_t)m_calibrationData.dig_H1) >> 4);
	var = var < 0 ? 0 : var;
	var = var > 419430400 ? 419430400 : var;
	return var >> 12;
}

bool DriverBMP280::ReadValues(float& temperature, float& pressure, float& humidity)
{
	uint8_t data[8];

	if (!ReadData(REG_PRESS_MSB, data, sizeof(data))) // sczytujemy na raz rejestry cisnienia, temperatury i wilgotnosci
	{
		return false;
	}

	int32_t t_fine;
	int32_t temp = data[3] << 12 | data[4] << 4 | data[5] >> 4;
	int32_t press = data[0] << 12 | data[1] << 4 | data[2] >> 4;
	int32_t hum = data[6] << 8 | data[7];

	temperature = CompensateTemperature(temp, t_fine) / 100.0f;
	pressure = CompensatePressure(press, t_fine) / 256.0;
	humidity = CompensateHumidity(hum, t_fine) / 1024.0;

	return true;
}


bool DriverBMP280::WriteData(uint8_t registerAddress, uint8_t value, size_t size)
{
	return (HAL_I2C_Mem_Write(p_hi2c, m_DeviceAddress, registerAddress, 1, &value, size, 5000) == HAL_OK);
}

bool DriverBMP280::ReadData(uint8_t registerAddress, uint8_t* value, size_t size)
{
	return (HAL_I2C_Mem_Read(p_hi2c, m_DeviceAddress, registerAddress, 1, value, size, 5000) == HAL_OK);
}

bool DriverBMP280::ReadAndSaveCalibrationData()
{
	uint16_t h4, h5;
	if (
		ReadData(REG_CALIB, reinterpret_cast<uint8_t*>(&m_calibrationData.dig_T1), 2) && 
		ReadData(REG_CALIB + 2,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_T2), 2) && 
		ReadData(REG_CALIB + 4,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_T3), 2) && 
		ReadData(REG_CALIB + 6,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P1), 2) && 
		ReadData(REG_CALIB + 8,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P2), 2) && 
		ReadData(REG_CALIB + 10,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P3), 2) && 
		ReadData(REG_CALIB + 12,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P4), 2) && 
		ReadData(REG_CALIB + 14,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P5), 2) && 
		ReadData(REG_CALIB + 16,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P6), 2) && 
		ReadData(REG_CALIB + 18,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P7), 2) && 
		ReadData(REG_CALIB + 20,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P8), 2) && 
		ReadData(REG_CALIB + 22,  reinterpret_cast<uint8_t*>(&m_calibrationData.dig_P9), 2) && 
		ReadData(REG_CALIB_HUM, &m_calibrationData.dig_H1, 1) && 
		ReadData(REG_CALIB_HUM + 64, reinterpret_cast<uint8_t*>(&m_calibrationData.dig_H2), 2) && 
		ReadData(REG_CALIB_HUM + 66, &m_calibrationData.dig_H3, 1) && 
		ReadData(REG_CALIB_HUM + 67, reinterpret_cast<uint8_t*>(&h4), 2) && 
		ReadData(REG_CALIB_HUM + 68, reinterpret_cast<uint8_t*>(&h5), 2) && 
		ReadData(REG_CALIB_HUM + 70, reinterpret_cast<uint8_t*>(&m_calibrationData.dig_H6), 1))
	{
		m_calibrationData.dig_H4 = (h4 & 0x00FF) << 4 | (h4 & 0x0F00) >> 8;
		m_calibrationData.dig_H5 = h5 >> 4;
		return true;
	}

	return false;
}

