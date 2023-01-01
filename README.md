# I2C_LCD
Example software for interfacing a 16x2 HD44780 based LCD using an I2C driver with the TM4C123GH6PM (Tiva C Launchpad).

# Usage
This example is meant to closely mirror the popular Arduino [I2C LCD](https://github.com/johnrickman/LiquidCrystal_I2C) library. 
However, the code was developed using C. All functions have the same name so porting to the `TM4C123GH6PM` shouldn't be too difficult. The only major 
difference is that global functions were implemented and a struct was used to house the LCD I2C components.

See the example in `main.c` for usage.
