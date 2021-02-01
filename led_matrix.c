/*
	Led_Matrix II - Raspberry Pi powered scrolling LED matrix display made by Bernhard Ritter
	based on an idea of Journal lumineux défilant à base de Raspberry Pi / Copyright (C) 2014 François Mocq, F1GYT
	See https://github.com/framboise314/led_matrix

	Use display with 4 matrix from AZ-delivery for optimal compatibility

	This program is free software: you can redistribute it
	or modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation, i.e. version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY: without even the implied warranty of MERCHANTABILITY
	nor FITNESS FOR A PARTICULAR PURPOSE. Read the GNU General Public License
	for more details.

	You should have received a copy of the GNU General Public License with
	This program ; if not, see: <http://www.gnu.org/licenses/>.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
* Compiler : gcc led_matrix.c -o led -I/usr/local/include -L/usr/local/lib -lwiringPi
* or use makefile with default target:  make; ./led "my own text"
*
*******************************************************************************************
*
* The Raspberry Pi communicates with the MAX7219 using 3 signals: DATA, CLK, and LOAD (CS).
* Signals corresponding to those of the SPI bus on GPIO P1 are used here, but are managed
* by the program itself. Data D15 is sent first.
*                   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
*    DATA _________|D15|D14|D13|D12|D11|D10|D09|D08|D07|D06|D05|D04|D03|D02|D01|D00|______
*         ________    __    __    __    __    __    __    __    __    __    __    ________
*    CLK          |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
*         ______                                                                      __
*    LOAD       |____________________________________________________________________|  |__
*
*/

#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <string.h>
#include "font.h"

//
// Definition of pins according to MAX7219
// WiringPi numbers translated into GPIO port numbers and names
// 10 corresponds to CSO  GPIO 24 
// 12 corresponds to MOSI GPIO 19 
// 14 corresponds to SCLK GPIO 23 

// If you use AZ-Delivery 4 (8x) Matrix layout you may connect using the colors
#define MAX7219_CS0		10 // white (CE0)
#define MAX7219_DIN		12 // grey (MOSI)
#define MAX7219_CLK		14 // black (SCLK)

// definiton of registers of MAX7219
#define MAX7219_REG_NOOP 		0x00
#define MAX7219_REG_DECODE 		0x09 // used for 7 Segment Display
#define MAX7219_REG_INTENSITY 	0x0A
#define MAX7219_REG_SCAN_LIMIT 	0x0B
#define MAX7219_REG_SHUTDOWN 	0x0C
#define MAX7219_REG_TEST 		0x0F

// definiton of brightnes of matrix
#define MAX7219_INTENSITY 		0x10

// dfinition of AZ-Delivery display default size
#define MATRIX_PIXELS			8

// all Elements are counted from left=0 to right=max 
// one lement with 4 Matrix
//#define MATRIX_COUNT			4   // if you have one display with for matrix this is 4
//#define MATRIX_COUNT_PREPM		6   // for full scrolling this is MATRIX_COUNT+2
//#define FIRST_MATRIX_IN_REAL    0   // This is the first element where the scrolling is visual

//two elements with 4 matrix
#define MATRIX_COUNT			8   // if you have one display with for matrix this is 4
#define MATRIX_COUNT_PREPM		10   // for full scrolling this is MATRIX_COUNT+2
#define FIRST_MATRIX_IN_REAL    0   // This is the first element where the scrolling is visual

//two elements with 4 matrix - scrolling only on the right
//#define MATRIX_COUNT			8   // if you have one display with for matrix this is 4
//#define MATRIX_COUNT_PREPM		6   // for full scrolling this is MATRIX_COUNT+2
//#define FIRST_MATRIX_IN_REAL    4   // This is the first element where the scrolling is visual
/************************************/
/*    defintions of macros          */
/************************************/

// Function to test the value of a bit
// utilisation : CHECK_BIT(temp, 3)
// Source : http://stackoverflow.com/questions/523724/c-c-check-if-one-bit-is-set-in-i-e-int-variable
// et http://www.bien-programmer.fr/bits.htm
//****************************************

#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))

/************************************/
/*    defintion of global variables      */
/************************************/
int messageLength;					// Message length in bytes - Pay attention to the final 0
int totalBufferLengthForMessage;	// Total length of the buffer containing the bytes to display

/***************************************/
/*    Definition of the message buffer */
/***************************************/
unsigned char buffer_prep_matrix[MATRIX_COUNT_PREPM][MATRIX_PIXELS];   // this is the buffer where we scroll and prepare display data - Text & Effects
unsigned char buffer_real_matrix[MATRIX_COUNT][MATRIX_PIXELS];	       // this is the buffer which is going to be displayed 

/************************************/
/*    defintion of functions        */
/************************************/
void MAX7219_Clear(void);
int MAX7219_Setup(int number);
int MAX7219_Write(unsigned char reg, unsigned char data);

/****** Bit operations***************/

// Function to set the kth bit of n 
int setBit(int n, int num)
{
	return (1 << n) | num;
	//return (n | (1 << (k - 1)));
}

// Function to clear the kth bit of n 
int clearBit(int n, int k)
{
	return (n & (~(1 << (k - 1))));
}

// Function to toggle the kth bit of n 
int toggleBit(int n, int k)
{
	return (n ^ (1 << (k - 1)));
}

// Function to check the kth bit of n
int checkBit(int n, int k) {
	return ((n) & (1 << (k)));
}

// For debug visualize a single byte
void visualByte(unsigned char b) {
	int colDst = 7;
	int rowDst = 7;

	for (int i = 0;i < 8;i++) {
		if (CHECK_BIT(b, i) == 0)
			printf(" ");
		else
			printf("X");
	}
	printf("\n");
}

// Clearing LEDs from the matrix
//*********************************
void MAX7219_Clear(void)
{
	unsigned char i;
	for (i = 0; i < 8; i++)
		MAX7219_Write(i, 0x00);
}

// Load Data which was transfered into registers of MAX7219
void MAX7219_Load(void)
{
	// Change CS0 to 1 and wait 1µs 
	digitalWrite(MAX7219_CS0, HIGH);
	delayMicroseconds(1);

	// Change CS0 to 1 and wait 1µs 
	digitalWrite(MAX7219_CS0, LOW);
	delayMicroseconds(1);
}

// Initializing the MAX7219
//**************************
int MAX7219_Setup(int number)
{
	unsigned char i;
	for (i = 0; i < (number + 1); i++)
	{
		// No scan limitation, scan all 8 lines
		MAX7219_Write(MAX7219_REG_SCAN_LIMIT, 7);
	}
	MAX7219_Load();

	for (i = 0; i < (number + 1); i++)
	{
		// Use of an 8x8 matrix => mode no decode
		MAX7219_Write(MAX7219_REG_DECODE, 0x00);
	}
	MAX7219_Load();

	for (i = 0; i < (number + 1); i++)
	{
		// Normal operating mode: no shutdown
		MAX7219_Write(MAX7219_REG_SHUTDOWN, 0x01);
	}
	MAX7219_Load();

	for (i = 0; i < (number + 1); i++)
	{
		// Normal operating mode: no test mode
		MAX7219_Write(MAX7219_REG_TEST, 0x00);
	}
	MAX7219_Load();

	for (i = 0; i < (number + 1); i++)
	{
		// Adjust the brightness of the matrices (value between 0x00 and 0x0F)
		MAX7219_Write(MAX7219_REG_INTENSITY, MAX7219_INTENSITY);
	}
	MAX7219_Load();

	for (i = 0; i < (number + 1); i++)
	{
		// Erase the LEDs from the matrix
		MAX7219_Clear();
	}
	MAX7219_Load();

	return 0;
}

// Write data and register as 16 Bit value to the MAX7219
//**************************
int MAX7219_Write(unsigned char reg, unsigned char data)
{
	// Combine the two bytes into a 16-bit word
	unsigned short mot = (reg << 8) | data;

	// Pass CS0 to 0 to activate the MAX7219 and wait 1µs 
	digitalWrite(MAX7219_CS0, LOW);
	delayMicroseconds(1);

	// We can start sending the data
	// the data variable contains 16 bits (0 to 15)
	// which must be sent one by one on DIN
	for (int bit = 16; bit > 0;bit--) {
		// each cycle starts with CLK=0 
		digitalWrite(MAX7219_CLK, LOW);
		delayMicroseconds(1);

		// send bit by bit into data channel
		digitalWrite(MAX7219_DIN, CHECK_BIT(mot, bit - 1));
		delayMicroseconds(1);

		// rise clock to load data into register 
		digitalWrite(MAX7219_CLK, HIGH);
		delayMicroseconds(1);

	}
	//delayMicroseconds(1);

	return 0;
}

/**********************************************************/
/*     text handling functions					          */
/*     Translating from char into Bit which are presented */
/**********************************************************/

// rotate a char 90 degree left
void rotateChar(unsigned char* tempChar) {
	int colDst = 7;
	int lineDst = 0; // 90 degree
	
	// init DST char with 0
	unsigned char dstChar[9] = { 0,0,0,0,0,0,0,0,0 };

	for (int line = 0;line < 8;line++) {
		for (int column = 0; column < 8; column++) {
			int bit = CHECK_BIT(tempChar[line], column);
			if (bit > 0) {
				// copy used bits
				dstChar[lineDst] = setBit(colDst, dstChar[lineDst]);
			}

			lineDst++;
		}
		// next dst col
		lineDst = 0;
		colDst--;
	}

	// finally copy rotated into source 
	for (int line = 0; line < MATRIX_PIXELS; line++) {
		tempChar[line] = dstChar[line]; // copy rotated char into destination
	}
}

// copy char from font definition into assigned memory
void getCharFromFont(unsigned char* tempChar, char visualChar) {
	// Retrieve the 8 bytes of the character
	for (int column = 0; column < MATRIX_PIXELS; column++)
	{
		unsigned char val = cp437_font[visualChar][column];
		//unsigned char val = font1[visualChar][column];
		tempChar[column] = val;
	}
}

// For debug reasons - show a character from memory visual at console
void showChar(unsigned char* tempChar) {
	for (int line = 0; line < MATRIX_PIXELS; line++) {
		int val = tempChar[line];
		for (int bit = 0; bit < 8;bit++) {
			if (CHECK_BIT(val, bit) == 0)
				printf(" ");
			else
				printf("X");
		}
		printf("|\n");
	}
	printf("\n");
}

// load one character to desired pos in buffer
void loadCharToMatrix(unsigned char buffer[][MATRIX_PIXELS], unsigned char theChar, int pos) {
	unsigned char tempChar[MATRIX_PIXELS];
	getCharFromFont(tempChar, theChar);
	rotateChar(tempChar); // rotate as font definition is not made for left to right scrolling

	// Retrieve the 8 bytes of the character
	for (int column = 0; column < MATRIX_PIXELS; column++)
	{
		buffer[pos][column] = tempChar[column];
	}
}

// For Debug - Use ASCII to Show complete requested buffer on console
void showBuffer(unsigned char buffer[][MATRIX_PIXELS], int highest_max) {
	printf("---------------------------------\n");
	for (int max = 0; max < highest_max; max++) {
		printf("max(%d)\n", max);
		for (int line = 0; line < MATRIX_PIXELS; line++) {
			int val = buffer[max][line];
			for (int bit = 0; bit < 8;bit++) {
				if (CHECK_BIT(val, bit) == 0)
					printf(" ");
				else
					printf("X");
			}
			printf("|\n");
		}
		printf("\n");
	}
	printf("---------------------------------\n");
}

// scroll requested buffer one bit - default is to left
void scrollBuffer(unsigned char buffer[][MATRIX_PIXELS], int length) {
	for (int max=length; max >= 0; max--) {
		int overflow[MATRIX_COUNT+2][MATRIX_PIXELS];

		for (int line = 0; line < MATRIX_PIXELS; line++) {
			int val = buffer[max][line];
			
			overflow[max][line] = val & 128; // save highhest bit as overflow
			//printf("overflow (max:%d) (line:%d) (val:%d)\n", max, line, overflow[max][line]);
			val = val << 1;

			if (overflow[max + 1][line] > 0)
				val += 1; // set lowest bit from overflow

			buffer[max][line] = val;
		}
	}
}

// Show a static text in the display - used if parts are scrolling
void show_static_text(char* text, int pos) {
	for (int i = 0;i < strlen(text);i++) {
		loadCharToMatrix(buffer_real_matrix, text[i], pos+i);
		printf("Load to pos %d\n", pos + i);
	}
}

/***********************************************************************************
**************  main programm       ************************************************
************************************************************************************
*/

int main(int argc, char* argv[])
{
	// check if we got a personal message as parameter
	char* message;
	if (argc > 1) {
		message = argv[1];
	}
	else {
		// Init text if not given
		message = "\3 I love scrolling \3    ";	// Contient le message en ASCII
	}
	printf("Message data : %s \n", message);
	messageLength = strlen(message);

	// Initialize wiringPi
	if (wiringPiSetup() < 0)
	{
		fprintf(stderr, "setup failed\n");
		exit(1);
	}

	// Init GPIO utilisés aus output
	printf("Initializing GPIO ports\n");
	pinMode(MAX7219_CS0, OUTPUT);
	pinMode(MAX7219_DIN, OUTPUT);
	pinMode(MAX7219_CLK, OUTPUT);

	// Initialiser le MAX7219
	printf("Initialization of max7219 - count(%d)\n", MATRIX_COUNT);
	MAX7219_Setup(MATRIX_COUNT);

	printf("init phase finished\n");

	/**************************************************/
	/*     Clear Matrix on start			          * /
	/**************************************************/
	// Send Bits to all configured MAX7219 matrix - lines are numbered from 1-9!!!!
	printf("clear matrix\n");
	for (int line = 1;line < MATRIX_PIXELS + 1;line++) {
		for (int max = 0;max < MATRIX_COUNT;max++)
			MAX7219_Write(line, 0b10101010);

		MAX7219_Load(); // let all MAX devices load the column
	}
	delay(200);

	/**************************************************/
	/*     Init all leds of real matrix to off       * /
	/**************************************************/
	printf("init real matrix\n");
	for (int i = 0; i < MATRIX_COUNT; i++)
	{
		for (int j = 0; j < MATRIX_PIXELS;j++) {
			buffer_real_matrix[i][j] = 0;
		}
	}
	/**************************************************/
	/*     Init all leds of prep matrix to off       * /
	/**************************************************/
	printf("init prep matrix\n");
	for (int i = 0; i < MATRIX_COUNT_PREPM; i++)
	{
		for (int j = 0; j < MATRIX_PIXELS;j++) {
			buffer_prep_matrix[i][j] = 0;
		}
	}
	
	do // begin from start
	{
		// init pointer
		printf("Start full loop\n");
		int message_buffer_pointer = 0;

		//show_static_text("halb",0);
		
		do // loop through text
		{
			//printf("Load char %d %c\n", message_buffer_pointer, message[message_buffer_pointer]);
			
			// load char to postion out of sight and than scroll into visual 
			loadCharToMatrix(buffer_prep_matrix, message[message_buffer_pointer], MATRIX_COUNT_PREPM - 1); 
			message_buffer_pointer++;

			// scroll one char in matrix
			for (int fullscroll = 0;fullscroll < MATRIX_PIXELS; fullscroll++) {
				//Load Bit from Message Buffer into Display Buffer
				
				for (int i = 1; i < MATRIX_COUNT_PREPM-1; i++) {  // start from one because at 0 every char disapears 
					for (int j = 0; j < MATRIX_PIXELS; j++) {
						buffer_real_matrix[i-1 + FIRST_MATRIX_IN_REAL][j] = buffer_prep_matrix[i][j];
					}
				}
				
				//Think about effects
				//showBuffer(buffer_real_matrix, MATRIX_COUNT);

				// Send out the matrix buffer to the leds
				for (int line = 1;line < (MATRIX_PIXELS + 1);line++) {
					for (int max = 0; max < MATRIX_COUNT; max++) {
						MAX7219_Write(line, buffer_real_matrix[max][line - 1]);
					}
					MAX7219_Load(); // let all MAX devices load the column
				}
				
				scrollBuffer(buffer_prep_matrix, MATRIX_COUNT_PREPM - 1);
				delay(50);
			}

		} while (message_buffer_pointer < messageLength);

	} while (1);
}

// Send Bits to MAX7219 matrix
/*for (int line = 1;line < 9;line++) {
	MAX7219_Write(line, 0b01010101);
	MAX7219_Write(line, 0b01010101);
	MAX7219_Write(line, 0b01010101);
	MAX7219_Write(line, 0b01010101);
	MAX7219_Load(); // let all MAX devices load the column
	//delay(1);
}*/



