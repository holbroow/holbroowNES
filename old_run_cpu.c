// Main CPU Execution Loop
// void run_cpu(Cpu* cpu) {
//     int i = 0;

//     printf("CPU: CPU is now running!\n");
//     printf("CPU: Press ENTER to step through instructions.\n");
//     printf("\n");
//     printf("Current CPU State:\n");
//     print_cpu(cpu);

//     while (cpu->running) {
//         printf("\n");
//         printf("CPU Cycles left: %d\n", cpu->cycles_left);
        
//         // Initialize timing
//         uint64_t start_time_ns = get_time_ns();
//         uint64_t last_sync_time_ns = start_time_ns;
        
//         if (cpu->cycles_left == 0) {
//             // Here we can optionally cap the program at a number of instructions 
//             // in order to save time while needing to test with large instruction sequences
//             // without having to step 1 by 1.
//             // Default = Initially disabled with an example value of 50
//             bool run_capped = false;
//             int capped_value = 50;
//             if (run_capped && cpu->A == capped_value) {
//                 exit(0);
//             }
            
//             // Here we allow instruction stepping using the ENTER key
//             int c  = getchar();
//             while (c != '\n' && c != EOF) {
//                 c = getchar();
//             }

//             uint8_t opcode = bus_read(cpu->bus, cpu->PC++);
//             Opcode current_opcode = opcode_table[opcode];
//             uint8_t next_opcode = bus_read(cpu->bus, cpu->PC++);
//             cpu->PC--;  // We increment the PC to read the next opcode, so we need to go back (I hate this...)
            
//             switch (current_opcode.instruction) {
//                 case LDA:
//                     handle_LDA(cpu, opcode);
//                     break;
//                 case LDX:
//                     handle_LDX(cpu, opcode);
//                     break;
//                 case LDY:
//                     handle_LDY(cpu, opcode);
//                     break;
//                 case STA:
//                     handle_STA(cpu, opcode);
//                     break;
//                 case STX:
//                     handle_STX(cpu, opcode);
//                     break;
//                 case STY:
//                     handle_STY(cpu, opcode);
//                     break;
//                 case TAX:
//                     handle_TAX(cpu, opcode);
//                     break;
//                 case TAY:
//                     handle_TAY(cpu, opcode);
//                     break;
//                 case TXA:
//                     handle_TXA(cpu, opcode);
//                     break;
//                 case TYA:
//                     handle_TYA(cpu, opcode);
//                     break;
//                 case TSX:
//                     handle_TSX(cpu, opcode);
//                     break;
//                 case TXS:
//                     handle_TXS(cpu, opcode);
//                     break;
//                 case PHA:
//                     handle_PHA(cpu, opcode);
//                     break;
//                 case PHP:
//                     handle_PHP(cpu, opcode);
//                     break;
//                 case PLA:
//                     handle_PLA(cpu, opcode);
//                     break;
//                 case PLP:
//                     handle_PLP(cpu, opcode);
//                     break;
//                 case AND:
//                     handle_AND(cpu, opcode);
//                     break;
//                 case EOR:
//                     handle_EOR(cpu, opcode);
//                     break;
//                 case ORA:
//                     handle_ORA(cpu, opcode);
//                     break;
//                 case BIT:
//                     handle_BIT(cpu, opcode);
//                     break;
//                 case ADC:
//                     handle_ADC(cpu, opcode);
//                     break;
//                 case SBC:
//                     handle_SBC(cpu, opcode);
//                     break;
//                 case CMP:
//                     handle_CMP(cpu, opcode);
//                     break;
//                 case CPX:
//                     handle_CPX(cpu, opcode);
//                     break;
//                 case CPY:
//                     handle_CPY(cpu, opcode);
//                     break;
//                 case INC:
//                     handle_INC(cpu, opcode);
//                     break;
//                 case INX:
//                     handle_INX(cpu, opcode);
//                     break;
//                 case INY:
//                     handle_INY(cpu, opcode);
//                     break;
//                 case DEC:
//                     handle_DEC(cpu, opcode);
//                     break;
//                 case DEX:
//                     handle_DEX(cpu, opcode);
//                     break;
//                 case DEY:
//                     handle_DEY(cpu, opcode);
//                     break;
//                 case ASL:
//                     handle_ASL(cpu, opcode);
//                     break;
//                 case LSR:
//                     handle_LSR(cpu, opcode);
//                     break;
//                 case ROL:
//                     handle_ROL(cpu, opcode);
//                     break;
//                 case ROR:
//                     handle_ROR(cpu, opcode);
//                     break;
//                 case JMP:
//                     handle_JMP(cpu, opcode);
//                     break;
//                 case JSR:
//                     handle_JSR(cpu, opcode);
//                     break;
//                 case RTS:
//                     handle_RTS(cpu, opcode);
//                     break;
//                 case BCC:
//                     handle_BCC(cpu, opcode);
//                     break;
//                 case BCS:
//                     handle_BCS(cpu, opcode);
//                     break;
//                 case BEQ:
//                     handle_BEQ(cpu, opcode);
//                     break;
//                 case BMI:
//                     handle_BMI(cpu, opcode);
//                     break;
//                 case BNE:
//                     handle_BNE(cpu, opcode);
//                     break;
//                 case BPL:
//                     handle_BPL(cpu, opcode);
//                     break;
//                 case BVC:
//                     handle_BVC(cpu, opcode);
//                     break;
//                 case BVS:
//                     handle_BVS(cpu, opcode);
//                     break;
//                 case CLC:
//                     handle_CLC(cpu, opcode);
//                     break;
//                 case CLD:
//                     handle_CLD(cpu, opcode);
//                     break;
//                 case CLI:
//                     handle_CLI(cpu, opcode);
//                     break;
//                 case CLV:
//                     handle_CLV(cpu, opcode);
//                     break;
//                 case SEC:
//                     handle_SEC(cpu, opcode);
//                     break;
//                 case SED:
//                     handle_SED(cpu, opcode);
//                     break;
//                 case SEI:
//                     handle_SEI(cpu, opcode);
//                     break;
//                 case BRK:
//                     handle_BRK(cpu, opcode);
//                     break;
//                 case NOP:
//                     handle_NOP(cpu, opcode);
//                     break;
//                 case RTI:
//                     handle_RTI(cpu, opcode);
//                     break;
//                 default:
//                     // Handle illegal or undefined opcode(s)
//                     printf("CPU: Error, Encountered illegal or undefined opcode: 0x%02X at PC: 0x%04X\n", opcode, cpu->PC - 1);
//                     cpu->running = false;
//                     break;
//             }

//             printf("CPU: Instruction %d: \n", i+1);
//             printf("Current Opcode: %02x\n", opcode);
//             printf("\n");
//             print_instruction(cpu, current_opcode, next_opcode);
//             printf("\n");
//             print_cpu(cpu);

//             i++;

//             // Future Note to self: Interrupts, etc might need to be handled/implemented.

//         } else {
//             // If there are cpu cycles left, decrease by one and continue the run state
//             // NOTE: When there are 0 cycles left, the next instruction will be called, 
//             // with more cycles to come and so on.
//             cpu->cycles_left--;
//         }
//     }
// }