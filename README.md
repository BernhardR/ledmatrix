# ledmatrix
a small c-project which uses 8x8 matrix led displays connected to max 7219

Installation:

- install linux with gcc support
- install wiringPi
- connect display as follows

If you use AZ-Delivery 4 (8x) Matrix layout you may connect using the colors
- MAX7219_CS0		10 // white (CE0)
- MAX7219_DIN		12 // grey (MOSI)
- MAX7219_CLK		14 // black (SCLK)

use 5V external power source

Run
- run make
- ./led
