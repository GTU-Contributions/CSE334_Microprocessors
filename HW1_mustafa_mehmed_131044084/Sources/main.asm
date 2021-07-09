;*****************************************************************
;* CSE331 Microprocessors - Exercise 1                           *
;* Mehmed Mustafa - 131044084                                    *
;* 12/03/2017                                                    *
;* Bonus Part is implemented                                     *
;*****************************************************************

; export symbols
            XDEF Entry, _Startup  ; export 'Entry' symbol
            ABSENTRY Entry        ; for absolute assembly: mark this as application entry point


; Include derivative-specific definitions 
		INCLUDE 'derivative.inc' 

ROMStart    EQU   $4000  ; absolute address to place my code/constant data
RESULT      EQU   $1500  ; The result of the expression
                         ; $1500-$1501 - integer part, 65535D max
                         ; $1502       - decimal part, 99D max
                         
            ORG   RAMStart
            
; Constants
            ORG   $11F0
DOT         DC.B  $2E     ; Keeps the ASCII value of the '.'                     
            ORG   $11F1
SPACE       DC.B  $20     ; Keeps the ASCII value of the SPACE
            ORG   $11F2
ASSIGN      DC.B  $3D     ; Keeps the ASCII value of the '='
            ORG   $11F3
PLUS        DC.B  $2B     ; Keeps the ASCII value of the '+'
            ORG   $11F4
MINUS       DC.B  $2D     ; Keeps the ASCII value of the '-'            

; Operands and Operator field            
            ORG   $1400
OPERAND1_I  DS.W  1       ; $1400-$1401
OPERAND1_D  DS.B  1       ; $1402
            ORG   $1403
OPERAND2_I  DS.W  1       ; $1403-$1404
OPERAND2_D  DS.B  1       ; $1405
OPERATOR    DS.B  1       ; $1406
            ORG   $1410   
OP_FIELD    DS.W  1       ; Starting address for storing the info about 
                          ; operands and operator
            ORG   $1413
OVERFLOW_S  DS.B  1       ; To check overflow over 65535

; Expression which will be calculated
            ORG   $1200
MYEXPR      FCC   "15060.30 + 20020.05 =" ; The expression to be calculated

;********************************************************************************************************
;* EXAMPLE INPUTS AND OUTPUTS TO CHECK ALL CASES:                                                       *
;* EX1, 15000.25(3A98.19H) +    25.80(0019.50H) = 15026.05(3AB2.05H), PORTB = 55 since no overflow      *
;* EX2, 65535.10(FFFF.0AH) +    31.75(001F.4BH) =    30.85(001A.55H), PORTB = FF since overflow occured *
;* EX3, 10501.20(2905.14H) -    40.80(0190.50H) = 10460.40(28DC.28H), PORTB = 55 since no overflow      *
;* EX4, 35500.50(8AAC.32H) - 15500.30(3C8C.1EH) = 20000.20(4E20.14H), PORTB = 55 since no overflow      *                                          
;*                                                                                                      *
;* Undefined actions when the first operand is smaller than the second and the operation is subtraction *
;* Undefined actions when the decimal part of an operand is a single digit                              *
;********************************************************************************************************

; code section
            ORG   ROMStart


Entry:
_Startup:
            LDS   #RAMEnd+1       ; Initialize the stack pointer
            LDX   #$1400
            STX   OP_FIELD        ; Store #$1400 in OP_FIELD
            LDX   #$1200          ; Load Register with the beggining of the MYEXPR
            
            ; Initializing Register Y to 0
            CLRA                  ; Clear A
            STAA  OVERFLOW_S      ; OVERFLOW_S = 0, overflow status for 65535
            CLRB                  ; Clear B
            PSHD                  ; Push D(AB)
            PULY                  ; Pull Y

; Read integer part of an Operand            
SCAN_I        
            CLRA                  ; Clear A every loop to ignore multiplication error
            LDAB  0, X            ; Load char from memory X into B
            CMPB  DOT             ; Compare B with M to see if the last loaded char is '.'
            BEQ   SCAN_D          ; Jump to SCAN_D to read decimal part if the char is '.'
            SUBB  #$30            ; Convert the char in B into a digit
            PSHB                  ; Save the digit in B
            LDAB  #10             ; Load 10 into B for multiplication the value in Y
            EMUL                  ; Multiply Y with D(AB) and store the result in D(AB)
            PSHD                  ; Push the result in D into Stack
            PULY                  ; Pull the result into Y
            PULB                  ; Load the saved digit into B
            ABY                   ; Y = Y + B
            INX                   ; ++X, process the next digit
            JMP SCAN_I            ; Jump to SCAN_I
            
; Read decimal part of an Operand         
SCAN_D      
            PSHX                  ; Store the old value of X in stack
            LDX   OP_FIELD        ; X = OP_FIELD
            STY   0, X            ; Store integer part of an Operand into memory
            INX                   ; ++X
            INX                   ; ++X, select the place of the decimal part of the first operand
            STX   OP_FIELD        ; OP_FIELD = X
            PULX                  ; Restore the old value of X from the stack
            
            INX                   ; Jump the '.'
            
            ; Read the ten's number
            LDAB  0, X            ; Load char from memory X into B
            SUBB  #$30            ; Convert the char in B into a digit
            LDAA  #10             ; A = 10
            MUL                   ; Multiply the number in B with 10
            LDY   OP_FIELD        ; Y = OP_FIELD
            STAB  0, Y            ; Y[0] = B
            
            ; Read the one's number
            INX                   ; ++X
            LDAB  0, X            ; Load char from memory X into B
            SUBB  #$30            ; Convert the char in B into a digit
            ADDB  0, Y            ; 
            STAB  0, Y            ;
            INY                   ; ++Y
            STY   OP_FIELD        ; OP_FIELD
            
            ; Clear Register Y
            CLRA                  ; A = 0
            CLRB                  ; B = 0
            PSHD                  ; Push D(value 0) into stack
            PULY                  ; Y = 0, clear Y for the next operand
            
            INX                   ; ++X, go to read the next operand
    
            ; Skip spaces, read operator and jump to CALCULATE when ASSIGN char is found        
SKIP        
            INX                   ; ++X
            LDAB  0, X            ; Load char from memory X into B
            CMPB  SPACE           ; Compare B with M to see if the last loaded char is SPACE
            BEQ   SKIP            ; Jump to SKIP if it's SPACE
            CMPB  ASSIGN          ; Compare B with M to see if the last loaded char is ASSIGN
            BEQ   CALCULATE       ; Jump to CALCULATE if it's ASSIGN
            STAB  OPERATOR        ; Since the char is not SPACE or ASSIGN it should be OPERATOR
            INX                   ; ++X
            INX                   ; ++X
            JMP   SCAN_I          ; Jump to SCAN_I to scan the second Operand
            
; Calculate the result according to the Operands and Operator
CALCULATE   
            CLRB                  ; B = 0         
            LDAA  OPERATOR        ; A = OPERATOR
            CMPA  PLUS            ; If the operator is +
            BEQ   ADD_            ; Jump to ADD_
            CMPA  MINUS           ; If the operator is -
            BEQ   SUB_            ; Jump to SUB_
            JMP   EXIT            ; Jump to EXIT if the operator is not + or -

; Addition            
ADD_
            LDAA  OPERAND1_D      ; A = Operand1 decimal part
            ADDA  OPERAND2_D      ; A = A + Operand2 decimal part
            CMPA  #100            
                                  ; If A < 100
            BLT   NO_SUBC_A       ; Jump to NO_SUBC_A
                                  ; Else
            SUBA  #100            ; A = A - 100
            LDAB  #1              ; B = 1, set flag for carry
            
NO_SUBC_A
            STAA  RESULT+2        ; Store the decimal part of the sum
            LDX   #$1400          ; X = OPERAND1_I address
            CLRA                  ; A = 0
            ADDD  X               ; D(AB) = D(AB) + X
                                  ; If overflow doesn't occurs > 65535
            BCC   NO_CARRY_A      ; Jump to NO_CARRY_A
                                  ; Else
            INC   OVERFLOW_S      ; ++OVERFLOW_S flag
NO_CARRY_A  
            LDY   #$1403          ; Y = OPERAND2_I address
            ADDD  Y
            STD   RESULT          ; Store the integet part of the result
            JMP   EXIT            ; Exit

; Subtraction
SUB_
            LDAA  OPERAND1_D      ; A = Operand1 decimal part
            LDAB  OPERAND1_D      ; B = Operand1 decimal part
            SUBA  OPERAND2_D      ; A = A - Operand2 decimal part
            CMPB  OPERAND2_D      ; Operand1 decimal part > Operand2 decimal part
            BGT   NO_SUBC_S       ; Jump to NO_SUBC_S
            PSHC                  ; Save condition register into stack
            CMPA  #156
                                  ; If A < 156
            BLT   NO_SUBC_S       ; Jump to NO_SUBC_S
                                  ; Else
            SUBA  #156            ; A = A - 156
            PULC                  ; Restore condition register from the stack
            
NO_SUBC_S            
            STAA  RESULT+2        ; Store the decimal part of the subtract
            LDX   #$1400          ; X = OPERAND1_I address
            LDY   #$1403          ; Y = OPERAND2_I address
            LDAA  1, X
            SBCA  1, Y
            STAA  RESULT+1        ; Store the result into the memory
            LDAA  0, X
            SBCA  0, Y
            STAA  RESULT          ; Store the result into the memory

            JMP   EXIT            ; Exit
    
                   
EXIT        
            LDAA  #$FF            ; A = $FF
            STAA  DDRB            ; Make Port B output
            STAA  PORTB           ; Set $FF in Port B
            BCS   EXIT            ; Jump to EXIT2 if there is a carry
            LDAA  #$1             ; A = 1
            CMPA  OVERFLOW_S      ; Check if there is carry over 65535
            BEQ   EXIT2           ; Jump to EXIT2 if so
            LDAA  #$55            ; A = $55
            STAA  PORTB           ; Set $55 in Port B since there is no carry
EXIT2      
            END

;**************************************************************
;*                 Interrupt Vectors                          *
;**************************************************************
            ORG   $FFFE
            DC.W  Entry           ; Reset Vector
