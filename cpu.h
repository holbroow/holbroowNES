#include <stdint.h>

//// Registers and Important Vars
#define stackp uint8_t;     // Stack pointer
#define accum uint8_t;      // Accumulator
#define x uint8_t;          // X Register
#define y uint8_t;          // Y Register
#define status uint8_t;     // Status Register

// Flags for structure of Status Register. (Although I've learned that the register can be accessed directly...)
enum STATUS_FLAGS {
    C = (1 << 0), 
    Z = (1 << 1), 
    I = (1 << 2), 
    D = (1 << 3), 
    B = (1 << 4), 
    NA = (1 << 5), 
    O = (1 << 6), 
    N = (1 << 7), 
};

//// 6502 Instruction Set (Really helpful resource at https://www.nesdev.org/obelisk-6502-guide/instructions.html)
// Load/Store Operations
uint8_t LDA();
uint8_t LDX();
uint8_t LDY();
uint8_t STA();
uint8_t STX();
uint8_t STY();
// Register Transfers
uint8_t TAX();
uint8_t TAY();
uint8_t TXA();
uint8_t TYA();
// Stack Operations
uint8_t TSX();
uint8_t TXS();
uint8_t PHA();
uint8_t PHP();
uint8_t PLA();
uint8_t PLP();
// Logical Instructions
uint8_t AND();
uint8_t EOR();
uint8_t ORA();
uint8_t BIT();
// Arithmetic Instructions
uint8_t ADC();
uint8_t SBC();
uint8_t CMP();
uint8_t CPX();
uint8_t CPY();
// Increments & Decrements
uint8_t INC();
uint8_t INX();
uint8_t INY();
uint8_t DEC();
uint8_t DEX();
uint8_t DEY();
// Shifts
uint8_t ASL();
uint8_t LSR();
uint8_t ROL();
uint8_t ROR();
// Jumps & Calls
uint8_t JMP();
uint8_t JSR();
uint8_t RTS();
// Branches
uint8_t BCC();
uint8_t BCS();
uint8_t BEQ();
uint8_t BMI();
uint8_t BNE();
uint8_t BPL();
uint8_t BVC();
uint8_t BVS();
// Status Flag Changes
uint8_t CLC();
uint8_t CLD();
uint8_t CLI();
uint8_t CLV();
uint8_t SEC();
uint8_t SED();
uint8_t SEI();
// System Functions
uint8_t BRK();
uint8_t NOP();
uint8_t RTI();


