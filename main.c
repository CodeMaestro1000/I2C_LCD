#include "LCD_I2C.h"

int main () {
  LCD_I2C lcd;
	LCDI2CInit(&lcd, 0x27, 16, 2);
	backlight(&lcd);
	const char* s = "Hello World";
	print(&lcd, s);
  while(1) {
    
  }
}
