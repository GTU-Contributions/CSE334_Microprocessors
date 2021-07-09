#include "lcd.h"    /* Functions needed for the LCD */

void delay_1ms(int times){
     unsigned int i, j;
     for(i=0; i<times; ++i)
        for(j=0; j<1000; ++j);
}

void ConfigureSerial(){
     SCI0BDH = 0x00; // Choose
     SCI0BDL = 0x1A; // Baud Rate To Communicate with the Board
     SCI0CR1 = 0x00; // 8-bit data, no interrupt
     SCI0CR2 = 0xAC; // enable transmit and receive and their interrupts
}

void SerialSend(unsigned char c){
     // wait to send if the buffer is full
     while(!(SCI0SR1 & SCI0SR1_TDRE_MASK));
     // place value in buffer
     SCI0DRL = c;
}

void SerialReceive(){
     // wait to receive
     while(!(SCI0SR1 & SCI0SR1_RDRF_MASK));
     // place received value in a global SerialChar
     SerialChar = SCI0DRL;
}

void ShowCharLCD(char c, char type){
     char c_low, c_high;   /* Low and High 4 Bits of a char */
      
     c_high = (c & 0xF0) >> 2;      /* Set the High 4 Bits in 5-2 area */
     c_low  = (c & 0x0F) << 2;      /* Set the Low  4 Bits in 5-2 area */
     
     LCD_DAT = LCD_DAT & (~LCD_RS); /* Select LCD Command Register */
     
     /* Set High 4 Bits of the C */
     if(type == LCD_DATA)
        LCD_DAT = c_high | LCD_RS;  /* Output High 4 Bits, E, RS high */
     else
        LCD_DAT = c_high; 
        
     LCD_DAT = LCD_DAT | LCD_E;     /* Set E Signal to High */
     /* Lengthen E Signal, wait 8 nops */
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     LCD_DAT = LCD_DAT & (~LCD_E);  /* Set E Signal to Low */ 
     
     /* Set Low 4 Bits of the C */     
     if(type == LCD_DATA)
        LCD_DAT = c_low | LCD_RS;   /* Output Low 4 Bits, E, RS high */
     else
        LCD_DAT = c_low;
     
     LCD_DAT = LCD_DAT | LCD_E;     /* Set E Signal to High */
     /* Lengthen E Signal, wait 8 nops */
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     __asm(nop);
     LCD_DAT = LCD_DAT & (~LCD_E);  /* Set E Signal to Low */ 
     
     delay_1ms(10);                 /* Wait 50 MikroS for Command to execute */
}

void ShowStringLCD(char *str, int delay){
     while(*str){
        ShowCharLCD(*str, LCD_DATA);
        delay_1ms(delay);           /* Wait for Data to be written */
        ++str; 
     }
}

void StartLCD(void){
     LCD_DIR = 0xFF;          /* Configure LCD port for output */
     delay_1ms(100);          /* Wait 100 ms for LCD to be ready */ 
     ShowCharLCD(0x28, LCD_CMD);
     ShowCharLCD(0x0F, LCD_CMD);
     ShowCharLCD(0x06, LCD_CMD);
     ShowCharLCD(0x01, LCD_CMD);
     delay_1ms(10);           /* Wait 2 ms until "Clear Display" command complete */
     
}

void PrintSerialMenu(){
     char *buffer = "Please enter a character: ";
     int counter = 0;
     
     SerialSend(10);
     SerialSend(13);
     while(*buffer){
        SerialSend(*buffer);
        ++buffer;          
     }
}

/*
interrupt (((0x10000-Vsci0)/2)-1) void SCI0_ISR(void){
     // wait to receive
     if(SCI0SR1 & SCI0SR1_RDRF_MASK){
        SerialChar = SCI0DRL;
        ShowCharLCD(SerialChar, LCD_DATA);
        SCI0DRL = SerialChar;
        ++lineCounter;
        if(lineCounter == 16){
          ShowCharLCD(0xC0, LCD_CMD);// Move cursor to 2nd row, 1st column   
        }
     }     
}
*/