; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; SSD1306_OutChar   outputs a single 8-bit ASCII character
; SSD1306_OutString outputs a null-terminated string 
; SSD1306_OutString outputs a null-terminated string 

    IMPORT   SSD1306_OutChar
    IMPORT   SSD1306_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix
    PRESERVE8
    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB
N	EQU	4
CNT	EQU 0
FP	RN	R11	; frame pointer


;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec
	  ;INIT 1
	  PUSH{R11, R0}
	  SUB SP, #4 ; making space for CNT, #4 for 4 bytes = 32 bits
	  MOV FP, SP ; frame pointer = stack pointer
	  ;INIT 2
	  PUSH{LR}
	  MOV R1, #0
	  STR R1, [FP, #CNT] ; making CNT = 0
	  MOV R2, #10
Read_Loop
	  LDR R1, [FP, #CNT] ; reading count
	  ADD R1, #1 ; incrementing count
	  STR R1, [FP, #CNT] ; storing count
	  LDR R1, [FP, #N] ; reading N which is NUM
	  MOV R3, R1 ; making copy of N into R3
	  UDIV R1, R1, R2 ; R1 = N/10
	  STR R1, [FP, #N] ; storing n/10 as new N
	  MUL R1, R1, R2 
	  SUB R3, R3, R1 ; R3 = R3-R1
	  SUB SP, #4 ; decrementing stack pointer one position
	  STR R3, [SP] ; storing difference into stack
	  LDR R1, [FP, #N] ; load new N value
	  CMP R1, #0 ; is new N = 0
	  BNE Read_Loop ; No? loop back to read loop. Yes? Continue.
Write_Loop
	  LDR R0, [SP]
	  ADD SP, #4 ; incrementing stack pointer one position
	  ADD R0, #0x30 ; adding 0x30 to get ASCII value
	  BL SSD1306_OutChar
	  LDR R1, [FP, #CNT] ; loading count into R1
	  SUB R1, #1 ; decrementing count
	  STR R1, [FP, #CNT] ; storing CNT from R1
	  CMP R1, #0 ; is CNT(R1) greater than 0?
	  BNE Write_Loop ; Yes? Loop back to write loop 
	  ;No? Continue
	  POP{LR}
	  ADD SP, #8
	  POP{R11}


      BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.03 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix
	 ;INIT 1
	 PUSH{R11, R0}					
	 SUB SP, #4 ; making space for CNT
	 MOV FP, SP ; frame pointer = stack pointer
	 ;INIT 2
	 PUSH{LR}
	 MOV R1, #0
	 STR R1, [FP, #CNT] ; making CNT = 0
	 MOV R2, #10
	 
	 LDR R3, [FP, #N] ; loading N into R3
	 CMP R3, #1000 ; is N < 1000?
	 BLO InRange ; Yes? jump to InRange
	 ;No? Continue
	 ;Output for out of range nums: *.**
	 MOV R0, #0x2A ; R0 = *
	 BL SSD1306_OutChar
	 MOV R0, #0x2E ; R0 = .
	 BL SSD1306_OutChar
	 MOV R0, #0x2A ; R0 = *
	 BL SSD1306_OutChar
	 MOV R0, #0x2A ; R0 = *
	 BL SSD1306_OutChar
	 B Exit
InRange
	 LDR R1, [FP, #CNT]; reading count
	 ADD R1, #1; incrementing count
	 STR R1, [FP, #CNT]; storing count
	 LDR R1, [FP, #N]; loading N
	 MOV R3, R1; making copy of N into R3
	 UDIV R1, R1, R2; R1 = N/10
	 STR R1, [FP, #N]; Storing N/10 as new N
	 MUL R1, R1, R2
	 SUB R3, R3, R1 ; R3 = R3 - R1
	 SUB SP, #4 ; decrementing stack pinter one position
	 STR R3, [SP] ; storing difference into stack
	 LDR R1, [FP, #CNT] ; loading count
	 CMP R1, #3 ;compare count to 3
	 BLO InRange ; Less than? Loop to InRange
	 ;No? Continue
	 LDR R0, [SP]
	 ADD SP, #4 ; incrementing stack pointer one position
	 ADD R0, #0x30 ; adding 0x30 to get ASCII value
	 BL SSD1306_OutChar
	 MOV R0, #0x2E ; outputting a decimal 
	 BL SSD1306_OutChar
	 LDR R0, [SP] 
	 ADD SP, #4
	 ADD R0, #0x30
	 BL SSD1306_OutChar
	 LDR R0, [SP]
	 ADD SP, #4
	 ADD R0, #0x30
	 BL SSD1306_OutChar
Exit
	 POP{LR}
	 ADD SP, #8
	 POP{R11}

     BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN          ; make sure the end of this section is aligned
     END            ; end of file
