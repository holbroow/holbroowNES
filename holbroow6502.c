// Will Holbrook | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)
// holbroow6502.c (Cpu with defined, and handled, addressing modes, instructions, and therefore opcodes!)

// Great resource, this file is mostly inspired from this datasheet.
// https://www.nesdev.org/obelisk-6502-guide

#include "holbroow6502.h"
#include "Bus.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Define the opcode_table
const Opcode opcode_table[256] = {
    // Initialize all 256 opcodes
    // For simplicity, initialize unspecified opcodes as NOP with IMPLIED addressing
    [0 ... 255] = {NOP, IMPLIED, 1, 2},

    // Define specific opcodes as per the 6502 instruction set
    // Load/Store Operations
    [0xA9] = {LDA, IMMEDIATE, 2, 2},
    [0xA5] = {LDA, ZERO_PAGE, 2, 3},
    [0xAD] = {LDA, ABSOLUTE, 3, 4},
    [0xB5] = {LDA, ZERO_PAGE_X, 2, 4},
    [0xBD] = {LDA, ABSOLUTE_X, 3, 4},
    [0xB9] = {LDA, ABSOLUTE_Y, 3, 4},
    [0xA1] = {LDA, INDEXED_INDIRECT, 2, 6},
    [0xB1] = {LDA, INDIRECT_INDEXED, 2, 5},

    [0xA2] = {LDX, IMMEDIATE, 2, 2},
    [0xA6] = {LDX, ZERO_PAGE, 2, 3},
    [0xAE] = {LDX, ABSOLUTE, 3, 4},
    [0xB6] = {LDX, ZERO_PAGE_Y, 2, 4},
    [0xBE] = {LDX, ABSOLUTE_Y, 3, 4},

    [0xA0] = {LDY, IMMEDIATE, 2, 2},
    [0xA4] = {LDY, ZERO_PAGE, 2, 3},
    [0xAC] = {LDY, ABSOLUTE, 3, 4},
    [0xB4] = {LDY, ZERO_PAGE_X, 2, 4},
    [0xBC] = {LDY, ABSOLUTE_X, 3, 4},

    [0x85] = {STA, ZERO_PAGE, 2, 3},
    [0x8D] = {STA, ABSOLUTE, 3, 4},
    [0x95] = {STA, ZERO_PAGE_X, 2, 4},
    [0x9D] = {STA, ABSOLUTE_X, 3, 5},
    [0x99] = {STA, ABSOLUTE_Y, 3, 5},
    [0x81] = {STA, INDEXED_INDIRECT, 2, 6},
    [0x91] = {STA, INDIRECT_INDEXED, 2, 6},

    [0x86] = {STX, ZERO_PAGE, 2, 3},
    [0x8E] = {STX, ABSOLUTE, 3, 4},
    [0x96] = {STX, ZERO_PAGE_Y, 2, 4},

    [0x84] = {STY, ZERO_PAGE, 2, 3},
    [0x8C] = {STY, ABSOLUTE, 3, 4},
    [0x94] = {STY, ZERO_PAGE_X, 2, 4},

    // Register Transfers
    [0xAA] = {TAX, IMPLIED, 1, 2},
    [0xA8] = {TAY, IMPLIED, 1, 2},
    [0x8A] = {TXA, IMPLIED, 1, 2},
    [0x98] = {TYA, IMPLIED, 1, 2},

    // Stack Operations
    [0xBA] = {TSX, IMPLIED, 1, 2},
    [0x9A] = {TXS, IMPLIED, 1, 2},
    [0x48] = {PHA, IMPLIED, 1, 3},
    [0x08] = {PHP, IMPLIED, 1, 3},
    [0x68] = {PLA, IMPLIED, 1, 4},
    [0x28] = {PLP, IMPLIED, 1, 4},

    // Logical Instructions
    [0x29] = {AND, IMMEDIATE, 2, 2},
    [0x25] = {AND, ZERO_PAGE, 2, 3},
    [0x2D] = {AND, ABSOLUTE, 3, 4},
    [0x35] = {AND, ZERO_PAGE_X, 2, 4},
    [0x3D] = {AND, ABSOLUTE_X, 3, 4},
    [0x39] = {AND, ABSOLUTE_Y, 3, 4},
    [0x21] = {AND, INDEXED_INDIRECT, 2, 6},
    [0x31] = {AND, INDIRECT_INDEXED, 2, 5},

    [0x49] = {EOR, IMMEDIATE, 2, 2},
    [0x45] = {EOR, ZERO_PAGE, 2, 3},
    [0x4D] = {EOR, ABSOLUTE, 3, 4},
    [0x55] = {EOR, ZERO_PAGE_X, 2, 4},
    [0x5D] = {EOR, ABSOLUTE_X, 3, 4},
    [0x59] = {EOR, ABSOLUTE_Y, 3, 4},
    [0x41] = {EOR, INDEXED_INDIRECT, 2, 6},
    [0x51] = {EOR, INDIRECT_INDEXED, 2, 5},

    [0x09] = {ORA, IMMEDIATE, 2, 2},
    [0x05] = {ORA, ZERO_PAGE, 2, 3},
    [0x0D] = {ORA, ABSOLUTE, 3, 4},
    [0x15] = {ORA, ZERO_PAGE_X, 2, 4},
    [0x1D] = {ORA, ABSOLUTE_X, 3, 4},
    [0x19] = {ORA, ABSOLUTE_Y, 3, 4},
    [0x01] = {ORA, INDEXED_INDIRECT, 2, 6},
    [0x11] = {ORA, INDIRECT_INDEXED, 2, 5},

    [0x24] = {BIT, ZERO_PAGE, 2, 3},
    [0x2C] = {BIT, ABSOLUTE, 3, 4},

    // Arithmetic Instructions
    [0x69] = {ADC, IMMEDIATE, 2, 2},
    [0x65] = {ADC, ZERO_PAGE, 2, 3},
    [0x6D] = {ADC, ABSOLUTE, 3, 4},
    [0x75] = {ADC, ZERO_PAGE_X, 2, 4},
    [0x7D] = {ADC, ABSOLUTE_X, 3, 4},
    [0x79] = {ADC, ABSOLUTE_Y, 3, 4},
    [0x61] = {ADC, INDEXED_INDIRECT, 2, 6},
    [0x71] = {ADC, INDIRECT_INDEXED, 2, 5},

    [0xE9] = {SBC, IMMEDIATE, 2, 2},
    [0xE5] = {SBC, ZERO_PAGE, 2, 3},
    [0xED] = {SBC, ABSOLUTE, 3, 4},
    [0xF5] = {SBC, ZERO_PAGE_X, 2, 4},
    [0xFD] = {SBC, ABSOLUTE_X, 3, 4},
    [0xF9] = {SBC, ABSOLUTE_Y, 3, 4},
    [0xE1] = {SBC, INDEXED_INDIRECT, 2, 6},
    [0xF1] = {SBC, INDIRECT_INDEXED, 2, 5},

    [0xC9] = {CMP, IMMEDIATE, 2, 2},
    [0xC5] = {CMP, ZERO_PAGE, 2, 3},
    [0xCD] = {CMP, ABSOLUTE, 3, 4},
    [0xD5] = {CMP, ZERO_PAGE_X, 2, 4},
    [0xDD] = {CMP, ABSOLUTE_X, 3, 4},
    [0xD9] = {CMP, ABSOLUTE_Y, 3, 4},
    [0xC1] = {CMP, INDEXED_INDIRECT, 2, 6},
    [0xD1] = {CMP, INDIRECT_INDEXED, 2, 5},

    [0xE0] = {CPX, IMMEDIATE, 2, 2},
    [0xE4] = {CPX, ZERO_PAGE, 2, 3},
    [0xEC] = {CPX, ABSOLUTE, 3, 4},

    [0xC0] = {CPY, IMMEDIATE, 2, 2},
    [0xC4] = {CPY, ZERO_PAGE, 2, 3},
    [0xCC] = {CPY, ABSOLUTE, 3, 4},

    // Increments & Decrements
    [0xE6] = {INC, ZERO_PAGE, 2, 5},
    [0xEE] = {INC, ABSOLUTE, 3, 6},
    [0xF6] = {INC, ZERO_PAGE_X, 2, 6},
    [0xFE] = {INC, ABSOLUTE_X, 3, 7},

    [0xE8] = {INX, IMPLIED, 1, 2},
    [0xC8] = {INY, IMPLIED, 1, 2},

    [0xC6] = {DEC, ZERO_PAGE, 2, 5},
    [0xCE] = {DEC, ABSOLUTE, 3, 6},
    [0xD6] = {DEC, ZERO_PAGE_X, 2, 6},
    [0xDE] = {DEC, ABSOLUTE_X, 3, 7},

    [0xCA] = {DEX, IMPLIED, 1, 2},
    [0x88] = {DEY, IMPLIED, 1, 2},

    // Shifts
    [0x0A] = {ASL, ACCUMULATOR, 1, 2},
    [0x06] = {ASL, ZERO_PAGE, 2, 5},
    [0x0E] = {ASL, ABSOLUTE, 3, 6},
    [0x16] = {ASL, ZERO_PAGE_X, 2, 6},
    [0x1E] = {ASL, ABSOLUTE_X, 3, 7},

    [0x4A] = {LSR, ACCUMULATOR, 1, 2},
    [0x46] = {LSR, ZERO_PAGE, 2, 5},
    [0x4E] = {LSR, ABSOLUTE, 3, 6},
    [0x56] = {LSR, ZERO_PAGE_X, 2, 6},
    [0x5E] = {LSR, ABSOLUTE_X, 3, 7},

    [0x2A] = {ROL, ACCUMULATOR, 1, 2},
    [0x26] = {ROL, ZERO_PAGE, 2, 5},
    [0x2E] = {ROL, ABSOLUTE, 3, 6},
    [0x36] = {ROL, ZERO_PAGE_X, 2, 6},
    [0x3E] = {ROL, ABSOLUTE_X, 3, 7},

    [0x6A] = {ROR, ACCUMULATOR, 1, 2},
    [0x66] = {ROR, ZERO_PAGE, 2, 5},
    [0x6E] = {ROR, ABSOLUTE, 3, 6},
    [0x76] = {ROR, ZERO_PAGE_X, 2, 6},
    [0x7E] = {ROR, ABSOLUTE_X, 3, 7},

    // Jumps & Calls
    [0x4C] = {JMP, ABSOLUTE, 3, 3},
    [0x6C] = {JMP, INDIRECT, 3, 5}, // Note: 6502 JMP indirect bug not handled here
    [0x20] = {JSR, ABSOLUTE, 3, 6},
    [0x60] = {RTS, IMPLIED, 1, 6},

    // Branches
    [0x90] = {BCC, RELATIVE, 2, 2},
    [0xB0] = {BCS, RELATIVE, 2, 2},
    [0xF0] = {BEQ, RELATIVE, 2, 2},
    [0x30] = {BMI, RELATIVE, 2, 2},
    [0xD0] = {BNE, RELATIVE, 2, 2},
    [0x10] = {BPL, RELATIVE, 2, 2},
    [0x50] = {BVC, RELATIVE, 2, 2},
    [0x70] = {BVS, RELATIVE, 2, 2},

    // Status Flag Changes
    [0x18] = {CLC, IMPLIED, 1, 2},
    [0xD8] = {CLD, IMPLIED, 1, 2},
    [0x58] = {CLI, IMPLIED, 1, 2},
    [0xB8] = {CLV, IMPLIED, 1, 2},
    [0x38] = {SEC, IMPLIED, 1, 2},
    [0xF8] = {SED, IMPLIED, 1, 2},
    [0x78] = {SEI, IMPLIED, 1, 2},

    // System Functions
    [0x00] = {BRK, IMPLIED, 1, 7},
    [0xEA] = {NOP, IMPLIED, 1, 2},
    [0x40] = {RTI, IMPLIED, 1, 6},
};

// Define mapped 'Instruction' string names
const char *InstructionStrings[56] = {
    "LDA",
    "LDX",
    "LDY",
    "STA",
    "STX",
    "STY",
    "TAX",
    "TAY",
    "TXA",
    "TYA",
    "TSX",
    "TXS",
    "PHA",
    "PHP",
    "PLA",
    "PLP",
    "AND",
    "EOR",
    "ORA",
    "BIT",
    "ADC",
    "SBC",
    "CMP",
    "CPX",
    "CPY",
    "INC",
    "INX",
    "INY",
    "DEC",
    "DEX",
    "DEY",
    "ASL",
    "LSR",
    "ROL",
    "ROR",
    "JMP",
    "JSR",
    "RTS",
    "BCC",
    "BCS",
    "BEQ",
    "BMI",
    "BNE",
    "BPL",
    "BVC",
    "BVS",
    "CLC",
    "CLD",
    "CLI",
    "CLV",
    "SEC",
    "SED",
    "SEI",
    "BRK",
    "NOP",
    "RTI"
};

// Define mapped 'Addressing Modes' string names
const char *AddressModeStrings[13] = {
    "IMMEDIATE",
    "ZERO_PAGE",
    "ABSOLUTE",
    "ZERO_PAGE_X",
    "ZERO_PAGE_Y",
    "ABSOLUTE_X",
    "ABSOLUTE_Y",
    "INDEXED_INDIRECT",
    "INDIRECT_INDEXED",
    "ACCUMULATOR",
    "IMPLIED",
    "RELATIVE",
    "INDIRECT"
};


// Helper Functions Implementations
uint8_t fetch_operand(Cpu* cpu, AddressingMode mode, uint16_t* address) {
    uint8_t value = 0;
    switch (mode) {
        case IMMEDIATE:
            value = bus_read(cpu->bus, cpu->PC++);
            break;
        case ZERO_PAGE:
            *address = bus_read(cpu->bus, cpu->PC++);
            value = bus_read(cpu->bus, *address);
            break;
        case ZERO_PAGE_X:
            *address = (bus_read(cpu->bus, cpu->PC++) + cpu->X) & 0xFF;
            value = bus_read(cpu->bus, *address);
            break;
        case ZERO_PAGE_Y:
            *address = (bus_read(cpu->bus, cpu->PC++) + cpu->Y) & 0xFF;
            value = bus_read(cpu->bus, *address);
            break;
        case ABSOLUTE:
            *address = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
            value = bus_read(cpu->bus, *address);
            break;
        case ABSOLUTE_X:
            *address = (bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8)) + cpu->X;
            value = bus_read(cpu->bus, *address);
            break;
        case ABSOLUTE_Y:
            *address = (bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8)) + cpu->Y;
            value = bus_read(cpu->bus, *address);
            break;
        case INDEXED_INDIRECT:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                uint16_t addr = (zp + cpu->X) & 0xFF;
                *address = bus_read(cpu->bus, addr) | (bus_read(cpu->bus, (addr + 1) & 0xFF) << 8);
                value = bus_read(cpu->bus, *address);
            }
            break;
        case INDIRECT_INDEXED:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                uint16_t base = bus_read(cpu->bus, zp) | (bus_read(cpu->bus, (zp + 1) & 0xFF) << 8);
                *address = base + cpu->Y;
                value = bus_read(cpu->bus, *address);
            }
            break;
        case RELATIVE:
            {
                int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
                // Note: Branch instructions handle the PC update
                // Here, we return the target address, but actual branching is handled in the handler
                *address = cpu->PC + offset;
            }
            break;
        case ACCUMULATOR:
            // No operand to fetch
            break;
        case IMPLIED:
            // No operand to fetch
            break;
        default:
            // Handle unsupported addressing modes
            break;
    }
    return value;
}

void set_zero_flag(Cpu* cpu, uint8_t value) {
    if (value == 0) {
        cpu->STATUS |= FLAG_ZERO;
    } else {
        cpu->STATUS &= ~FLAG_ZERO;
    }
}

void set_negative_flag(Cpu* cpu, uint8_t value) {
    if (value & 0x80) {
        cpu->STATUS |= FLAG_NEGATIVE;
    } else {
        cpu->STATUS &= ~FLAG_NEGATIVE;
    }
}

void set_carry_flag(Cpu* cpu, bool set) {
    if (set) {
        cpu->STATUS |= FLAG_CARRY;
    } else {
        cpu->STATUS &= ~FLAG_CARRY;
    }
}

void set_overflow_flag(Cpu* cpu, bool set) {
    if (set) {
        cpu->STATUS |= FLAG_OVERFLOW;
    } else {
        cpu->STATUS &= ~FLAG_OVERFLOW;
    }
}

void set_interrupt_flag(Cpu* cpu, bool set) {
    if (set) {
        cpu->STATUS |= FLAG_INTERRUPT;
    } else {
        cpu->STATUS &= ~FLAG_INTERRUPT;
    }
}

// Stack Operations
void push_stack(Cpu* cpu, uint8_t value) {
    bus_write(cpu->bus, 0x0100 + cpu->SP, value);
    cpu->SP--;
}

uint8_t pull_stack(Cpu* cpu) {
    cpu->SP++;
    return bus_read(cpu->bus, 0x0100 + cpu->SP);
}

// Initialize interrupt vectors
void initialize_interrupt_vectors(Bus* bus) {
    // NMI Vector: 0xFFFA and 0xFFFB
    bus_write(bus, 0xFFFA, 0x00); // Low byte of NMI handler
    bus_write(bus, 0xFFFB, 0x80); // High byte of NMI handler (e.g., 0x8000)

    // IRQ Vector: 0xFFFE and 0xFFFF
    bus_write(bus, 0xFFFE, 0x00); // Low byte of IRQ handler
    bus_write(bus, 0xFFFF, 0x80); // High byte of IRQ handler (e.g., 0x8000)
}

// CPU Initialization and helper functions
Cpu* init_cpu(Bus* bus) {
    Cpu* cpu = (Cpu*)malloc(sizeof(Cpu));
    if (!cpu) {
        fprintf(stderr, "CPU: Error, Failed to allocate memory for the CPU.\n");
        exit(1);
    }
    printf("CPU: CPU allocated!\n");

    // Initialize CPU registers
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFD; // Typical reset value
    cpu->PC = bus_read(bus, 0xFFFC) | (bus_read(bus, 0xFFFD) << 8); // Reset vector
    cpu->STATUS = FLAG_UNUSED;
    cpu->bus = bus;
    cpu->running = true;
    cpu->cycles_left = 0;

    // Initialize interrupt vectors
    initialize_interrupt_vectors(bus);

    printf("CPU: CPU Initialised!\n");
    return cpu;
}

void print_cpu(Cpu* cpu) {
    printf("|  A:%02x |  X:%02x |  Y:%02x |  SP:%04x |  PC:%04x |\n", cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->PC);
    printf("| C:%01x | Z:%01x | I:%01x | D:%01x | B:%01x | - | O:%01x | N:%01x |\n", (cpu->STATUS >> 0) & 1,
                                                                                     (cpu->STATUS >> 1) & 1,
                                                                                     (cpu->STATUS >> 2) & 1,
                                                                                     (cpu->STATUS >> 3) & 1,
                                                                                     (cpu->STATUS >> 4) & 1,
                                                                                     (cpu->STATUS >> 6) & 1,
                                                                                     (cpu->STATUS >> 7) & 1);
    printf("\n");
}

void print_instruction(Cpu* cpu, Opcode c, uint8_t n) {
    // PC ADDR | INSTRUCTION | VALUE? (value of PC + 1) | ADDRESSING MODE
    if (strcmp(InstructionStrings[c.instruction], "BCC") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BCS") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BEQ") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BMI") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BNE") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BPL") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BVC") == 0 || 
        strcmp(InstructionStrings[c.instruction], "BVS") == 0 || 
        strcmp(InstructionStrings[c.instruction], "JMP") == 0 || 
        strcmp(InstructionStrings[c.instruction], "JSR") == 0) {
        printf("$%04x:  %s  #$%04x  {%s}\n", cpu->PC, 
                                            InstructionStrings[c.instruction], 
                                            n,
                                            AddressModeStrings[c.addressing_mode]);
    } else {
        printf("$%04x:  %s  #$%02x  {%s}\n", cpu->PC, 
                                            InstructionStrings[c.instruction], 
                                            n,
                                            AddressModeStrings[c.addressing_mode]);
    }
}

// Main CPU Execution Loop
void run_cpu(Cpu* cpu) {
    int i = 0;

    printf("CPU: CPU is now running!\n");
    printf("CPU: Press ENTER to step through instructions.\n");
    printf("\n");
    printf("Current CPU State:\n");
    print_cpu(cpu);

    while (cpu->running) {
        printf("\n");
        printf("CPU Cycles left: %d\n", cpu->cycles_left);

        if (cpu->cycles_left == 0) {
            // Here we can optionally cap the program at a number of instructions 
            // in order to save time while needing to test with large instruction sequences
            // without having to step 1 by 1.
            // Default = Initially disabled with an example value of 50
            bool run_capped = false;
            int capped_value = 50;
            if (run_capped && cpu->A == capped_value) {
                exit(0);
            }
            
            // Here we allow instruction stepping using the ENTER key
            int c  = getchar();
            while (c != '\n' && c != EOF) {
                c = getchar();
            }

            uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
            Opcode current_opcode = opcode_table[opcode];
            uint8_t next_opcode = bus_read(cpu->bus, cpu->PC++);
            cpu->PC--;  // We increment the PC to read the next opcode, so we need to go back (I hate this...)

            switch (current_opcode.instruction) {
                case LDA:
                    handle_LDA(cpu, opcode);
                    break;
                case LDX:
                    handle_LDX(cpu, opcode);
                    break;
                case LDY:
                    handle_LDY(cpu, opcode);
                    break;
                case STA:
                    handle_STA(cpu, opcode);
                    break;
                case STX:
                    handle_STX(cpu, opcode);
                    break;
                case STY:
                    handle_STY(cpu, opcode);
                    break;
                case TAX:
                    handle_TAX(cpu, opcode);
                    break;
                case TAY:
                    handle_TAY(cpu, opcode);
                    break;
                case TXA:
                    handle_TXA(cpu, opcode);
                    break;
                case TYA:
                    handle_TYA(cpu, opcode);
                    break;
                case TSX:
                    handle_TSX(cpu, opcode);
                    break;
                case TXS:
                    handle_TXS(cpu, opcode);
                    break;
                case PHA:
                    handle_PHA(cpu, opcode);
                    break;
                case PHP:
                    handle_PHP(cpu, opcode);
                    break;
                case PLA:
                    handle_PLA(cpu, opcode);
                    break;
                case PLP:
                    handle_PLP(cpu, opcode);
                    break;
                case AND:
                    handle_AND(cpu, opcode);
                    break;
                case EOR:
                    handle_EOR(cpu, opcode);
                    break;
                case ORA:
                    handle_ORA(cpu, opcode);
                    break;
                case BIT:
                    handle_BIT(cpu, opcode);
                    break;
                case ADC:
                    handle_ADC(cpu, opcode);
                    break;
                case SBC:
                    handle_SBC(cpu, opcode);
                    break;
                case CMP:
                    handle_CMP(cpu, opcode);
                    break;
                case CPX:
                    handle_CPX(cpu, opcode);
                    break;
                case CPY:
                    handle_CPY(cpu, opcode);
                    break;
                case INC:
                    handle_INC(cpu, opcode);
                    break;
                case INX:
                    handle_INX(cpu, opcode);
                    break;
                case INY:
                    handle_INY(cpu, opcode);
                    break;
                case DEC:
                    handle_DEC(cpu, opcode);
                    break;
                case DEX:
                    handle_DEX(cpu, opcode);
                    break;
                case DEY:
                    handle_DEY(cpu, opcode);
                    break;
                case ASL:
                    handle_ASL(cpu, opcode);
                    break;
                case LSR:
                    handle_LSR(cpu, opcode);
                    break;
                case ROL:
                    handle_ROL(cpu, opcode);
                    break;
                case ROR:
                    handle_ROR(cpu, opcode);
                    break;
                case JMP:
                    handle_JMP(cpu, opcode);
                    break;
                case JSR:
                    handle_JSR(cpu, opcode);
                    break;
                case RTS:
                    handle_RTS(cpu, opcode);
                    break;
                case BCC:
                    handle_BCC(cpu, opcode);
                    break;
                case BCS:
                    handle_BCS(cpu, opcode);
                    break;
                case BEQ:
                    handle_BEQ(cpu, opcode);
                    break;
                case BMI:
                    handle_BMI(cpu, opcode);
                    break;
                case BNE:
                    handle_BNE(cpu, opcode);
                    break;
                case BPL:
                    handle_BPL(cpu, opcode);
                    break;
                case BVC:
                    handle_BVC(cpu, opcode);
                    break;
                case BVS:
                    handle_BVS(cpu, opcode);
                    break;
                case CLC:
                    handle_CLC(cpu, opcode);
                    break;
                case CLD:
                    handle_CLD(cpu, opcode);
                    break;
                case CLI:
                    handle_CLI(cpu, opcode);
                    break;
                case CLV:
                    handle_CLV(cpu, opcode);
                    break;
                case SEC:
                    handle_SEC(cpu, opcode);
                    break;
                case SED:
                    handle_SED(cpu, opcode);
                    break;
                case SEI:
                    handle_SEI(cpu, opcode);
                    break;
                case BRK:
                    handle_BRK(cpu, opcode);
                    break;
                case NOP:
                    handle_NOP(cpu, opcode);
                    break;
                case RTI:
                    handle_RTI(cpu, opcode);
                    break;
                default:
                    // Handle illegal or undefined opcode(s)
                    printf("CPU: Error, Encountered illegal or undefined opcode: 0x%02X at PC: 0x%04X\n", opcode, cpu->PC - 1);
                    cpu->running = false;
                    break;
            }

            printf("CPU: Instruction %d: \n", i+1);
            printf("Current Opcode: %02x\n", opcode);
            printf("\n");
            print_instruction(cpu, current_opcode, next_opcode);
            printf("\n");
            print_cpu(cpu);

            i++;

            // Future Note to self: Interrupts, etc might need to be handled/implemented.

        } else {
            // If there are cpu cycles left, decrease by one and continue the run state
            // NOTE: When there are 0 cycles left, the next instruction will be called, 
            // with more cycles to come and so on.
            cpu->cycles_left--;
        }
    }
}

static inline bool page_crossed(uint16_t old_addr, uint16_t new_addr) {
    return ((old_addr & 0xFF00) != (new_addr & 0xFF00));
}


void handle_LDA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles; 
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->A = value;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    // If ABSOLUTE_X or ABSOLUTE_Y, check page crossing
    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) {
            cpu->cycles_left += 1;
        }
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) {
            cpu->cycles_left += 1;
        }
    }
}

void handle_LDX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->X = value;
    set_zero_flag(cpu, cpu->X);
    set_negative_flag(cpu, cpu->X);

    // LDX uses ABSOLUTE_Y for page crossing check
    if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) {
            cpu->cycles_left += 1;
        }
    }
}

void handle_LDY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->Y = value;
    set_zero_flag(cpu, cpu->Y);
    set_negative_flag(cpu, cpu->Y);

    // LDY uses ABSOLUTE_X for page crossing check
    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) {
            cpu->cycles_left += 1;
        }
    }
}

void handle_STA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles; 
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    // Address calculation
    switch(mode) {
        case ZERO_PAGE:
            address = bus_read(cpu->bus, cpu->PC++);
            break;
        case ZERO_PAGE_X:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                address = (zp + cpu->X) & 0xFF;
            }
            break;
        case ZERO_PAGE_Y:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                address = (zp + cpu->Y) & 0xFF;
            }
            break;
        case ABSOLUTE:
            address = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
            break;
        case ABSOLUTE_X:
            {
                uint16_t base = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
                address = base + cpu->X;
                // STA does NOT add a cycle for crossing in standard 6502 behavior
            }
            break;
        case ABSOLUTE_Y:
            {
                uint16_t base = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
                address = base + cpu->Y;
                // STA does NOT add a cycle for crossing
            }
            break;
        case INDEXED_INDIRECT:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                uint16_t ptr = (zp + cpu->X) & 0xFF;
                address = bus_read(cpu->bus, ptr) | (bus_read(cpu->bus, (ptr + 1) & 0xFF) << 8);
            }
            break;
        case INDIRECT_INDEXED:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                uint16_t base = bus_read(cpu->bus, zp) | (bus_read(cpu->bus, (zp + 1) & 0xFF) << 8);
                address = base + cpu->Y;
                // STA (INDIRECT_INDEXED) does not add cycle for crossing
            }
            break;
        default:
            printf("STA encountered with unsupported addressing mode: %d\n", mode);
            cpu->running = false;
            return;
    }

    bus_write(cpu->bus, address, cpu->A);
}

void handle_STX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles; 
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    switch(mode) {
        case ZERO_PAGE:
            address = bus_read(cpu->bus, cpu->PC++);
            break;
        case ZERO_PAGE_Y:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                address = (zp + cpu->Y) & 0xFF;
            }
            break;
        case ABSOLUTE:
            address = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
            break;
        default:
            printf("STX encountered with unsupported addressing mode: %d\n", mode);
            cpu->running = false;
            return;
    }

    bus_write(cpu->bus, address, cpu->X);
}

void handle_STY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles; 
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    switch(mode) {
        case ZERO_PAGE:
            address = bus_read(cpu->bus, cpu->PC++);
            break;
        case ZERO_PAGE_X:
            {
                uint8_t zp = bus_read(cpu->bus, cpu->PC++);
                address = (zp + cpu->X) & 0xFF;
            }
            break;
        case ABSOLUTE:
            address = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
            break;
        default:
            printf("STY encountered with unsupported addressing mode: %d\n", mode);
            cpu->running = false;
            return;
    }

    bus_write(cpu->bus, address, cpu->Y);
}

void handle_TAX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->X = cpu->A;
    set_zero_flag(cpu, cpu->X);
    set_negative_flag(cpu, cpu->X);
}

void handle_TAY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->Y = cpu->A;
    set_zero_flag(cpu, cpu->Y);
    set_negative_flag(cpu, cpu->Y);
}

void handle_TXA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->A = cpu->X;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);
}

void handle_TYA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->A = cpu->Y;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);
}

void handle_TSX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->X = cpu->SP;
    set_zero_flag(cpu, cpu->X);
    set_negative_flag(cpu, cpu->X);
}

void handle_TXS(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->SP = cpu->X;
}

void handle_PHA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    push_stack(cpu, cpu->A);
}

void handle_PHP(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint8_t status = cpu->STATUS | FLAG_BREAK | FLAG_UNUSED;
    push_stack(cpu, status);
}

void handle_PLA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->A = pull_stack(cpu);
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);
}

void handle_PLP(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->STATUS = pull_stack(cpu);
    cpu->STATUS |= FLAG_UNUSED;
}

void handle_AND(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->A &= value;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    // Check page crossing if ABSOLUTE_X or ABSOLUTE_Y
    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_EOR(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->A ^= value;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_ORA(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint8_t value = fetch_operand(cpu, mode, &address);
    cpu->A |= value;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_BIT(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, opcode_table[opcode].addressing_mode, &address);
    uint8_t result = cpu->A & value;
    set_zero_flag(cpu, result);
    if (value & 0x40) {
        cpu->STATUS |= FLAG_OVERFLOW;
    } else {
        cpu->STATUS &= ~FLAG_OVERFLOW;
    }
    set_negative_flag(cpu, value);
    // BIT does not have indexed variants that cause extra cycles.
}

void handle_ADC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, mode, &address);
    uint16_t sum = cpu->A + value + ((cpu->STATUS & FLAG_CARRY) ? 1 : 0);

    set_carry_flag(cpu, sum > 0xFF);
    set_overflow_flag(cpu, ((cpu->A ^ sum) & (value ^ sum) & 0x80) != 0);

    cpu->A = sum & 0xFF;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_SBC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, mode, &address);
    uint8_t carry = (cpu->STATUS & FLAG_CARRY) ? 1 : 0;
    uint16_t diff = cpu->A - value - (1 - carry);

    set_carry_flag(cpu, cpu->A >= (value + (1 - carry)));
    set_overflow_flag(cpu, ((cpu->A ^ diff) & (value ^ diff) & 0x80) != 0);

    cpu->A = diff & 0xFF;
    set_zero_flag(cpu, cpu->A);
    set_negative_flag(cpu, cpu->A);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_CMP(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, mode, &address);
    uint16_t result = cpu->A - value;
    set_carry_flag(cpu, cpu->A >= value);
    set_zero_flag(cpu, result & 0xFF);
    set_negative_flag(cpu, result & 0xFF);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    } else if (mode == ABSOLUTE_Y) {
        uint16_t base = address - cpu->Y;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_CPX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    // CPX does not have ABSOLUTE_X or ABSOLUTE_Y modes
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, opcode_table[opcode].addressing_mode, &address);
    uint16_t result = cpu->X - value;
    set_carry_flag(cpu, cpu->X >= value);
    set_zero_flag(cpu, result & 0xFF);
    set_negative_flag(cpu, result & 0xFF);
}

void handle_CPY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    // CPY does not have ABSOLUTE_X or ABSOLUTE_Y modes
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, opcode_table[opcode].addressing_mode, &address);
    uint16_t result = cpu->Y - value;
    set_carry_flag(cpu, cpu->Y >= value);
    set_zero_flag(cpu, result & 0xFF);
    set_negative_flag(cpu, result & 0xFF);
}

void handle_INC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, mode, &address);
    value++;
    bus_write(cpu->bus, address, value);
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_INX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->X++;
    set_zero_flag(cpu, cpu->X);
    set_negative_flag(cpu, cpu->X);
}

void handle_INY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->Y++;
    set_zero_flag(cpu, cpu->Y);
    set_negative_flag(cpu, cpu->Y);
}

void handle_DEC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    uint16_t address = 0;
    uint8_t value = fetch_operand(cpu, mode, &address);
    value--;
    bus_write(cpu->bus, address, value);
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (mode == ABSOLUTE_X) {
        uint16_t base = address - cpu->X;
        if (page_crossed(base, address)) cpu->cycles_left += 1;
    }
}

void handle_DEX(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->X--;
    set_zero_flag(cpu, cpu->X);
    set_negative_flag(cpu, cpu->X);
}

void handle_DEY(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->Y--;
    set_zero_flag(cpu, cpu->Y);
    set_negative_flag(cpu, cpu->Y);
}

void handle_ASL(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    uint8_t value;
    bool is_accumulator = false;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    if (mode == ACCUMULATOR) {
        value = cpu->A;
        is_accumulator = true;
    } else {
        value = fetch_operand(cpu, mode, &address);
    }

    set_carry_flag(cpu, (value & 0x80) != 0);

    value <<= 1;
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (is_accumulator) {
        cpu->A = value;
    } else {
        bus_write(cpu->bus, address, value);
    }
    // ASL does not add extra cycles for page crossing
}

void handle_LSR(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    uint8_t value;
    bool is_accumulator = false;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    if (mode == ACCUMULATOR) {
        value = cpu->A;
        is_accumulator = true;
    } else {
        value = fetch_operand(cpu, mode, &address);
    }

    set_carry_flag(cpu, (value & 0x01) != 0);

    value >>= 1;
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (is_accumulator) {
        cpu->A = value;
    } else {
        bus_write(cpu->bus, address, value);
    }
    // LSR does not add extra cycles for page crossing
}

void handle_ROL(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    uint8_t value;
    bool is_accumulator = false;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    if (mode == ACCUMULATOR) {
        value = cpu->A;
        is_accumulator = true;
    } else {
        value = fetch_operand(cpu, mode, &address);
    }

    uint8_t carry_in = (cpu->STATUS & FLAG_CARRY) ? 1 : 0;
    set_carry_flag(cpu, (value & 0x80) != 0);

    value = (value << 1) | carry_in;
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (is_accumulator) {
        cpu->A = value;
    } else {
        bus_write(cpu->bus, address, value);
    }
    // ROL does not add extra cycles for page crossing
}

void handle_ROR(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t address = 0;
    uint8_t value;
    bool is_accumulator = false;
    AddressingMode mode = opcode_table[opcode].addressing_mode;

    if (mode == ACCUMULATOR) {
        value = cpu->A;
        is_accumulator = true;
    } else {
        value = fetch_operand(cpu, mode, &address);
    }

    uint8_t carry_in = (cpu->STATUS & FLAG_CARRY) ? 0x80 : 0;
    set_carry_flag(cpu, (value & 0x01) != 0);

    value = (value >> 1) | carry_in;
    set_zero_flag(cpu, value);
    set_negative_flag(cpu, value);

    if (is_accumulator) {
        cpu->A = value;
    } else {
        bus_write(cpu->bus, address, value);
    }
    // ROR does not add extra cycles for page crossing
}

void handle_JMP(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    AddressingMode mode = opcode_table[opcode].addressing_mode;
    if (mode == ABSOLUTE) {
        uint16_t addr = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
        cpu->PC = addr;
    } else if (mode == INDIRECT) {
        uint16_t ptr = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
        uint16_t addr;
        if ((ptr & 0x00FF) == 0x00FF) {
            addr = bus_read(cpu->bus, ptr) | (bus_read(cpu->bus, ptr & 0xFF00) << 8);
        } else {
            addr = bus_read(cpu->bus, ptr) | (bus_read(cpu->bus, ptr + 1) << 8);
        }
        cpu->PC = addr;
    } else {
        printf("JMP encountered with unsupported addressing mode: %d\n", mode);
        cpu->running = false;
    }
    // JMP does not have variable cycle additions based on conditions
}

void handle_JSR(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint16_t addr = bus_read(cpu->bus, cpu->PC++) | (bus_read(cpu->bus, cpu->PC++) << 8);
    uint16_t return_addr = cpu->PC - 1;
    push_stack(cpu, (return_addr >> 8) & 0xFF);
    push_stack(cpu, return_addr & 0xFF);
    cpu->PC = addr;
    // JSR no extra conditional cycles
}

void handle_RTS(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    uint8_t low = pull_stack(cpu);
    uint8_t high = pull_stack(cpu);
    uint16_t return_addr = (high << 8) | low;
    cpu->PC = return_addr + 1;
    // RTS no extra conditional cycles
}

// Branches (add cycles if branch taken and possibly if page crossed)
void handle_BCC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (!(cpu->STATUS & FLAG_CARRY)) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1; // branch taken
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BCS(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (cpu->STATUS & FLAG_CARRY) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BEQ(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (cpu->STATUS & FLAG_ZERO) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BMI(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (cpu->STATUS & FLAG_NEGATIVE) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BNE(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (!(cpu->STATUS & FLAG_ZERO)) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BPL(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (!(cpu->STATUS & FLAG_NEGATIVE)) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BVC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (!(cpu->STATUS & FLAG_OVERFLOW)) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_BVS(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    int8_t offset = (int8_t)bus_read(cpu->bus, cpu->PC++);
    if (cpu->STATUS & FLAG_OVERFLOW) {
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;
        cpu->cycles_left += 1;
        if (page_crossed(old_pc, cpu->PC)) cpu->cycles_left += 1;
    }
}

void handle_CLC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    set_carry_flag(cpu, false);
}

void handle_CLD(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->STATUS &= ~FLAG_DECIMAL;
}

void handle_CLI(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->STATUS &= ~FLAG_INTERRUPT;
}

void handle_CLV(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    set_overflow_flag(cpu, false);
}

void handle_SEC(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    set_carry_flag(cpu, true);
}

void handle_SED(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->STATUS |= FLAG_DECIMAL;
}

void handle_SEI(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    set_interrupt_flag(cpu, true);
}

void handle_BRK(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->PC++; 
    push_stack(cpu, (cpu->PC >> 8) & 0xFF);
    push_stack(cpu, cpu->PC & 0xFF);
    uint8_t status = cpu->STATUS | FLAG_BREAK;
    push_stack(cpu, status);
    set_interrupt_flag(cpu, true);
    uint16_t irq_low = bus_read(cpu->bus, 0xFFFE);
    uint16_t irq_high = bus_read(cpu->bus, 0xFFFF) << 8;
    cpu->PC = irq_low | irq_high;
}

void handle_NOP(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    // NOP does nothing else
}

void handle_RTI(Cpu* cpu, uint8_t opcode) {
    cpu->cycles_left += opcode_table[opcode].cycles;
    cpu->STATUS = pull_stack(cpu);
    cpu->STATUS |= FLAG_UNUSED;
    uint8_t low = pull_stack(cpu);
    uint8_t high = pull_stack(cpu);
    cpu->PC = (high << 8) | low;
}
