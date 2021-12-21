#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

#define BMP280_REG_TEMP_XLSB 0xFC /* bits: 7-4 */
#define BMP280_REG_TEMP_LSB 0xFB
#define BMP280_REG_TEMP_MSB 0xFA
#define BMP280_REG_TEMP (BMP280_REG_TEMP_MSB)
#define BMP280_REG_PRESS_XLSB 0xF9 /* bits: 7-4 */
#define BMP280_REG_PRESS_LSB 0xF8
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_PRESSURE (BMP280_REG_PRESS_MSB)
#define BMP280_REG_CONFIG 0xF5   /* bits: 7-5 t_sb; 4-2 filter; 0 spi3w_en */
#define BMP280_REG_CTRL 0xF4     /* bits: 7-5 osrs_t; 4-2 osrs_p; 1-0 mode */
#define BMP280_REG_STATUS 0xF3   /* bits: 3 measuring; 0 im_update */
#define BMP280_REG_CTRL_HUM 0xF2 /* bits: 2-0 osrs_h; */
#define BMP280_REG_RESET 0xE0
#define BMP280_REG_ID 0xD0
#define BMP280_REG_CALIB 0x88
#define BMP280_REG_HUM_CALIB 0x88

#define BMP280_CHIP_ID 0x58
#define BMP280_RESET_VALUE 0xB6


class BMP280
{
public:
    BMP280(I2C_HandleTypeDef *hi2c, uint16_t deviceAddress) : p_hi2c(hi2c), m_DeviceAddress(deviceAddress << 1) {}
    
	bool Init(uint8_t* chipId)
    {
		if (!WriteData(BMP280_REG_RESET, BMP280_RESET_VALUE, 1))
		{
			return false;
    	}
		
		if (!ReadData(BMP280_REG_ID, chipId, 1))
		{
			return false;
		}
		
		while (1) {
		uint8_t status;
		if (ReadData(BMP280_REG_STATUS, &status, 1) && (status & 1) == 0)
			break;
		}
		
		if(!ReadCalibrationData()) {
			return false;
		}
		
		if (!WriteData(0xF5, ( 3 << 5 | 0 << 2 ), 1))
		{
			return false;
    	}
		
		if (!WriteData(0xF2, 3, 1)) {
			return false;
		}
		
		if (!WriteData(0xF4, ( 3 | 3 << 5 | 3 << 2 ), 1))
		{
			return false;
    }
		
		return true;
		}
    bool ReadValues(float *temperature, float *pressure, float *humidity)
    {
        uint8_t data[8];

        if (!ReadData(0xf7, data, sizeof(data)))
        {
            return false;
        }

		int32_t t_fine;
		int32_t temp = data[3] << 12 | data[4] << 4 | data[5] >> 4;
		int32_t press = data[0] << 12 | data[1] << 4 | data[2] >> 4;
		int32_t hum = data[6] << 8 | data[7];
        
		*temperature = CompensateTemperature(temp, &t_fine) / 100.0f;
        *pressure = CompensatePressure(press, t_fine) / 256.0;
        *humidity = CompensateHumidity(hum, t_fine) / 1024.0;

        return true;
    }

		int32_t CompensateTemperature(int32_t adc_temp, int32_t* t_fine) {
	int32_t var1, var2;

	var1 = ((((adc_temp >> 3) - ((int32_t) m_calibrationData.dig_T1 << 1)))
			* (int32_t) m_calibrationData.dig_T2) >> 11;
	var2 = (((((adc_temp >> 4) - (int32_t) m_calibrationData.dig_T1)
			* ((adc_temp >> 4) - (int32_t) m_calibrationData.dig_T1)) >> 12)
			* (int32_t) m_calibrationData.dig_T3) >> 14;

	*t_fine = var1 + var2;
	return (*t_fine * 5 + 128) >> 8;
}
		
uint32_t CompensatePressure(int32_t adc_press, int32_t t_fine) {
	int64_t var1, var2, p;

	var1 = (int64_t) t_fine - 128000;
	var2 = var1 * var1 * (int64_t) m_calibrationData.dig_P6;
	var2 = var2 + ((var1 * (int64_t) m_calibrationData.dig_P5) << 17);
	var2 = var2 + (((int64_t) m_calibrationData.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t) m_calibrationData.dig_P3) >> 8)
			+ ((var1 * (int64_t) m_calibrationData.dig_P2) << 12);
	var1 = (((int64_t) 1 << 47) + var1) * ((int64_t) m_calibrationData.dig_P1) >> 33;

	if (var1 == 0) {
		return 0;  // avoid exception caused by division by zero
	}

	p = 1048576 - adc_press;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = ((int64_t) m_calibrationData.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = ((int64_t) m_calibrationData.dig_P8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + ((int64_t) m_calibrationData.dig_P7 << 4);
	return p;
}

uint32_t CompensateHumidity(int32_t adc_hum, int32_t t_fine) {
	int32_t v_x1_u32r;

	v_x1_u32r = t_fine - (int32_t) 76800;
	v_x1_u32r = ((((adc_hum << 14) - ((int32_t) m_calibrationData.dig_H4 << 20)
			- ((int32_t) m_calibrationData.dig_H5 * v_x1_u32r)) + (int32_t) 16384) >> 15)
			* (((((((v_x1_u32r * (int32_t) m_calibrationData.dig_H6) >> 10)
					* (((v_x1_u32r * (int32_t) m_calibrationData.dig_H3) >> 11)
							+ (int32_t) 32768)) >> 10) + (int32_t) 2097152)
					* (int32_t) m_calibrationData.dig_H2 + 8192) >> 14);
	v_x1_u32r = v_x1_u32r
			- (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
					* (int32_t) m_calibrationData.dig_H1) >> 4);
	v_x1_u32r = v_x1_u32r < 0 ? 0 : v_x1_u32r;
	v_x1_u32r = v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r;
	return v_x1_u32r >> 12;
}


private:
	
struct BMP280_Calibration_Data
{
        uint16_t dig_T1;
        int16_t  dig_T2;
        int16_t  dig_T3;
    
        uint16_t dig_P1;
        int16_t  dig_P2;
        int16_t  dig_P3;
        int16_t  dig_P4;
        int16_t  dig_P5;
        int16_t  dig_P6;
        int16_t  dig_P7;
        int16_t  dig_P8;
        int16_t  dig_P9;
    
        uint8_t  dig_H1;
        int16_t  dig_H2;
        uint8_t  dig_H3;
        int16_t  dig_H4;
        int16_t  dig_H5;
        int8_t   dig_H6;
    
} m_calibrationData;

	  bool WriteData(uint8_t registerAddress, uint8_t value, size_t size)
		{
        return (HAL_I2C_Mem_Write(p_hi2c, m_DeviceAddress, registerAddress, 1, &value, size, 5000) == HAL_OK);
		}

    bool ReadData(uint8_t registerAddress, uint8_t *value, size_t size)
		{
        return (HAL_I2C_Mem_Read(p_hi2c, m_DeviceAddress, registerAddress, 1, value, size, 5000) == HAL_OK);
		}
		
		bool ReadData(uint8_t registerAddress, uint16_t *value)
		{
			uint8_t rx_buff[2];
      if(HAL_I2C_Mem_Read(p_hi2c, m_DeviceAddress, registerAddress, 1, rx_buff, 2, 5000) == HAL_OK) {
				*value = (uint16_t) ((rx_buff[1] << 8) | rx_buff[0]);
				return true;
			}
			return false;
		}
		
		bool ReadCalibrationData()
		{
			uint16_t h4, h5;
				if(ReadData(BMP280_REG_HUM_CALIB, &m_calibrationData.dig_T1) 
					&& ReadData(0x8a, (uint16_t *)&m_calibrationData.dig_T2) 
					&& ReadData(0x8c, (uint16_t *)&m_calibrationData.dig_T3)
				&& ReadData(0x8e, &m_calibrationData.dig_P1)
				&& ReadData(0x90, (uint16_t *)&m_calibrationData.dig_P2)
				&& ReadData(0x92, (uint16_t *)&m_calibrationData.dig_P3)
				&& ReadData(0x94, (uint16_t *)&m_calibrationData.dig_P4)
				&& ReadData(0x96, (uint16_t *)&m_calibrationData.dig_P5)
				&& ReadData(0x98, (uint16_t *)&m_calibrationData.dig_P6)
				&& ReadData(0x9a, (uint16_t *)&m_calibrationData.dig_P7)
				&& ReadData(0x9c, (uint16_t *)&m_calibrationData.dig_P8)
				&& ReadData(0x9e, (uint16_t *)&m_calibrationData.dig_P9)
				
				&& ReadData(0xa1, &m_calibrationData.dig_H1, 1)
				&& ReadData(0xe1, (uint16_t *)&m_calibrationData.dig_H2)
				&& ReadData(0xe3, &m_calibrationData.dig_H3, 1)
				&& ReadData(0xe4, &h4)
				&& ReadData(0xe5, &h5)
				&& ReadData(0xe7, (uint8_t *)&m_calibrationData.dig_H6, 1))
					{
						m_calibrationData.dig_H4 = (h4 & 0x00ff) << 4 | (h4 & 0x0f00) >> 8;
						m_calibrationData.dig_H5 = h5 >> 4;
						
						return true;
					}
					
					return false;
		}

    I2C_HandleTypeDef *p_hi2c;
    uint16_t m_DeviceAddress;
};
