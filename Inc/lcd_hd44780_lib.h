#ifndef LCD_HD44780_LIB_H
#define LCD_HD44780_LIB_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "main.h"


#define HD44780_CLEAR					0x01

#define HD44780_HOME					0x02

#define HD44780_ENTRY_MODE				0x04
	#define HD44780_EM_SHIFT_CURSOR		0
	#define HD44780_EM_SHIFT_DISPLAY	1
	#define HD44780_EM_DECREMENT		0
	#define HD44780_EM_INCREMENT		2

#define HD44780_DISPLAY_ONOFF			0x08
	#define HD44780_DISPLAY_OFF		    0
	#define HD44780_DISPLAY_ON			4
	#define HD44780_CURSOR_OFF			0
	#define HD44780_CURSOR_ON			2
	#define HD44780_CURSOR_NOBLINK	    0
	#define HD44780_CURSOR_BLINK		1

#define HD44780_DISPLAY_CURSOR_SHIFT    0x10
	#define HD44780_SHIFT_CURSOR		0
	#define HD44780_SHIFT_DISPLAY		8
	#define HD44780_SHIFT_LEFT			0
	#define HD44780_SHIFT_RIGHT		    4

#define HD44780_FUNCTION_SET			0x20
	#define HD44780_FONT5x7				0
	#define HD44780_FONT5x10			4
	#define HD44780_ONE_LINE			0
	#define HD44780_TWO_LINE			8
	#define HD44780_4_BIT				0
	#define HD44780_8_BIT				16

#define HD44780_CGRAM_SET				0x40

#define HD44780_DDRAM_SET				0x80

void LCD_Initialize(void);
void LCD_Clear(void);
void LCD_WriteData(uint8_t dataToWrite);
void LCD_WriteCommand(uint8_t commandToWrite);
void LCD_WriteText(uint8_t* text);
void LCD_WriteTextXY(uint8_t* text, uint8_t x, uint8_t y);
void LCD_GoTo(uint8_t x, uint8_t y);
void LCD_WriteBinary(uint8_t var, uint8_t bitCount);
void LCD_SetUserChar(uint8_t chrNum, uint8_t n, const uint8_t* p);

#ifdef __cplusplus
}
#endif

#endif//LCD_HD44780_LIB_H
