#include "oled_text.h"
#include <string.h>

#define OLED_I2C_ADDRESS (0x3C << 1)
#define OLED_WIDTH       128
#define OLED_HEIGHT      64
#define OLED_PAGES       8

static I2C_HandleTypeDef *oledI2C;

static uint8_t oledBuffer[OLED_WIDTH * OLED_PAGES];
static uint8_t cursorX;
static uint8_t cursorPage;

static const uint8_t numberFont[10][5] =
{
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

static const uint8_t letterFont[26][5] =
{
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

static void OLED_SendCommand(uint8_t command)
{
  HAL_I2C_Mem_Write(
      oledI2C,
      OLED_I2C_ADDRESS,
      0x00,
      I2C_MEMADD_SIZE_8BIT,
      &command,
      1,
      100
  );
}

static void OLED_GetCharacter(char character, uint8_t output[5])
{
  memset(output, 0, 5);

  if ((character >= '0') && (character <= '9'))
  {
    memcpy(output, numberFont[character - '0'], 5);
  }
  else if ((character >= 'A') && (character <= 'Z'))
  {
    memcpy(output, letterFont[character - 'A'], 5);
  }
  else
  {
    switch (character)
    {
      case '-':
        output[0] = 0x08;
        output[1] = 0x08;
        output[2] = 0x08;
        output[3] = 0x08;
        output[4] = 0x08;
        break;

      case '+':
        output[0] = 0x08;
        output[1] = 0x08;
        output[2] = 0x3E;
        output[3] = 0x08;
        output[4] = 0x08;
        break;

      case ':':
        output[1] = 0x36;
        output[2] = 0x36;
        break;

      case '.':
        output[1] = 0x60;
        output[2] = 0x60;
        break;

      case '/':
        output[0] = 0x20;
        output[1] = 0x10;
        output[2] = 0x08;
        output[3] = 0x04;
        output[4] = 0x02;
        break;

      case ' ':
      default:
        break;
    }
  }
}

void OLED_Text_Init(I2C_HandleTypeDef *i2cHandle)
{
  oledI2C = i2cHandle;

  HAL_Delay(100);

  OLED_SendCommand(0xAE);
  OLED_SendCommand(0x20);
  OLED_SendCommand(0x00);
  OLED_SendCommand(0xB0);
  OLED_SendCommand(0xC8);
  OLED_SendCommand(0x00);
  OLED_SendCommand(0x10);
  OLED_SendCommand(0x40);
  OLED_SendCommand(0x81);
  OLED_SendCommand(0xFF);
  OLED_SendCommand(0xA1);
  OLED_SendCommand(0xA6);
  OLED_SendCommand(0xA8);
  OLED_SendCommand(0x3F);
  OLED_SendCommand(0xA4);
  OLED_SendCommand(0xD3);
  OLED_SendCommand(0x00);
  OLED_SendCommand(0xD5);
  OLED_SendCommand(0xF0);
  OLED_SendCommand(0xD9);
  OLED_SendCommand(0x22);
  OLED_SendCommand(0xDA);
  OLED_SendCommand(0x12);
  OLED_SendCommand(0xDB);
  OLED_SendCommand(0x20);
  OLED_SendCommand(0x8D);
  OLED_SendCommand(0x14);
  OLED_SendCommand(0xAF);

  OLED_Text_Clear();
  OLED_Text_Update();
}

void OLED_Text_Clear(void)
{
  memset(oledBuffer, 0, sizeof(oledBuffer));
  cursorX = 0;
  cursorPage = 0;
}

void OLED_Text_SetCursor(uint8_t x, uint8_t page)
{
  cursorX = x;
  cursorPage = page;
}

void OLED_Text_WriteChar(char character)
{
  uint8_t characterData[5];

  if ((cursorX > 122) || (cursorPage >= OLED_PAGES))
  {
    return;
  }

  OLED_GetCharacter(character, characterData);

  for (uint8_t column = 0; column < 5; column++)
  {
    oledBuffer[(cursorPage * OLED_WIDTH) + cursorX] =
        characterData[column];

    cursorX++;
  }

  oledBuffer[(cursorPage * OLED_WIDTH) + cursorX] = 0x00;
  cursorX++;
}

void OLED_Text_WriteString(const char *text)
{
  while (*text != '\0')
  {
    OLED_Text_WriteChar(*text);
    text++;
  }
}

void OLED_Text_Update(void)
{
  OLED_SendCommand(0x21);
  OLED_SendCommand(0x00);
  OLED_SendCommand(0x7F);

  OLED_SendCommand(0x22);
  OLED_SendCommand(0x00);
  OLED_SendCommand(0x07);

  HAL_I2C_Mem_Write(
      oledI2C,
      OLED_I2C_ADDRESS,
      0x40,
      I2C_MEMADD_SIZE_8BIT,
      oledBuffer,
      sizeof(oledBuffer),
      1000
  );
}
