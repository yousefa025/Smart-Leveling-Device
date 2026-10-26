#ifndef OLED_TEXT_H
#define OLED_TEXT_H

#include "stm32f3xx_hal.h"
#include <stdint.h>

void OLED_Text_Init(I2C_HandleTypeDef *i2cHandle);
void OLED_Text_Clear(void);
void OLED_Text_SetCursor(uint8_t x, uint8_t page);
void OLED_Text_WriteChar(char character);
void OLED_Text_WriteString(const char *text);
void OLED_Text_Update(void);

#endif
