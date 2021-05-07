// DAC.c
// This software configures 4-bit DAC output

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data


// **************DAC_Init*********************
// Initialize 4-bit or 6-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x02;                  // Enable clock on port B
  while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};   // Wait for the clock to stabilize
  
  GPIO_PORTB_DIR_R   |=  0x0F;                // PB3-0 as outputs
  GPIO_PORTB_AFSEL_R &= ~0x0F;                // Disable alternate function
  GPIO_PORTB_AMSEL_R &= ~0x0F;                // Disable analog function
  GPIO_PORTB_DEN_R   |=  0x0F;                // Enable digital I/O on PB3-0
}

// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// or     6-bit data, 0 to 63
// Input=n is converted to n*3.3V/15
// or Input=n is converted to n*3.3V/63
// Output: none
void DAC_Out(uint8_t data){ // Changed the parameter that was passed from a 32 bit value to a 8 bit value to match with the prototype defined in the DAC header
  GPIO_PORTB_DATA_R = (uint32_t)(data & 0x0F); // Writes the 4 bit data to the DAC. The DAC circuit will convert the digital data into analog voltages accordingly.
  
  /* Minimally intrusive debugging */
  GPIO_PORTF_DATA_R ^= 0x02; // Toggle red every interrupt handle
}
