// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 5/2/21
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// 
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************
#define C    2389   // 1046.5 Hz
#define B    2531   // 987.8 Hz
#define BF   2681   // 932.3 Hz
#define A    2841   // 880 Hz
#define AF   3010   // 830.6 Hz
#define G    3189   // 784 Hz
#define GF  3378   // 740 Hz
#define F   3579   // 698.5 Hz
#define E   3792   // 659.3 Hz
#define EF  4018   // 622.3 Hz
#define D   4257   // 587.3 Hz
#define DF  4510   // 554.4 Hz


#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
#define PA432                 (*((volatile uint32_t *)0x40004070)) // bits 3-1	
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040))
#define PE0       (*((volatile uint32_t *)0x40024004))	
// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void LogicAnalyzerTask2(void){
  UART0_DR_R = 0x80|PF321|(PA432<<2)|PE0; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
}
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))

// **************SysTick_Init*********************
// Initialize Systick periodic interrupts
// Input: interrupt period
//        Units of period are 12.5ns
//        Maximum is 2^24-1
//        Minimum is determined by length of ISR
// Output: none
void SysTick_Init(unsigned long period){
  // write this
	NVIC_ST_RELOAD_R = period-1; // reload value
	NVIC_ST_CURRENT_R = 0;			 // any write will reload counter and clear count
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000;
	NVIC_ST_CTRL_R = 0x07;
	
}

// Initialize Port F so PF1, PF2 and PF3 are heartbeats
void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;      // activate port F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 (built-in LED)
  GPIO_PORTF_PUR_R |= 0x10;
  GPIO_PORTF_DEN_R |=  0x1E;   // enable digital I/O on PF
}
int32_t Data;         // 12-bit ADC
uint32_t Position;    // 32-bit fixed-point 0.01 cm

int main1(void){      // single step this program and look at Data
  //TExaS_Init();       // Bus clock is 80 MHz 
  ADC_Init(SAC_NONE); // turn on ADC, set channel to 5
  while(1){                
    Data = ADC_In();  // sample 12-bit channel 5
  }
}
//Convert Function
uint32_t Convert(uint32_t data){	
  return  (160*data)/4096+21;
}
const uint8_t* PlayerPiece; //Ptr to DogPiece or CatPiece
uint16_t x_slider = 0; // 0-6 value of slider position
uint16_t PieceSliderValue;	//Coordinate value for drawing player sprite when moving the Slider
int MailStatus;
uint32_t MailValue;
void SysTick_Handler(void){ 
  PF1 ^= 0x02;     // Heartbeat, LED for TexasDisplay
  MailValue = ADC_In(); // MailValue is a global variable
	MailStatus = 1; // MailStatus is a global variable for main
	
}

int main(void){  
	
  DisableInterrupts();
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  ADC_Init(SAC_32);  // turn on ADC, set channel to 5
 // 32-point averaging  
  PortF_Init();
  // other initialization for the Heartbeat
	PlayerPiece = CatPiece;
	EnableInterrupts();	

	SSD1306_OutClear();
	Random_Init(1);
	Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
	SysTick_Init(2000001);
  while(1){
    // wait on mailbox
    while(MailStatus == 0){};
		PF3 ^= 0x08;       // Heartbeat
		MailStatus = 0; // indicate mail is read
	  SSD1306_ClearBuffer();   
	
		Position = Convert(MailValue);
		
		//Segments Position into usable values
		
		if(Position <= 21){
		x_slider = 0;
		PieceSliderValue = 2;		
    }
		if(Position < 40 && Position > 21){
		x_slider = 1;
		PieceSliderValue = 20;		
    }
		if(Position < 70 && Position > 40 ){
		x_slider = 2;	
		PieceSliderValue = 38;	
    }
		if(Position < 100 && Position > 70 ){
		x_slider = 3;
		PieceSliderValue = 56;		
    }
		if(Position < 130 && Position > 100 ){
		x_slider = 4;	
		PieceSliderValue = 74;	
    }
		if(Position < 160 && Position > 130 ){
		x_slider = 5;	
		PieceSliderValue = 92;	
    }
		if(Position > 165){
		x_slider = 6;	
		PieceSliderValue = 110;
    }
		
		//
		
		//Draw Movement
SSD1306_DrawBMP(0, 64, Connect4Grid, 0, SSD1306_WHITE); //Draw Grid
SSD1306_DrawBMP(PieceSliderValue, 8, PlayerPiece, 0, SSD1306_WHITE); //Draw current piece in current slider position
SSD1306_OutBuffer();		
  }
}

