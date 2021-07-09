#ifndef _LCD_H
#define _LCD_H

/* HCS12 Dragon12 Development Board LCD */
#define LCD_DIR   DDRK   /* Direction of LCD port */
#define LCD_DAT   PORTK  /* Port K drives LCD data pins, E and RS */
#define LCD_E     0x02   /* LCD E Signal */
#define LCD_RS    0x01   /* LCD Register Select Signal */
#define LCD_CMD   0      /* Command Type for LCD */
#define LCD_DATA  1      /* Data Type for LCD */


/* Prototypes for functions in lcd.c */
void openlcd(void);
void put2lcd(char c, char type);
void puts2lcd(char *str, int ms);
void delay_50us(int times);     /* Delay times * 50 MicroSeconds */
void delay_1ms(int times);      /* Delay times * 1 MilliSeconds */

void clearDisplay(void);
void goTo1stLine(void);
void goTo1stLineProblemEnd(int size);
void goTo2ndLine(void);


/* Implementations of functions in lcd.c */
void openlcd(void){
     LCD_DIR = 0xFF;         /* Configure LCD port for output */
     delay_1ms(50);          /* Wait 50 ms for LCD to be ready */ 
     put2lcd(0x28, LCD_CMD); /* Set 4-bit data, 2-line display, 5x7 font */
	   put2lcd(0x0F, LCD_CMD); /* Turn on display, cursor, blinking */
   	 put2lcd(0x06, LCD_CMD); /* Move cursor right */
	   put2lcd(0x01, LCD_CMD); /* Clear screen, move cursor to home */
     delay_1ms(1);           /* Wait 1 ms until "Clear Display" command complete */    
}

void put2lcd(char c, char type){
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
     
     delay_50us(7);                 /* Wait 350 MicroS for Command to execute */
}

void puts2lcd(char *str, int ms){
     while(*str){
        put2lcd(*str, LCD_DATA);
        delay_1ms(ms);              /* Wait 350 MicroS for Data to be written */
        ++str; 
     }
}

// Board freq is 0.75 MHz, Prescaler 32
// 1 cycle = 1/0.75 MHz = 1,33 MicroS
// Inner loop: 9 cycles
// We need 855 cycles  = 95 x 9 Cycles 
#define M_50 95 
void delay_50us(int times){
    volatile int counter;
    for (; times>0; --times)
      for (counter=M_50; counter>0; counter--);
}

void delay_1ms(int times){
    for (; times>0; --times)
      delay_50us(20);
}

void clearDisplay(void){
    put2lcd(0x01, LCD_CMD); // Clear screen, move cursor to home
    delay_1ms(2);           // Wait 2 ms to command complete 
}

void goTo1stLine(void){
    put2lcd(0x80, LCD_CMD); // Go to the 1st line of the display
    delay_1ms(2);           // Wait 2 ms to command complete    
}

void goTo1stLineProblemEnd(int size){
    int counter;
    
    goTo1stLine();
    
    for(counter = 0; counter<size; ++counter){
      put2lcd(0x14, LCD_CMD);
    }
    delay_1ms(2);           // Wait 2 ms to command complete  
}

void goTo2ndLine(void){
    put2lcd(0xC0, LCD_CMD); // Go to the 2nd line of the display
    delay_1ms(2);           // Wait 2 ms to command complete  
}





#endif