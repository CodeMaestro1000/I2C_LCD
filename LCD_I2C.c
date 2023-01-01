#include <string.h>
#include "LCD_I2C.h"
#include "driverlib/i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "tm4c123gh6pm.h"

//----------- helper functions ----------------
size_t write(LCD_I2C *display, uint8_t value){
	send(display, value, Rs);
	return 1;
}

size_t write_string(LCD_I2C *display, const uint8_t *buffer, size_t size){
	size_t n = 0;
	while (size--) {
    if (write(display, *buffer++)) n++;
    else break;
  }
  return n;
}


//-------------- LCD_Init Function ----------------------------------
/*
Initializes LCD_I2C object with I2C Address, number of rows and columns
Also Initializes System clock to run at 80MHz and Systick_Timer 
(this was included because we need systick for accurate delays)
*/
void LCDI2CInit(LCD_I2C *display, uint8_t lcd_address, uint8_t lcd_cols, uint8_t lcd_rows){
	PLL_Init80MHz();
	SysTick_Init();
	display->_Addr = lcd_address;
	display->_cols = lcd_cols;
	display->_rows = lcd_rows;
	display->_backlightval = LCD_NOBACKLIGHT;
	
	display->_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(display);
}

void begin(LCD_I2C *display){
	InitI2C0();
	display->_numlines = 2;
	display->_displayfunction = LCD_2LINE;
	
	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	SysTick_Wait10ms(5);
	
	// Now we pull both RS and R/W low to begin commands
	expanderWrite(display, display->_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
	SysTick_Wait10ms(100);

  //put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	// we start in 8bit mode, try to set 4 bit mode
  write4bits(display, 0x03 << 4);
  SysTick_Wait10us(450); // wait min 4.1ms
   
  // second try
  write4bits(display, 0x03 << 4);
  SysTick_Wait10us(450); // wait min 4.1ms
   
  // third go!
  write4bits(display, 0x03 << 4); 
  SysTick_Wait10us(15);
   
  // finally, set to 4-bit interface
  write4bits(display, 0x02 << 4); 


	// set # lines, font size, etc.
	command(display, LCD_FUNCTIONSET | display->_displayfunction);  
	
	// turn the display on with no cursor or blinking default
	display->_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	configDisplay(display);
	
	// clear it off
	clear(display);
	
	// Initialize to default text direction (for roman languages)
	display->_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(display, LCD_ENTRYMODESET | display->_displaymode);
	
	home(display);
}

void configDisplay(LCD_I2C *display) {
	display->_displaycontrol |= LCD_DISPLAYON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void clear(LCD_I2C *display){
	command(display, LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	SysTick_Wait10us(200);  // this command takes a long time!
}

void home(LCD_I2C *display){
	command(display, LCD_RETURNHOME);  // set cursor position to zero
	SysTick_Wait10us(200);  // this command takes a long time!
}

// Turns the underline cursor on/off
void noCursor(LCD_I2C *display) {
	display->_displaycontrol &= ~LCD_CURSORON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}
void cursor(LCD_I2C *display) {
	display->_displaycontrol |= LCD_CURSORON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void backlight(LCD_I2C *display) {
	display->_backlightval=LCD_BACKLIGHT;
	expanderWrite(display, 0);
}

void noBacklight(LCD_I2C *display) {
	display->_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(display, 0);
}

void noBlink(LCD_I2C *display) {
	display->_displaycontrol &= ~LCD_BLINKON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}

void blink(LCD_I2C *display) {
	display->_displaycontrol |= LCD_BLINKON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(LCD_I2C *display) {
	command(display, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(LCD_I2C *display) {
	command(display, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Turn the display on/off (quickly)
void noDisplay(LCD_I2C *display) {
	display->_displaycontrol &= ~LCD_DISPLAYON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}
void display(LCD_I2C *display) {
	display->_displaycontrol |= LCD_DISPLAYON;
	command(display, LCD_DISPLAYCONTROL | display->_displaycontrol);
}

// This is for text that flows Left to Right
void leftToRight(LCD_I2C *display) {
	display->_displaymode |= LCD_ENTRYLEFT;
	command(display, LCD_ENTRYMODESET | display->_displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(LCD_I2C *display) {
	display->_displaymode &= ~LCD_ENTRYLEFT;
	command(display, LCD_ENTRYMODESET | display->_displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(LCD_I2C *display) {
	display->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(display, LCD_ENTRYMODESET | display->_displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(LCD_I2C *display) {
	display->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(display, LCD_ENTRYMODESET | display->_displaymode);
}

size_t printChar(LCD_I2C *display, const char c){
	return write(display, c);
}

size_t print(LCD_I2C *display, const char* str){
	return write_string(display, (const uint8_t *)str, strlen(str)); // beware of casting!!! Type safe in this case :)
}

//*********** Low-level LCD functions **************

void command(LCD_I2C *display, uint8_t value) {
	send(display, value, 0);
}

void send(LCD_I2C *display, uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
  write4bits(display, (highnib)|mode);
	write4bits(display, (lownib)|mode); 
}

void write4bits(LCD_I2C *display, uint8_t value) {
	expanderWrite(display, value);
	pulseEnable(display, value);
}

void expanderWrite(LCD_I2C *display, uint8_t _data){                                        
	I2CSendByte(display->_Addr, (int)_data | display->_backlightval);   
}

void pulseEnable(LCD_I2C *display, uint8_t _data){
	expanderWrite(display, _data | En);	// En high
	SysTick_Wait_us(1);		// enable pulse must be >450ns
	
	expanderWrite(display, _data & ~En);	// En low
	SysTick_Wait10us(5);		// commands need > 37us to settle
}


//********* Low-level I2C Functions *****************
//initialize I2C module 0
//Slightly modified version of TI's example code
void InitI2C0(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
 
    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
     
    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
 
    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
     
    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
 
    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);
     
    //clear I2C FIFOs
    HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
}

//sends one byte of data to slave
void I2CSendByte(uint8_t slave_addr, uint8_t value){
	I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false); // set address of slave for master to communicate with
	//put data to be sent into FIFO
  I2CMasterDataPut(I2C0_BASE, value);
	//Initiate send of data from the MCU
  I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
         
  // Wait until MCU is done transferring.
  while(I2CMasterBusy(I2C0_BASE));
}

// ----------- Systick and PLL Sections ---------------------------
// Stores the number of ticks in 10ms based on the bus clock frequency.
//	Calculated inside SysTick_Init.
static uint32_t NumTicksPer10ms;
static uint32_t NumTicksPer10us;
static uint32_t NumTicksPer_us;
static uint32_t busClockFreq_;

void PLL_Init80MHz(void)
{
	
	// 0) Set USERCC2 bit in order to use RCC2
	SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;
	
	// 1) Set the BYPASS bit to bypass PLL while initializing.
	SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;

	// 2) Clear and set the XTAL bits to 16MHz
	SYSCTL_RCC_R = ((SYSCTL_RCC_R & ~SYSCTL_RCC_XTAL_M) | SYSCTL_RCC_XTAL_16MHZ);
	
	// 3)	Clear the OSCSRC2 bits to configure for Main OSC
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;
	
	// 4) Activate PLL by clearing PWRDN.
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;
	
	// 5) Use 400 MHz PLL
	SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400 ;
	
	// 6) Clear the SYSDIV2 bits and set to 0x4 = 80MHz
	SYSCTL_RCC2_R = (SYSCTL_RCC2_R & ~(SYSCTL_RCC2_SYSDIV2_M | SYSCTL_RCC2_SYSDIV2LSB)) | (0x4 << 22);  
	
	// 7) Wait for the PLL to lock
	while ((SYSCTL_RIS_R & SYSCTL_RIS_PLLLRIS) == 0)
	{
	}
	
	// 8) Clear BYPASS to enable PLL.
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;
	
	busClockFreq_ = 80000000;
	
}

//----------------------- PLL_GetBusClockFreq ------------------------
// Gets the PLL bus clock frequency.
// Inputs:  none.
// Outputs:  the configured bus clock frequency or zero if uninitialized.
uint32_t PLL_GetBusClockFreq(void)
{
	return busClockFreq_;
}

//----------------------- SysTick_Init --------------------------
// Initializes the SysTick bus clock timer.
// Inputs:  none.
// Outputs:  none.
int SysTick_Init(void)
{
	uint32_t busClockFreq = PLL_GetBusClockFreq();
	
	// Return failure if the PLL hasn't been initialized.
	if (busClockFreq == 0) {
		return -1;
	}
	
	// Establish the number of ticks in 10ms for later use...
	NumTicksPer10ms = busClockFreq * 0.01;

	// Establish the number of ticks in 10us for later use...
	NumTicksPer10us = busClockFreq * 0.00001;
	
	// Establish the number of ticks in 1us for later use...
	uint32_t NumTicksPer_us = busClockFreq * 0.000001;

	// 1) Disable SysTick during initialization.
	NVIC_ST_CTRL_R = 0;
	
	// 2) Set the RELOAD register to establish a modulo RELOAD + 1 decrement counter
	NVIC_ST_RELOAD_R = 0x00FFFFFF;
	
	// 3) Clear the accumulator
	NVIC_ST_CURRENT_R = 0;
	
	// 4) Set clock source to core clock and enable 
	NVIC_ST_CTRL_R = NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_ENABLE;
	
	// Success
	return 0;
}

//----------------------- SysTick_Wait --------------------------
// Waits (blocks) for a given number of bus cycles.
// Inputs:  delay - the number of bus cycles to wait.
// Outputs:  none.
void SysTick_Wait(uint32_t delay)
{
	// Delay in number of bus cycle period units
	NVIC_ST_RELOAD_R = delay - 1;
	
	// Clear CURRENT
	NVIC_ST_CURRENT_R = 0; 
	
	// Wait for COUNT flag to be set upon rollover.
	while ((NVIC_ST_CTRL_R & NVIC_ST_CTRL_COUNT) == 0) {
	}
	
}

//---------------------- SysTick_Wait10ms--------------------------
// Waits (blocks) for a given number of 10 millisecond intervals.
// Inputs:  delay - the number of 10ms intervals to wait.
// Outputs:  none.
void SysTick_Wait10ms(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		SysTick_Wait(NumTicksPer10ms);
	}
}

//---------------------- SysTick_Wait10us--------------------------
// Waits (blocks) for a given number of 10 microsecond intervals.
// Inputs:  delay - the number of 10us intervals to wait.
// Outputs:  none.
void SysTick_Wait10us(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		SysTick_Wait(NumTicksPer10us);
	}
}

//---------------------- SysTick_Wait_us--------------------------
// Waits (blocks) for a given number of 1 microsecond intervals.
// Inputs:  delay - the number of 10us intervals to wait.
// Outputs:  none.
void SysTick_Wait_us(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		SysTick_Wait(NumTicksPer_us);
	}
}
