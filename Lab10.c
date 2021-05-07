// Lab10.c
// Runs on TM4C123
// Jacob Tomczeszyn and Grayson Drinkard

// Last Modified: 5/5/21

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
// ******* Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2
// Slide pot pin 3 connected to +3.3V 
// left button connected to PF4
//right button connected to PF0
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

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
#define PE210                   (*((volatile uint32_t *)0x4002401C)) // bits 2-0	
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040))
#define PE0       (*((volatile uint32_t *)0x40024004))	
	

#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))	
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF4       (*((volatile uint32_t *)0x40025040))
#define SWITCHES  (*((volatile uint32_t *)0x40025044))
#define SW1       0x10                      // on the left side of the Launchpad board
#define SW2       0x01                      // on the right side of the Launchpad board
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control	
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
///end of debugging stuff












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

// Initialize Port F so PF1, PF2 and PF3 are heartbeats, and so PF4 and PF0 are button inputs
void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;      // activate port F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
	GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
	GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog on PF	
	GPIO_PORTF_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PF4-0	
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 (built-in LED)
  GPIO_PORTF_PUR_R = 0x11;          // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital I/O on PF4-0
}
// Initialize Port B so PB3-0 are the DAC output bits
void PortB_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B
  while((SYSCTL_PRGPIO_R&0x22) != 0x22){};
	GPIO_PORTB_CR_R = 0x1F;           // allow changes to PB3-0
	GPIO_PORTB_AMSEL_R = 0x00;        // 3) disable analog on PF	
	GPIO_PORTB_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PB3-0	
  GPIO_PORTB_DIR_R |=  0x0F;   //
  GPIO_PORTB_PUR_R = 0x0F;          // enable pull-up PB3=0
  GPIO_PORTB_DEN_R = 0x0F;          // 7) enable digital I/O on PB3-0
}






uint32_t PortF_Input(void){
  return (GPIO_PORTF_DATA_R&0x11);  // read PF4,PF0 inputs
}

//Matrix for Connect 4 positions
uint16_t Connect4_grid[6][7];	
// 0: open, 1: filled by player 2: filled by CPU
//|0|0|0|0|0|0|0|
//|0|0|0|0|0|0|0|
//|0|0|0|0|0|0|0|
//|0|0|0|0|0|0|0|
//|0|0|0|0|0|0|0|
//|0|0|0|0|0|0|0|


uint16_t Connect4_x[7] = 	//Matrix for possible Piece x positions, for drawing sprites
	{2,20,38,56,74,92,110};
	
uint16_t Connect4_y[6] = 	//Matrix for possible piece y positions, for drawing sprites
	{18,27,36,45,54,63};

	
int32_t Data;         // 12-bit ADC
uint32_t Position;    // 32-bit fixed-point number
	
	
	
	
//Four in a row checks	
int Vertical_Check (void) {

for(int a=0; a<6; a++){
for(int b=0; b<7; b++){
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b]    * 	Connect4_grid[a+2][b] *  Connect4_grid[a+3][b] == 1){
	return 1;}
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b]    * 	Connect4_grid[a+2][b] *  Connect4_grid[a+3][b] == 16){
	return 2;}

}}
return 0;}
int Horizontal_Check (void) {

for(int a=0; a<6; a++){
for(int b=0; b<7; b++){
if (Connect4_grid[a][b]  *	Connect4_grid[a][b+1]    * 	Connect4_grid[a][b+2] *  Connect4_grid[a][b+3] == 1){
	return 1;}
if (Connect4_grid[a][b]  *	Connect4_grid[a][b+1]    * 	Connect4_grid[a][b+2] *  Connect4_grid[a][b+3] == 16){
	return 2;}

}}
return 0;}
int Diagonal_Check (void) {

for(int a=0; a<6; a++){
for(int b=0; b<7; b++){
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b+1]    * 	Connect4_grid[a+2][b+2] *  Connect4_grid[a+3][b+3] == 1){
	return 1;}
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b+1]    * 	Connect4_grid[a+2][b+2] *  Connect4_grid[a+3][b+3] == 16){
	return 2;}
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b-1]    * 	Connect4_grid[a+2][b-2] *  Connect4_grid[a+3][b-3] == 1){
	return 1;}
if (Connect4_grid[a][b]  *	Connect4_grid[a+1][b-1]    * 	Connect4_grid[a+2][b-2] *  Connect4_grid[a+3][b-3] == 16){
	return 2;}
}}
return 0;}
//
//
uint16_t buttonstatus;
int winner;
int PlayerTeam = 1; //0 is dog, 1 is cat

int lang;
//uint32_t rand=324;
int roundcount[2]; //counts number of rounds won, space 0 is player, space 1 is cpu
int roundscreen(void){
	
DisableInterrupts();
SSD1306_Init(SSD1306_SWITCHCAPVCC);
 PortF_Init();
 PortB_Init();
 Sound_Init();
  // other initialization for the Heartbeat and buttons
	//PlayerPiece = CatPiece;
	EnableInterrupts();	
	SSD1306_OutClear();
	Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
	SysTick_Init(2000001);
	SSD1306_ClearBuffer();
//zeros out grid after each round	
for(int a=0; a<6; a++){	
for(int b=0; b<7; b++){
Connect4_grid[a][b] = 0;
}}
//
//gameover section
//
while(roundcount[0] == 3){
if(lang==0){
	
if(PlayerTeam==0){

//English Dog
SSD1306_DrawBMP(0, 64, DogWinScreenEn, 0, SSD1306_WHITE);
SSD1306_OutBuffer();

}
if(PlayerTeam==1){
	
	//English Cat


SSD1306_DrawBMP(0, 64, CatWinScreenEn, 0, SSD1306_WHITE);
SSD1306_OutBuffer();

}
}
if(lang==1){
if(PlayerTeam==0){
	
//French Dog 

	
SSD1306_DrawBMP(0, 64, DogWinScreenFr, 0, SSD1306_WHITE);
SSD1306_OutBuffer();
		
	

}
if(PlayerTeam==1){
//French Cat	


SSD1306_DrawBMP(0, 64, CatWinScreenFr, 0, SSD1306_WHITE);
SSD1306_OutBuffer();
	
}
}
}
//CPU WIN
while(roundcount[1] == 3){
if(lang==0){
	
if(PlayerTeam==1){

//English Dog
SSD1306_DrawBMP(0, 64, DogWinScreenEn, 0, SSD1306_WHITE);
SSD1306_OutBuffer();

}
if(PlayerTeam==1){
	
	//English Cat


SSD1306_DrawBMP(0, 64, CatWinScreenEn, 0, SSD1306_WHITE);
SSD1306_OutBuffer();

}
}
if(lang==1){
if(PlayerTeam==0){
	
//French Dog 

	
SSD1306_DrawBMP(0, 64, DogWinScreenFr, 0, SSD1306_WHITE);
SSD1306_OutBuffer();
		
	

}
if(PlayerTeam==0){
//French Cat	


SSD1306_DrawBMP(0, 64, CatWinScreenFr, 0, SSD1306_WHITE);
SSD1306_OutBuffer();
	
}
}
}
//
//
//

//in between rounds screen
while(roundcount[0] < 3 && roundcount[1] < 3){
	
	buttonstatus = PortF_Input();	
		if(lang==1){
		if(PlayerTeam==1){
		
		SSD1306_SetCursor(6,1);
		SSD1306_OutString("Chiens=");
		SSD1306_OutUDec(roundcount[1]);
		
		SSD1306_SetCursor(6,2);
		SSD1306_OutString("Chats="); 
		SSD1306_OutUDec(roundcount[0]);
		}	
		if(PlayerTeam==0){

		SSD1306_SetCursor(6,1);
		SSD1306_OutString("Chiens=");
		SSD1306_OutUDec(roundcount[0]);
		
		SSD1306_SetCursor(6,2);
		SSD1306_OutString("Chats="); 
		SSD1306_OutUDec(roundcount[1]);
		}
		
		}
		if(lang==0){
				if(PlayerTeam==1){
			
		SSD1306_SetCursor(6,2);
		SSD1306_OutString("Cats="); 
		SSD1306_OutUDec(roundcount[0]);				
		SSD1306_SetCursor(6,1);
		SSD1306_OutString("Dogs=");
		SSD1306_OutUDec(roundcount[1]);
		

		}	
		if(PlayerTeam==0){
		
		SSD1306_SetCursor(6,1);
		SSD1306_OutString("Dogs:");
		SSD1306_OutUDec(roundcount[0]);
		
		SSD1306_SetCursor(6,2);
		SSD1306_OutString("Cats:"); 
		SSD1306_OutUDec(roundcount[1]);
		}
		
		}
if(buttonstatus == 0x10){
	

gameplay();}

}	
}






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
const uint8_t* CPUPiece; //Ptr to DogPiece or CatPiece
const uint8_t* PlayerDropperPiece; //Ptr to DogPiece or CatPiece
const uint8_t* CPUDropperPiece; //Ptr to DogPiece or CatPiece

uint16_t PieceXValue;
uint16_t PieceYValue;
uint16_t x_slider = 0; // 0-6 value of slider position
uint16_t PieceSliderValue;	//Coordinate value for drawing player sprite when moving the Slider
int MailStatus;
uint32_t MailValue;
void SysTick_Handler(void){ 
  PF1 ^= 0x02;     // Heartbeat, LED for TexasDisplay
  MailValue = ADC_In(); // MailValue is a global variable
	MailStatus = 1; // MailStatus is a global variable for main
	
}

int main(void){ uint16_t CPUTeam;
//lang;  0 is english, 1 is french
//PlayerTeam; 0 is dog, 1 is cat

	
  DisableInterrupts();
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  ADC_Init(SAC_32);  // turn on ADC, set channel to 5
 // 32-point averaging  

 PortF_Init();
 PortB_Init();
 Sound_Init();




	SSD1306_OutClear();
	Random_Init(1);
	Profile_Init(); // PB5,PB4,PF3,PF2,PF1
	

	SysTick_Init(2000001);
		EnableInterrupts();	
	SSD1306_ClearBuffer();  
//
//Language Select Screen
//
//
//

while(1){
			
		SSD1306_ClearBuffer(); 
		EnableInterrupts();
	
	//Language Select Sprite

	
	SSD1306_DrawBMP(0, 64, LanguageSelect, 0, SSD1306_WHITE);
	SSD1306_OutBuffer();
buttonstatus = PortF_Input();	
if(buttonstatus == 0x01){
Sound_PlayerDrop();		
lang = 0;// English
break;
}
else if(buttonstatus == 0x10){
Sound_PlayerDrop();		
lang = 1; //French
break;}

}
SSD1306_ClearBuffer(); 

//
//Title Screen
//
//
if(lang == 0){
for(int i=0; i<14; i++){
//
//
//	
//Title Screen English
//
//

	//Title Screen Sprite
	
	SSD1306_DrawBMP(0, 64, TitleScreenEn, 0, SSD1306_WHITE);
	SSD1306_OutBuffer();
	EnableInterrupts();

}
}
else{
for(int i=0; i<14; i++){
//
//
//	
//Title Screen French
//
//
//
	SSD1306_DrawBMP(0, 64, TitleScreenFr, 0, SSD1306_WHITE);
	SSD1306_OutBuffer();
	EnableInterrupts();
}
SSD1306_ClearBuffer();
}
//Character Select
while(1){

	EnableInterrupts();	
	SSD1306_ClearBuffer(); 
	SSD1306_DrawBMP(0, 64, CharacterSelect, 0, SSD1306_WHITE);
	SSD1306_OutBuffer();
buttonstatus = PortF_Input();	

	if(buttonstatus == 0x01){
Sound_PlayerDrop();		
PlayerTeam = 0; //Players are Dogs
CPUTeam = 1	;		//CPU is Cats
break;
}
else if(buttonstatus == 0x10){
Sound_PlayerDrop();		
PlayerTeam = 1; //Players are Cats
CPUTeam = 0;		//CPU is Dogs
break;}
}
SSD1306_ClearBuffer(); 
SSD1306_OutClear();
for(int i=0; i<35; i++){
//
//
//	
//Start Gameplay Screen
//

if (lang == 0){
  SSD1306_SetCursor(3,0);
	SSD1306_OutString("Get 4 in a row");
	SSD1306_SetCursor(3,1);
	SSD1306_OutString("3 times to win!");
	SSD1306_SetCursor(3,3);
	SSD1306_OutString("(if 4 in a row,");
	SSD1306_SetCursor(3,4);
	SSD1306_OutString("press button");	
	
	SSD1306_SetCursor(03,5);
	SSD1306_OutString("to complete round)");
	EnableInterrupts();
}

if (lang == 1){		//french
  SSD1306_SetCursor(3,0);
	SSD1306_OutString("quatre d'affilée");
	SSD1306_SetCursor(3,1);
	SSD1306_OutString("pour gagner!");
	EnableInterrupts();
}








}
SSD1306_ClearBuffer(); 

gameplay();}

//
//
//


//Gameplay Start
int gameplay(void){
  DisableInterrupts();
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  ADC_Init(SAC_32);  // turn on ADC, set channel to 5
 // 32-point averaging  

 PortF_Init();
 PortB_Init();
 Sound_Init();
  // other initialization for the Heartbeat and buttons
	//PlayerPiece = CatPiece;
	EnableInterrupts();	
	Random_Init(Position);	
	SSD1306_OutClear();
	Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
	SysTick_Init(2000001);
	
	SSD1306_ClearBuffer();  
  while(1){
	
		int turn; //0: player turn, 1: CPU turn		
		if(PlayerTeam == 0){
		PlayerPiece = DogPiece;
		PlayerDropperPiece = DogDropper;	
		CPUPiece = CatPiece;
		CPUDropperPiece = CatDropper;		
		}
		else{
		PlayerPiece = CatPiece;
		PlayerDropperPiece = CatDropper;		
		CPUPiece = DogPiece;		
		CPUDropperPiece = DogDropper;		

		}
		
		//Player's Turn	
    // wait on mailbox
    while(MailStatus == 0){};
		
		PF3 ^= 0x08;       // Heartbeat
		MailStatus = 0; // indicate mail is read
		SSD1306_ClearBuffer();   			
		DisableInterrupts();
	
		Position = Convert(MailValue);
		
		//Segments Position into usable values
		
		if(Position <= 28){
		x_slider = 0;
		//PieceSliderValue = 2;		
    }
		if(Position < 57 && Position > 29){
		x_slider = 1;
		//PieceSliderValue = 20;		
    }
		if(Position < 85 && Position > 57 ){
		x_slider = 2;	
		//PieceSliderValue = 38;	
    }
		if(Position < 113 && Position > 85 ){
		x_slider = 3;
		//PieceSliderValue = 56;		
    }
		if(Position < 141 && Position > 113 ){
		x_slider = 4;	
	//	PieceSliderValue = 74;	
    }
		if(Position < 169 && Position > 141 ){
		x_slider = 5;	
	//	PieceSliderValue = 92;	
    }
		if(Position > 169){
		x_slider = 6;	
		//PieceSliderValue = 110;

    }
EnableInterrupts();			

SSD1306_DrawBMP(0, 63, Connect4Grid, 0, SSD1306_WHITE); //Draw Grid
SSD1306_DrawBMP(Connect4_x[x_slider]-1, 10, PlayerDropperPiece, 0, SSD1306_WHITE); //Draw current piece in current slider position

//Stacking and Drawing of Placed Player Pieces		
for(uint16_t a = 0; a < 6; a++){
for(uint16_t c = 0; c < 7; c++){

	if (Connect4_grid[a][c] == 1){
	PieceXValue = Connect4_x[c];
	PieceYValue = Connect4_y[a];
	SSD1306_DrawBMP(PieceXValue, PieceYValue, PlayerPiece, 0, SSD1306_WHITE); //Draw Player Pieces	
	}


}
}

for(int i = 0; i< 5; i++){
for(uint16_t a = 0; a < 6; a++){
for(uint16_t c = 0; c < 7; c++){

	if (Connect4_grid[a][c] == 2){
	PieceXValue = Connect4_x[c];
	PieceYValue = Connect4_y[a];
	SSD1306_DrawBMP(PieceXValue, PieceYValue, CPUPiece, 0, SSD1306_WHITE); //Draw CPU Pieces
	}


}
}
}
		SSD1306_OutBuffer();		
	
		//Button Press to drop piece 
		
		buttonstatus = PortF_Input();	
			while(buttonstatus == 0x10){
			Sound_PlayerDrop();
			PF1 ^= 0x02;	
			DisableInterrupts();	
			
					//Store x position
					uint16_t Player_Piece_x_Location	= x_slider;
					//Win check
					winner = Vertical_Check();
					if(winner == 1){
					roundcount[0]++;
					//return 0, PlayerTeam, CPUTeam;	//exits gameplay, goes to scorescreen	
					roundscreen();	
					}
					if(winner == 2){
					roundcount[1]++;
					roundscreen();		
					}
					winner = Horizontal_Check();
					if(winner == 1){
					roundcount[0]++;
					roundscreen();	
					}
					if(winner == 2){
					roundcount[1]++;
					roundscreen();	
					}
					winner = Diagonal_Check();
					if(winner == 1){
					roundcount[0]++;
					roundscreen();	
					}
					if(winner == 2){
					roundcount[1]++;
					roundscreen();	
					}	
					///
					///Check if piece in the way
					if(Connect4_grid[5][Player_Piece_x_Location] == 0){
					EnableInterrupts();	
					Connect4_grid[5][Player_Piece_x_Location] = 1;
				
					turn=1;//go to CPU turn
					break;
	
					}
					if(Connect4_grid[4][Player_Piece_x_Location] == 0){
					EnableInterrupts();		
					Connect4_grid[4][Player_Piece_x_Location] = 1;

					turn=1;	
					break;
	
					}
					if(Connect4_grid[3][Player_Piece_x_Location] == 0){
					EnableInterrupts();	
					Connect4_grid[3][Player_Piece_x_Location] = 1;

					turn=1;
					break;
				
					}
					
					if(Connect4_grid[2][Player_Piece_x_Location] == 0){
					EnableInterrupts();	
					Connect4_grid[2][Player_Piece_x_Location] = 1;
					
						
					turn=1;	
					break;

					}
					if(Connect4_grid[1][Player_Piece_x_Location] == 0){
					EnableInterrupts();	
					Connect4_grid[1][Player_Piece_x_Location] = 1;
		
					turn=1;		
					break;

					}
					
					if(Connect4_grid[0][Player_Piece_x_Location] == 0){
					EnableInterrupts();	
					Connect4_grid[0][Player_Piece_x_Location] = 1;

					turn=1;	
					break;	
					}			
					
					if(Connect4_grid[0][Player_Piece_x_Location] == 1){
				EnableInterrupts();	

					break;
					
					}			
					if(Connect4_grid[0][Player_Piece_x_Location] == 2){
				EnableInterrupts();	

					break;
					
					}			
									
					}
//CPU Turn
while(turn == 1){
uint16_t CPU_x;
CPU_x = Random()%7;

for(uint16_t a = 0; a < 6; a++){
for(uint16_t c = 0; c < 7; c++){

	if (Connect4_grid[a][c] == 1){
	PieceXValue = Connect4_x[c];
	PieceYValue = Connect4_y[a];
	SSD1306_DrawBMP(PieceXValue, PieceYValue, PlayerPiece, 0, SSD1306_WHITE); //Draw Piece in lowest open y pos at current x pos	
	}


}
}	
for(int i=0; i < 2; i++){
	
	for(int v=1; v <= 9; v++){
	SSD1306_DrawFastHLine(Connect4_x[x_slider]-1, v, 17, SSD1306_BLACK); //Hide player 
		}
	for(int v=9; v <= 10; v++){
	SSD1306_DrawFastHLine(Connect4_x[x_slider]-1, v, 18, SSD1306_WHITE); //Hide player 
		}		
SSD1306_OutBuffer();}	
for(int i=0; i < 8; i++){	

SSD1306_DrawBMP(Connect4_x[CPU_x]-1, 10, CPUDropperPiece, 0, SSD1306_WHITE); //Draw cpu piece in random slider position
	

SSD1306_OutBuffer();}
//Checks if piece is in the way
if(Connect4_grid[5][CPU_x] == 0){
					EnableInterrupts();
					Sound_CPUDrop();
Connect4_grid[5][CPU_x] = 2;
turn =0;// go to player turn
break;	
}	
if(Connect4_grid[4][CPU_x] == 0){
					EnableInterrupts();
					Sound_CPUDrop();
Connect4_grid[4][CPU_x] = 2;
turn =0;
	break;	
}		
if(Connect4_grid[3][CPU_x] == 0){
					EnableInterrupts();	
					Sound_CPUDrop();
Connect4_grid[3][CPU_x] = 2;
turn =0;
	break;	
}		
if(Connect4_grid[2][CPU_x] == 0){
					EnableInterrupts();
					Sound_CPUDrop();
Connect4_grid[2][CPU_x] = 2;
turn =0;
	break;	
}		
if(Connect4_grid[1][CPU_x] == 0){
					EnableInterrupts();
					Sound_CPUDrop();
Connect4_grid[1][CPU_x] = 2;
turn =0;
	break;	
}		
if(Connect4_grid[0][CPU_x] == 0){
					EnableInterrupts();
					Sound_CPUDrop();
Connect4_grid[0][CPU_x] = 2;
turn =0;
	break;	
}		
if(Connect4_grid[0][CPU_x] == 1){
				EnableInterrupts();	
				continue;
					}				
if(Connect4_grid[0][CPU_x] == 2){
				EnableInterrupts();	
					continue;	
					}		
turn = 0;		
break;		
turn = 0;					
					
		
		
  }
}

}


