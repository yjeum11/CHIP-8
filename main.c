#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "isa.h"

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: c8 <rom>");
        exit(1);
    }
    // general-purpose registers
    uint8_t V[16];
    // index register
    uint16_t I = 0;
    // delay timer, sound timer. should automatically decrement at 60Hz
    uint8_t DT = 0;
    uint8_t ST = 0;

    uint8_t memory[MEM_SIZE];
    uint16_t stack[16];
    uint8_t sp = 0;
    uint16_t pc = RESET_PC;

    memset(V, 0, 16);
    memset(memory, 0, 4096);
    memset(stack, 0, 16);

    // Load program
    FILE *prog_f = fopen(argv[1], "rb");
    fread(memory+0x200, 1, 4096, prog_f);

    for (;;) {
        uint16_t inst = memory[pc] << 8 | memory[pc+1];
        switch (inst) {
            case CLS:
                // clear_screen();
                break;
            case RET:
                pc = stack[sp];
                sp--;
                break;
        }
        switch (inst & 0xF000) {
            uint8_t nnn = inst & 0x0FFF;
            uint8_t x = inst & 0x0F00;
            uint8_t y = inst & 0x00F0;
            uint8_t kk = inst & 0x00FF;
            case 1: // Jump to nnn
                pc = nnn;
                break;
            case 2: // Call subroutine nnn
                stack[++sp] = pc;
                pc = nnn;
                break;
            case 3: // Skip if x == kk
                if (V[x] == kk)
                    pc += 2;
                break;
            case 4: // Skip if x != kk
                if (V[x] != kk)
                    pc += 2;
                break;
            case 5:
                if (V[x] == V[y])
                    pc += 2;
                break;
            case 6:
                V[x] = kk;
                break;
            case 7:
                V[x] += kk;
                break;
            case 8:
                switch (inst & 0xF) {
                    case 0:
                        V[x] = V[y];
                        break;
                    case 1:
                        V[x] |= V[y];
                        break;
                    case 2:
                        V[x] &= V[y];
                        break;
                    case 3:
                        V[x] ^= V[y];
                        break;
                    case 4:
                        ;
                        uint8_t carry = ((uint16_t)V[x] + (uint16_t)V[y]) & 0x100;
                        V[x] += V[y];
                        V[0xF] = carry;
                        break;
                    case 5:
                        V[0xF] = V[x] > V[y];
                        V[x] = V[x] - V[y];
                        break;
                    case 6:
                        V[0xF] = V[x] & (0x01);
                        V[x] >>= 1;
                        break;
                    case 7:
                        V[0xF] = V[x] < V[y];
                        V[x] = V[y] - V[x];
                        break;
                    case 0xE:
                        V[0xF] = !!(V[x] & 0x80); // trick to detect one in MSB
                        V[x] <<= 1;
                        break;

                }
                break;
        }
        pc += 2;
    }

}
