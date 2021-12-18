
#include "lcd_hd44780_lib.h"

GPIO_InitTypeDef GPIO_InitStructure;

//-----------------------------------------------------------------------------
void LCD_WriteNibble(uint8_t nibbleToWrite)
{
  HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (GPIO_PinState)(nibbleToWrite & 0x01));
  HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (GPIO_PinState)(nibbleToWrite & 0x02));
  HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (GPIO_PinState)(nibbleToWrite & 0x04));
  HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (GPIO_PinState)(nibbleToWrite & 0x08));
  HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
}


//-----------------------------------------------------------------------------
uint8_t LCD_ReadNibble(void)
{
  uint8_t tmp = 0;
  HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
  tmp |= (HAL_GPIO_ReadPin(LCD_D4_GPIO_Port, LCD_D4_Pin) << 0);
  tmp |= (HAL_GPIO_ReadPin(LCD_D5_GPIO_Port, LCD_D5_Pin) << 1);
  tmp |= (HAL_GPIO_ReadPin(LCD_D6_GPIO_Port, LCD_D6_Pin) << 2);
  tmp |= (HAL_GPIO_ReadPin(LCD_D7_GPIO_Port, LCD_D7_Pin) << 3);
  HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
  return tmp;
}


//-----------------------------------------------------------------------------
uint8_t LCD_ReadStatus(void)
{
  unsigned char status = 0;
  
  GPIO_InitStructure.Pin   =  LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin | LCD_D7_Pin;
  GPIO_InitStructure.Mode  =  GPIO_MODE_INPUT;
  HAL_GPIO_Init(LCD_GPIO_Port, &GPIO_InitStructure);
  
  HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
  
  status |= (LCD_ReadNibble() << 4);
  status |= LCD_ReadNibble();
  
  GPIO_InitStructure.Pin   =  LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin | LCD_D7_Pin;
  GPIO_InitStructure.Mode  =  GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(LCD_GPIO_Port, &GPIO_InitStructure);
  
  return status;
}


//-----------------------------------------------------------------------------
void LCD_WriteData(uint8_t dataToWrite)
{
  HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
  
  LCD_WriteNibble(dataToWrite >> 4);
  LCD_WriteNibble(dataToWrite & 0x0F);
  
  while(LCD_ReadStatus() & 0x80);
}


//-----------------------------------------------------------------------------
void LCD_WriteCommand(uint8_t commandToWrite)
{
  HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);

  LCD_WriteNibble(commandToWrite >> 4);
  LCD_WriteNibble(commandToWrite & 0x0F);
  
  while(LCD_ReadStatus() & 0x80);
}


//-----------------------------------------------------------------------------
void LCD_WriteText(uint8_t* text)
{
  while(*text)
    LCD_WriteData(*text++);
}


//-----------------------------------------------------------------------------
void LCD_GoTo(uint8_t x, uint8_t y)
{
  LCD_WriteCommand(HD44780_DDRAM_SET | (x + (0x40 * y)));
}


//-----------------------------------------------------------------------------
void LCD_WriteTextXY(uint8_t* text, uint8_t x, uint8_t y)
{
  LCD_GoTo(x,y);
  while(*text)
    LCD_WriteData(*text++);
}


//-----------------------------------------------------------------------------
void LCD_WriteBinary(uint8_t var, uint8_t bitCount)
{
  int8_t i;
  
  for(i = (bitCount - 1); i >= 0; i--)
     {
     LCD_WriteData((var & (1 << i))?'1':'0');
     }
}


//-----------------------------------------------------------------------------
void LCD_ShiftLeft(void)
{
  LCD_WriteCommand(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_LEFT | HD44780_SHIFT_DISPLAY);
}


//-----------------------------------------------------------------------------
void LCD_ShiftRight(void)
{
  LCD_WriteCommand(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_RIGHT | HD44780_SHIFT_DISPLAY);
}


//-----------------------------------------------------------------------------
void LCD_Initialize(void)
{
  volatile unsigned char i = 0;
  volatile unsigned int delayCnt = 0;
  

  GPIO_InitStructure.Pin   =  LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin | LCD_D7_Pin | 
                              LCD_RS_Pin | LCD_RW_Pin | LCD_E_Pin;
  GPIO_InitStructure.Mode  =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed =  GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_GPIO_Port, &GPIO_InitStructure);
  
  HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
  
  //for(delayCnt = 0; delayCnt < 300000; delayCnt++);
  HAL_Delay(10);
  
  for(i = 0; i < 3; i++) {
    LCD_WriteNibble(0x03);            
    //for(delayCnt = 0; delayCnt < 30000; delayCnt++);
    HAL_Delay(10);
  }
  
  LCD_WriteNibble(0x02);             
  
  //for(delayCnt = 0; delayCnt < 6000; delayCnt++);
  HAL_Delay(10);
     
  LCD_WriteCommand(HD44780_FUNCTION_SET | 
                   HD44780_FONT5x7 | 
                   HD44780_TWO_LINE | 
                   HD44780_4_BIT);
  
  LCD_WriteCommand(HD44780_DISPLAY_ONOFF | 
                   HD44780_DISPLAY_OFF); 
  
  LCD_WriteCommand(HD44780_CLEAR); 
  
  LCD_WriteCommand(HD44780_ENTRY_MODE | 
                   HD44780_EM_SHIFT_CURSOR | 
                   HD44780_EM_INCREMENT);
  
  LCD_WriteCommand(HD44780_DISPLAY_ONOFF | 
                   HD44780_DISPLAY_ON |
                   HD44780_CURSOR_OFF | 
                   HD44780_CURSOR_NOBLINK);
}


//-----------------------------------------------------------------------------
void LCD_SetUserChar(uint8_t chrNum, uint8_t n, const uint8_t* p)
{         //chrNum  - character number (code) to be registered (0..7)
          //n       - number of characters to register
          //*p      - pointer to the character pattern (8 * n bytes)
	LCD_WriteCommand(HD44780_CGRAM_SET | chrNum * 8);
	n *= 8;
	do
		LCD_WriteData(*p++);
	while (--n);
}


//-----------------------------------------------------------------------------
void LCD_Clear(void)
{
    LCD_WriteCommand(HD44780_CLEAR); 
}
