#include <hidef.h>      /* common defines and macros */
#include "derivative.h" /* derivative-specific definitions */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "lcd.h"
#include "noteFreq.h"

#define TRUE 1
#define FALSE 0

#define BUFFER_SIZE 20
#define NO_KEY    -1

unsigned short JUNK_VALUES;
/*****************************************************************/
// KEYPAD MAPPING
#define KEY_SHARP 15
#define KEY_STAR  14
#define KEY_D     13
#define KEY_C     12
#define KEY_B     11
#define KEY_A     10

/*****************************************************************/
// SOUND DELAY SETTINGS 
#define PRESSED_KEY_SOUND_DELAY  2
#define WRONG_ANSWER_SOUND_DELAY 2
#define TIMEROVER_SOUND_DELAY    2

// Sounds are set on by default
// Change this with PH7 DIP Switch
unsigned int SOUNDS_ON;
unsigned int SENSOR_VALUE = 255;
/*****************************************************************/
// LCD DELAY SETTINGS
#define PROBLEM_DELAY 5
#define MSG_DELAY     75

/*****************************************************************/
// TIMER MODES
#define TIME_EASY   183 // 2.732 ms x 183 = 499.96 ms Per LED, ~4 sec total
#define TIME_NORMAL 138 // 2.732 ms x 138 = 377.02 ms Per LED, ~3 sec total
#define TIME_HARD    92 // 2.732 ms x 92  = 251,34 ms Per LED, ~2 sec total

/*****************************************************************/
// LEVEL SETTINGS
// Set on Easy mode as default
unsigned short PLAYER_TIME_MODE = TIME_EASY; // 4 sec total

// Generate problems with these variables
unsigned int MIN_NUMBER;
unsigned int MAX_NUMBER;
// 0 for +;
// 1 for +, -;
// 2 for +, -, %;
// 3 for +, -, %, /;
// 4 for +, -, %, / and *;
unsigned short MAX_OPERATOR;

// Current lives available
unsigned short LIVES;

// Current active level
unsigned short CURRENT_LEVEL;

/*****************************************************************/
// STATISTICS
unsigned short TOTAL_PROBLEMS;
unsigned short TOTAL_RIGHT_ANSWERS;
unsigned short TOTAL_PLAYER_POINTS;

/*****************************************************************/
// TIMERS
unsigned int RTI_PLAYER;
unsigned int TIMER_OVF;

// If the player doesn't enter the result on time raise this flag
short PLAYER_MISSED_FLAG;
// This value is bigger if the player answers faster
unsigned short TIMER_BONUS_POINT; 

/*****************************************************************/
// FUNCTIONS
void initializeSettings(void);
void initializeGame(void);
void enableRTI(void);
void disableRTI(void);

int GenRanNum(int from, int to);
int GenRanMathProblem(char *problem);

int GetKey(void);
void processLEDs(int player_rti);

void light7seg(int segPosition, int segValue);
void int7seg(int valueOn7seg);

void welcomeLCD(void);
void introLCD(void);
void gameOverLCD(void);

void playSingleNote(int freq, int duration);
void playKeyPress(int pressedKey);
void playWrongAnswer(void);
void playMissedTime(void);

void activateSensor(void){  
  DDRB = 0xFF;    //PORTB as output
  DDRJ = 0xFF;    //PTJ as output for Dragon12+ LEDs
  PTJ=0x0;        //Allow the LEDs to dsiplay data on PORTB pins
  
  ATD0CTL2 = 0x82;     //Turn on ATD0 with interrupt
  delay_1ms(15);
  ATD0CTL3 = 0x08;  //one conversion, no FIFO
  ATD0CTL4 = 0xEB;  //8-bit resolu, 16-clock for 2nd phase,
                    //prescaler of 24 for Conversion Freq=1MHz
  ATD0CTL5 = 0x84;  //Channel 4 (right justified, unsigned,single-conver,one chan only)   
}

interrupt (((0x10000-Vatd0)/2)-1) void SENSOR_ISR(void){
    SENSOR_VALUE = ATD0DR0L;  // Save the sensor result on SENSOR_VALUE
    // If the sensor is triggered, game over
    if(SENSOR_VALUE < 7)
      LIVES = 0;
    ATD0CTL5 = 0x84;   // Start converting Channel 4    
}

void levelLoop(int level){
  int pressedKey;
  int userAnswer, size, nextSize, correctAnswer, nextCorrectAnswer;  
  char problem[BUFFER_SIZE], nextProblem[BUFFER_SIZE];
  char levelMsg[BUFFER_SIZE];
  
  sprintf(levelMsg, "LEVEL: %d", level);
  puts2lcd(levelMsg, MSG_DELAY);
  goTo2ndLine();
  sprintf(levelMsg, "3 2 1 GO!!!");
  puts2lcd(levelMsg, MSG_DELAY*3);
  clearDisplay();
  
  // Generate first math problem 
  correctAnswer = GenRanMathProblem(problem);
  size = strlen(problem);
  // Show first line problem on the screen
  puts2lcd(problem, PROBLEM_DELAY);
  ++TOTAL_PROBLEMS;

  // Loop while you have lives  
  while(LIVES)
  { 
  
    // Generate second line problem
    nextCorrectAnswer = GenRanMathProblem(nextProblem);
    nextSize = strlen(nextProblem);
    // Go to the 2nd line
    goTo2ndLine();
    // Print the second line problem
    if(TOTAL_PROBLEMS%5 != 0){
      puts2lcd(nextProblem, PROBLEM_DELAY);  
    }else{
      sprintf(nextProblem, "NEXT LEVEL %d", ++level);
      puts2lcd(nextProblem, PROBLEM_DELAY);
    }
    
    // Go to the 1st line problem end to enter answer
    goTo1stLineProblemEnd(size);
   
    pressedKey = NO_KEY;
    userAnswer = 0;
    
    // Start the player's time
    enableRTI();
    
    do{
      PLAYER_MISSED_FLAG = FALSE;
      pressedKey = GetKey();
      
      if(pressedKey >= 0 && pressedKey <= 9){
        put2lcd(pressedKey+48, LCD_DATA);
        if(SOUNDS_ON) 
          playKeyPress(pressedKey);
        userAnswer = userAnswer * 10;
        userAnswer = userAnswer + pressedKey;
      }
      
      if(PLAYER_MISSED_FLAG){
        userAnswer = -1;
        break;  
      }
        
      if(pressedKey == KEY_STAR){
        if(SOUNDS_ON) 
          playKeyPress(KEY_STAR);  
      }
      
    }while(pressedKey != KEY_STAR);
    
    // If the player's time is over OR player's answer is wrong
    if(PLAYER_MISSED_FLAG || (userAnswer != correctAnswer)){
      --LIVES;
      
      if(SOUNDS_ON){
        if(PLAYER_MISSED_FLAG) 
          playMissedTime(); 
        else 
          playWrongAnswer();   
      }
    }else{
      ++TOTAL_RIGHT_ANSWERS;
      if(PLAYER_TIME_MODE == TIME_EASY){
        TOTAL_PLAYER_POINTS += TIMER_BONUS_POINT;  
      } 
      else if(PLAYER_TIME_MODE == TIME_NORMAL){
        TOTAL_PLAYER_POINTS += TIMER_BONUS_POINT*2;  
      }
      else if(PLAYER_TIME_MODE == TIME_HARD){
        TOTAL_PLAYER_POINTS += TIMER_BONUS_POINT*3;  
      }
      
    }
    
    // Stop the player's time;
    disableRTI();
	
	  // After 5 problems from this level
    // Go to the next level
    if(TOTAL_PROBLEMS%5 == 0){
      clearDisplay();
      return;  
    } 
	
    // Make the second line problem first line problem 
    // for better tracking when playing

    strncpy(problem, nextProblem, 20);
    size = nextSize;
    correctAnswer = nextCorrectAnswer;
    
    clearDisplay();    
    // Print the old second line problem to the first line
    puts2lcd(problem, PROBLEM_DELAY);
    ++TOTAL_PROBLEMS;
    
  }  
}

// PortH interrupt
interrupt ((0x10000 - Vporth)/2 - 1) void ISR_PORTH(void)
{

    if(PTH == 0x01){
      PIFH |= 0x01;
      MAX_NUMBER = 20;  
    }  
    else if(PTH == 0x02){
      PIFH |= 0x02;
      MAX_NUMBER = 25;
    }   
    else if(PTH == 0x04){
      PIFH |= 0x04;
      MAX_NUMBER = 30;;
    }
    else if(PTH == 0x08){
      PIFH |= 0x08;
      MAX_NUMBER = 35;
    }
    else if(PTH == 0x10){
      PIFH |= 0x10;
      MAX_NUMBER = 40;
    }
    else if(PTH == 0x20){
      PIFH |= 0x20;
      MAX_NUMBER = 45;
    }
    else if(PTH == 0x40){
      PIFH |= 0x40;
      SOUNDS_ON = TRUE;
    }
    else if(PTH == 0x80){
      PIFH |= 0x80;
      SOUNDS_ON = FALSE;
    }
}

// One interrupt occurs every 10.92 ms
interrupt (((0x10000-Vrti)/2) - 1) void RTI_ISR(void){
    ++RTI_PLAYER;
    
    // Process LED's and calculate the Bonus Points
    processLEDs(RTI_PLAYER);

    // Clear RTI flag
    CRGFLG = CRGFLG | CRGFLG_RTIF_MASK;
}

void main(void) {
  int seg7counter;
  char modeMsg[16];
  
  // Initialize the game (Keypad, Timers, LCD)
  initializeSettings(); 
  // Print the developer name
    welcomeLCD(); 
      
  activateSensor();

  while(TRUE){
    // Initialize the game (Keypad, Timers, LCD)
    initializeSettings(); 
    
    // Print the begging instructions
    introLCD();
  
    // Reset and set global variables
    initializeGame();
    
    while(LIVES){
      /*
      if(CURRENT_LEVEL == 0){
        PLAYER_TIME_MODE = TIME_EASY;
        sprintf(modeMsg, "GAME EASY NOW");
        puts2lcd(modeMsg, MSG_DELAY);
        clearDisplay();          
      }
      else if(CURRENT_LEVEL == 3){
        PLAYER_TIME_MODE = TIME_NORMAL;
        sprintf(modeMsg, "GAME NORMAL NOW");
        puts2lcd(modeMsg, MSG_DELAY);
        clearDisplay();  
      }else if(CURRENT_LEVEL = 6){
        PLAYER_TIME_MODE = TIME_HARD;
        sprintf(modeMsg, "GAME HARD NOW");
        puts2lcd(modeMsg, MSG_DELAY);
        clearDisplay();
      }*/
      levelLoop(CURRENT_LEVEL);
      ++CURRENT_LEVEL;
      // Print level message

      // Start and play the current level

    }
    
    // Pring Game Over Messages on the LCD
    gameOverLCD();
    
    seg7counter = 0;
    // Show Total Points on 7-Segment
    // For 5 seconds
    while(seg7counter < 300){
      int7seg(TOTAL_PLAYER_POINTS);
      ++seg7counter;  
    }
    
    _FEED_COP();    
  }
  /* please make sure that you never leave main */
}


// Initialize the settings
void initializeSettings(void){

    DisableInterrupts;
    
    // ENABLE KEYPAD
  	DDRA  = 0x0F;   // PortA: 7-4(ROWS) INPUT, PortA: 3-0(COLUMNS) OUTPUT
  	DDRT  = 0xFF;   // PortT as output
  	
    // Activate led array on PortB	
    //DDRJ |= 0x02; 
    //PTJ  &= ~0x02;  
    
    // Set PortB to output for LEDs
  	DDRB = 0xFF;
  	PORTB = 0x00;        
  	
  	// DISABLE 7-SEGMENT
  	DDRP  = 0x0F;       // Set PortP to output, (to turn off 7-seg display).
  	PTP   = 0x0F;       // PP3:0 are 7-segment common cathodes: turn them off.
    
    
    // TIMER PROPERTIES 
  	TSCR1 = 0x80; // Turn on system timer
  	 
  	// Board freq is 0.75 MHz, Prescaler 32
    // 1 cycle = 1/0.75 MHz = 1,33 MicroS
    // 12 overflows are needed for 1 second delay 
  	TSCR2 = 0x05;
  	
  	// Clear overflow flag
  	TFLG2 = 0x80;
     
    PIEH  = 0xFF; // Enable all PortH interrupts
    PPSH  = 0xFF; // Rising edge pulse will activate the interrupt
  	
  	// START THE LCD
  	openlcd();
  	
  	delay_1ms(75);
    
    EnableInterrupts;  
}

void initializeGame(void){
    DisableInterrupts;
    
    // Set starting level
    CURRENT_LEVEL = 0;
  
    // Reset all counters when the game is started;
    TOTAL_PROBLEMS = 0;
    TOTAL_RIGHT_ANSWERS = 0;
    TOTAL_PLAYER_POINTS = 0;
    
    // Reset all timers
    RTI_PLAYER = 0;
    TIMER_OVF = 0;
    TIMER_BONUS_POINT = 0;
    PLAYER_MISSED_FLAG = FALSE;
    
    // Set starting lives
    LIVES = 3;
    
    // Set operands range in easy
    MIN_NUMBER = 0;
    MAX_NUMBER = 9;

    // Set operator vaariants
    MAX_OPERATOR = 4;
    
    // Turn on the sounds
    SOUNDS_ON = TRUE;
    // Set sensor value to 255 
    SENSOR_VALUE = 255;    
    
    EnableInterrupts;   
}

// Enable Real Timer
void enableRTI(void){
    // Enabling  RTI
    // 1 cycle = 1/24 MHz = 0,416 nS
    // 0,416 nS x 2^16 = 2.731 ms
    RTI_PLAYER = 0; // Reset player real timer interrupt
    CRGINT = 0x80;  // Enable RTI interrupts 
    RTICTL = 0x60;  // RTR[6:4] = 111 (2^16), RTR[3:0] = 0000 (0) 
    CRGFLG = 0x80;  // Clear RTI Flag    
}

// Disable Real Timer
void disableRTI(void){
    CRGINT = 0x00; // Disable RTI interrupts 
    RTICTL = 0x00; // Set it to default
    CRGFLG = 0x80; // Clear RTI Flag
    PORTB  = 0x00; // Disable all leds  
}

// Generate random integer between from-to inclusive
int GenRanNum(int from, int to){
     // Set the seed to Timer Counter + Timer Bonus Point
     srand(TCNT);
  
     // Also use Real Time Timer and 
     // Timer Overflows for more randomization
     return ((rand()-RTI_PLAYER+TIMER_OVF) % (to+1)) + from;
}

// Generate random math problem
int GenRanMathProblem(char *problem){
     int num1, num2, temp, intOp, answer;
     char op;
     
     // Generate the operands
     num1 = GenRanNum(MIN_NUMBER, MAX_NUMBER);
     // Delay random time to get more random operands
     delay_1ms(TIMER_BONUS_POINT);
     num2 = GenRanNum(MIN_NUMBER, MAX_NUMBER);
     
     // Always make the first operand bigger
     if(num2 > num1){
        temp = num2;
        num2 = num1;
        num1 = temp; 
     }
     
     // Generate operator
     intOp = GenRanNum(0, MAX_OPERATOR);
     
     // If the operator is division and the second num is 0
     if((intOp == 2 || (intOp == 3)) && num2 == 0){
        // Generate new operand while the new operand is not 0
        while(num2 == 0)
          num2 = GenRanNum(MIN_NUMBER, MAX_NUMBER);
          
        // Always make the first operand bigger
        if(num2 > num1){
          temp = num2;
          num2 = num1;
          num1 = temp; 
       }   
     }
     
     // Set operator
     switch(intOp){
        case 0: op = '+'; answer = num1 + num2; break;
        case 1: op = '-'; answer = num1 - num2; break;
        case 2: op = '%'; answer = num1 % num2; break;
        case 3: op = '/'; answer = num1 / num2; break;
        case 4: op = '*'; answer = num1 * num2; break;
        default: op = 'E'; answer = temp; 
     }
     
     // Write the problem into buffer
     sprintf(problem, "%d%c%d%c", num1, op, num2, '=');
     
     return answer;       
}

// Gets pressed key number from keypad
// Reference: http://www.microdigitaled.com/HCS12/Hardware/Dragon12/CodeWarrior/CW_Keypad_to_PORTB_CProg.txt
// I modified this function a bit
int GetKey(void)
{  
  const unsigned int keypad[4][4] = 
  { 1, 2, 3, KEY_A,
    4, 5, 6, KEY_B,
    7, 8, 9, KEY_C,
    KEY_STAR, 0, KEY_SHARP, KEY_D };

    int row, column, pressedKey = -1;
    
    do{                                 //OPEN do1
       PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(PLAYER_MISSED_FLAG)
          return pressedKey;
    }while(row == 0x00);                //WAIT UNTIL KEY PRESSED //CLOSE do1


    do{                                 //OPEN do2
       do{                              //OPEN do3
          delay_1ms(1);                   //WAIT
          row = PORTA & 0xF0;           //READ ROWS
       }while(row == 0x00);             //CHECK FOR KEY PRESS //CLOSE do3
       
       delay_1ms(15);                     //WAIT FOR DEBOUNCE
       row = PORTA & 0xF0;
    }while(row == 0x00);                //FALSE KEY PRESS //CLOSE do2

    while(1){                           //OPEN while(1)
       if(PLAYER_MISSED_FLAG)
          return pressedKey;
       PORTA &= 0xF0;                   //CLEAR COLUMN
       PORTA |= 0x01;                   //COLUMN 0 SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(row != 0x00){                 //KEY IS IN COLUMN 0
          column = 0;
          break;                        //BREAK OUT OF while(1)
       }
       PORTA &= 0xF0;                   //CLEAR COLUMN
       PORTA |= 0x02;                   //COLUMN 1 SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(row != 0x00){                 //KEY IS IN COLUMN 1
          column = 1;
          break;                        //BREAK OUT OF while(1)
       }

       PORTA &= 0xF0;                   //CLEAR COLUMN
       PORTA |= 0x04;                   //COLUMN 2 SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(row != 0x00){                 //KEY IS IN COLUMN 2
          column = 2;
          break;                        //BREAK OUT OF while(1)
       }
       PORTA &= 0xF0;                   //CLEAR COLUMN
       PORTA |= 0x08;                   //COLUMN 3 SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(row != 0x00){                 //KEY IS IN COLUMN 3
          column = 3;
          break;                        //BREAK OUT OF while(1)
       }
       
       row = 0;                         //KEY NOT FOUND
       break;                           //step out of while(1) loop to not get stuck
    }                                   //end while(1)

    if(row == 0x10){
       pressedKey=keypad[0][column];         //OUTPUT TO PORTB LED

    }
    else if(row == 0x20){
       pressedKey=keypad[1][column];

    }
    else if(row == 0x40){
       pressedKey=keypad[2][column];

    }
    else if(row == 0x80){
       pressedKey=keypad[3][column];

    }

    do{
       delay_1ms(15);
       PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
       row = PORTA & 0xF0;              //READ ROWS
       if(PLAYER_MISSED_FLAG)
          return pressedKey;
    }while(row != 0x00);                //MAKE SURE BUTTON IS NOT STILL HELD

    return pressedKey; 
}

// Lighten the leds according to the Real Timer
// And calculate the bonus points from the timer
void processLEDs(int player_rti){

    // 1. LED
    if(player_rti == PLAYER_TIME_MODE){
      PORTB = 0x01;
      TIMER_BONUS_POINT = 7;  
    }
    // 2. LED 
    else if(player_rti == 2*PLAYER_TIME_MODE){
      PORTB = 0x03;
      TIMER_BONUS_POINT = 6;
    }
    // 3. LED
    else if(player_rti == 3*PLAYER_TIME_MODE){
      PORTB = 0x07;
      TIMER_BONUS_POINT = 5;
    }
    // 4. LED
    else if(player_rti == 4*PLAYER_TIME_MODE){
      PORTB = 0x0F;
      TIMER_BONUS_POINT = 4;
    }
    // 5. LED
    else if(player_rti == 5*PLAYER_TIME_MODE){
      PORTB = 0x1F;
      TIMER_BONUS_POINT = 3;
    }
    // 6. LED
    else if(player_rti == 6*PLAYER_TIME_MODE){
      PORTB = 0x3F;
      TIMER_BONUS_POINT = 2;
    }
    // 7. LED
    else if(player_rti == 7*PLAYER_TIME_MODE){
      PORTB = 0x7F;
      TIMER_BONUS_POINT = 1;
    }
    // 8. LED
    else if(player_rti == 8*PLAYER_TIME_MODE){
      PORTB = 0xFF;
      // Reset the real time timer for the player time
      RTI_PLAYER = 0;
      // When the player haven't enter the result on time
      PLAYER_MISSED_FLAG = TRUE; 
    }  
}

// Print a Digit (segValue) on the desired 
// Position (segPosition) 1-4 where 1 is MSD and 4 is LSD
void light7seg(int segPosition, int segValue){
    DDRP |= 0xF0;       // Set PortP to output, (to turn off 7-seg display).
    PTP  |= 0xF0;       // PP3:0 are 7-segment common cathodes: turn them off.

    // Select 7-segment position 
    if(segPosition == 1){
      PTP = 0x0E;  
    }else if(segPosition == 2){
      PTP = 0x0D;  
    }else if(segPosition == 3){
      PTP = 0x0B;  
    }else if(segPosition == 4){
      PTP = 0x07;  
    }
    
    // Select value to be showed
    if(segValue == 0){
      PORTB = 0x3F;  
    }else if(segValue == 1){
      PORTB = 0x06;
    }else if(segValue == 2){
      PORTB = 0x5B;
    }else if(segValue == 3){
      PORTB = 0x4F;
    }else if(segValue == 4){
      PORTB = 0x66;  
    }else if(segValue == 5){
      PORTB = 0x6D;
    }else if(segValue == 6){
      PORTB = 0x7D;  
    }else if(segValue == 7){
      PORTB = 0x27;
    }else if(segValue == 8){
      PORTB = 0x7F;
    }else if(segValue == 9){
      PORTB = 0x6F;
    // To set -
    }else if(segValue == 11){
      PTP = ~0x0E;  
    }else if(segValue == 12){
      PTP = ~0x0D;  
    }else if(segValue == 13){
      PTP = ~0x0B;
    }
    
    delay_1ms(5);
    
}

// Print an 4 digit integer on the 7-segment
// when the integer is > 9999, always printing 9999
void int7seg(int valueOn7seg){
  int remaining, thousands, hundreds, tens, ones;
  int biggerFrom4Digits = FALSE;
  
  // Get the thousands part
  thousands = valueOn7seg/1000;
  if(thousands >=10){
    biggerFrom4Digits = TRUE;  
  }
  remaining = valueOn7seg%1000;
  
  // Get the hundreds part
  hundreds = remaining/100;
  remaining = remaining%100;
  
  // Get the tens part
  tens = remaining/10;
  remaining = remaining%10;
  
  // Get the ones part
  ones = remaining;
  
  // Set - instead of zero
  if(thousands == 0){
    thousands = 11;
    // Set - instead of zero
    if(hundreds == 0){
      hundreds = 12;
      // Set - instead of zero
      if(tens == 0){
        tens = 13;  
      }
    }
  }
  
  // If Points > 9999, always print 9999, the max number
  if(biggerFrom4Digits){
    thousands = 9;
    hundreds = 9;
    tens = 9;
    ones = 9;  
  }

  // Turn 7-segment on
  PTP |= 0xF0;

  light7seg(1, thousands);
  light7seg(2, hundreds);
  light7seg(3, tens);
  light7seg(4, ones);
    
  // Turn 7-segment off
  PTP = 0x0F;
  PORTB = 0x00;
   
}

void welcomeLCD(void){
    puts2lcd("Welcome to my", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("game!", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("My name is", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("Mehmed!", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("Use the keypad", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("0-9 to play.", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("Use the * key", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("after each answr", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("3 wrong answers", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("--> GAME OVER!", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("Each wrong answr", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("will alert you.", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();
    
    puts2lcd("Answer in time", MSG_DELAY);
    goTo2ndLine();
    puts2lcd("or you will burn", MSG_DELAY);
    delay_1ms(500);
    clearDisplay();    
}

void introLCD(void){

  puts2lcd("Press any keypad", MSG_DELAY);
  goTo2ndLine();
  puts2lcd("key to start!", MSG_DELAY);
  
  // Press any key to continue
  JUNK_VALUES = GetKey(); 
  
  clearDisplay();  
}

void gameOverLCD(void){
  char endGame[BUFFER_SIZE];

  clearDisplay();
  // Print the game over text
  puts2lcd("GAME OVER!!!", MSG_DELAY);
  
  delay_1ms(800);
  clearDisplay();
  
  sprintf(endGame, "TotalQuizes:%d", --TOTAL_PROBLEMS);
  puts2lcd(endGame, MSG_DELAY);
  goTo2ndLine();
  
  sprintf(endGame, "RightAnswer:%d", TOTAL_RIGHT_ANSWERS);
  puts2lcd(endGame, MSG_DELAY);
  
  delay_1ms(800);
  clearDisplay();
  
  sprintf(endGame, "TotalPoints:%d", TOTAL_PLAYER_POINTS);
  puts2lcd(endGame, MSG_DELAY); 
}

void playSingleNote(int freq, int duration){
    // Timer prescaler is set to 32
    // 1 cycle = 1/0.75 MHz = 1,33 MicroS
    int period = (freq*4)/3;
    
    TIOS = 0x20;   // Set Channel 5 for Output compare
    TCTL1 = 0x04;  // Toggle Buzzer
    
    TFLG2 = 0x80;   // Clear timer flag
    while(duration > 0){
      TC5 = TCNT + period;
      while(!(TFLG1 & TFLG1_C5F_MASK)); // Wait for Channel 5 to match
      TFLG1 |= TFLG1_C5F_MASK;  // Clear Channel 5 Flag  
      
      if(TFLG2){
        TFLG2 = 0x80; // Clear the timer flag
        ++TIMER_OVF;  // Increase timer overflow flag
        --duration;          
      }
      
    }
    
    TCTL1 = 0x00;   // Disconnect Buzzer
    TIOS = 0x20;    // Clear Channel 5     
}

void playKeyPress(int pressedKey){
  switch(pressedKey){
    case 0:  playSingleNote(NOTE_C5, PRESSED_KEY_SOUND_DELAY); break;
    case 1:  playSingleNote(NOTE_CS5, PRESSED_KEY_SOUND_DELAY); break;
    case 2:  playSingleNote(NOTE_D5, PRESSED_KEY_SOUND_DELAY); break;
    case 3:  playSingleNote(NOTE_DS5, PRESSED_KEY_SOUND_DELAY); break;
    case 4:  playSingleNote(NOTE_E5, PRESSED_KEY_SOUND_DELAY); break;
    case 5:  playSingleNote(NOTE_F5, PRESSED_KEY_SOUND_DELAY); break;
    case 6:  playSingleNote(NOTE_FS5, PRESSED_KEY_SOUND_DELAY); break;
    case 7:  playSingleNote(NOTE_G5, PRESSED_KEY_SOUND_DELAY); break;
    case 8:  playSingleNote(NOTE_GS5, PRESSED_KEY_SOUND_DELAY); break;
    case 9:  playSingleNote(NOTE_A5, PRESSED_KEY_SOUND_DELAY); break;
    // Sound for * key
    case KEY_STAR: playSingleNote(NOTE_AS5, PRESSED_KEY_SOUND_DELAY); break;
    // Sound for # key
    case KEY_SHARP: playSingleNote(NOTE_B5, PRESSED_KEY_SOUND_DELAY); break;
  }   
}

void playWrongAnswer(void){
  playSingleNote(NOTE_C2, WRONG_ANSWER_SOUND_DELAY);
  playSingleNote(NOTE_C3, WRONG_ANSWER_SOUND_DELAY*2);
  playSingleNote(NOTE_C2, WRONG_ANSWER_SOUND_DELAY);  
}

void playMissedTime(void){
  playSingleNote(NOTE_C6, TIMEROVER_SOUND_DELAY);
  playSingleNote(NOTE_C7, TIMEROVER_SOUND_DELAY*2);
  playSingleNote(NOTE_C6, TIMEROVER_SOUND_DELAY);  
}

