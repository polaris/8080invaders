#include "emulator.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static unsigned char cycles[] = {
//   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  a,  b,  c,  d,  e,  f
     4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4, // 0
     4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4, // 1
     4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4, // 2
     4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4, // 3
     5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, // 4
     5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, // 5
     5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, // 6
     7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5, // 7
     4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, // 8
     4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, // 9
     4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, // a
     4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, // b
    11, 10, 10, 10, 17, 11,  7, 11, 11, 10, 10, 10, 10, 17,  7, 11, // c
    11, 10, 10, 10, 17, 11,  7, 11, 11, 10, 10, 10, 10, 17,  7, 11, // d
    11, 10, 10, 18, 17, 11,  7, 11, 11,  5, 10,  5, 17, 17,  7, 11, // e
    11, 10, 10,  4, 17, 11,  7, 11, 11,  5, 10,  4, 17, 17,  7, 11, // f
};

static unsigned char opsize[] = {
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 1
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1, // 2
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // a
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // b
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1, // c
    1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 2, 3, 1, 2, 1, // d
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, // e
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, // f
};

void handle_unimplemented_opcode(void);

void memory_write_direct(cpu_state_t* state, uint16_t address, uint8_t value);
uint8_t memory_read_direct(cpu_state_t* state, uint16_t address);
void memory_write_hl(cpu_state_t* state, uint8_t value);
uint8_t memory_read_hl(cpu_state_t* state);

int parity(int x, int size);

void update_flags(cpu_state_t* state, uint8_t value);
void update_arithmetic_flags(cpu_state_t* state, uint16_t res);
void update_logic_flags(cpu_state_t* state);

void push(cpu_state_t* state, uint8_t high, uint8_t low);
void pop(cpu_state_t* state, uint8_t* high, uint8_t* low);

void call(cpu_state_t* state, uint16_t address);
void ret(cpu_state_t* state);

void cpu_diagnose(void);

unsigned int emulate(cpu_state_t* state) {
    uint8_t* opcode = &state->memory[state->pc];

    state->pc += opsize[*opcode];

    switch(*opcode) {
        case 0x00:                                              // NOP
            break;
        case 0x01:                                              // LXI B, D16
            state->c = opcode[1];
            state->b = opcode[2];
            break;
        case 0x02: {                                            // STAX B
            uint16_t address = (state->b << 8) | (state->c);
            memory_write_direct(state, address, state->a);
            break;
        }
        case 0x03:                                              // INX B
            state->c += 1;
            if (state->c == 0) {
                state->b += 1;
            }
            break;
        case 0x04:                                              // INC B
            state->b += 1;
            update_flags(state, state->b);
            break;
        case 0x05:                                              // DCR B
            state->b -= 1;
            update_flags(state, state->b);
            break;
        case 0x06:                                              // MVI B, D8
            state->b = opcode[1];
            break;
        case 0x07: {                                            // RLC
            uint8_t x = state->a;
            state->a = (uint8_t)(((x & 0x80) >> 7) | (x << 1));
            state->flags.cy = (0x80 == (x & 0x80));
        }
            break;
        case 0x08:
            handle_unimplemented_opcode();
            break;
        case 0x09: {                                            // DAD B
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t bc = (state->b << 8) | state->c;
            uint32_t res = hl + bc;
            state->h = (uint8_t)((res & 0xff00) >> 8);
            state->l = (uint8_t)(res & 0xff);
            state->flags.cy = ((res & 0xffff0000) != 0);
        }
            break;
        case 0x0a: {                                            // LDAX B
            uint16_t address = (state->b << 8) | state->c;
            state->a = memory_read_direct(state, address);
            break;
        }
        case 0x0b:                                              // DCX B
            state->c -= 1;
            if (state->c == 0xff) {
                state->b -= 1;
            }
            break;
        case 0x0c:                                              // INR C
            state->c += 1;
            update_flags(state, state->c);
            break;
        case 0x0d:                                              // DCR C
            state->c -= 1;
            update_flags(state, state->c);
            break;
        case 0x0e:                                              // MVI C, D8
            state->c = opcode[1];
            break;
        case 0x0f: {                                            // RRC
            uint8_t res = state->a;
            state->a = ((res & 1) << 7) | (res >> 1);
            state->flags.cy = (1 == (res & 1));
        }
            break;
        case 0x10:
            handle_unimplemented_opcode();
            break;
        case 0x11:                                              // LXI D, D16
            state->e = opcode[1];
            state->d = opcode[2];
            break;
        case 0x12: {                                            // STAX D
            uint16_t address = (state->d << 8) | state->e;
            memory_write_direct(state, address, state->a);
        }
            break;
        case 0x13:                                              // INX D
            state->e += 1;
            if (state->e == 0) {
                state->d += 1;
            }
            break;
        case 0x14:                                              // INR D
            state->d += 1;
            update_flags(state, state->d);
            break;
        case 0x15:                                              // DCR D
            state->d -= 1;
            update_flags(state, state->d);
            break;
        case 0x16:                                              // MVI D, D8
            state->d = opcode[1];
            break;
        case 0x17: {                                            // RAL
            uint8_t rest = state->a;
            state->a = state->flags.cy | (rest << 1);
            state->flags.cy = (0x80 == (rest & 0x80));
        }
            break;
        case 0x18:
            handle_unimplemented_opcode();
            break;
        case 0x19: {                                            // DAD D
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t de = (state->d << 8) | state->e;
            uint32_t res = hl + de;
            state->h = (uint8_t)((res & 0xff00) >> 8);
            state->l = (uint8_t)(res & 0xff);
            state->flags.cy = ((res & 0xffff0000) != 0);
        }
            break;
        case 0x1a: {                                            // LDAX D
            uint16_t address = (state->d << 8) | state->e;
            state->a = memory_read_direct(state, address);
        }
            break;
        case 0x1b:                                              // DCX D
            state->e -= 1;
            if (state->e == 0xff) {
                state->d -= 1;
            }
            break;
        case 0x1c:                                              // INR E
            state->e += 1;
            update_flags(state, state->e);
            break;
        case 0x1d:                                              // DCR E
            state->e -= 1;
            update_flags(state, state->e);
            break;
        case 0x1e:                                              // MVI E, D8
            state->e = opcode[1];
            break;
        case 0x1f: {                                            // RAR
            uint8_t res = state->a;
            state->a = (state->flags.cy << 7) | (res >> 1);
            state->flags.cy = (1 == (res & 1));
        }
            break;
        case 0x20:
            handle_unimplemented_opcode();
            break;
        case 0x21:                                              // LXI H, D16
            state->l = opcode[1];
            state->h = opcode[2];
            break;
        case 0x22: {                                            // SHLD adr
            uint16_t address = (opcode[2] << 8) | opcode[1];
            memory_write_direct(state, address, state->l);
            memory_write_direct(state, address + 1, state->h);
        }
            break;
        case 0x23:                                              // INX H
            state->l += 1;
            if (state->l == 0) {
                state->h += 1;
            }
            break;
        case 0x24:                                              // INR H
            state->h += 1;
            update_flags(state, state->h);
            break;
        case 0x25:                                              // DCR H
            state->h -= 1;
            update_flags(state, state->h);
            break;
        case 0x26:                                              // MVI H, D8
            state->h = opcode[1];
            break;
        case 0x27:                                              // DAA
             if ((state->a & 0xf) > 9) {
                 state->a += 6;
             }
             if ((state->a & 0xf0) > 0x90) {
                 uint16_t res = (uint16_t)state->a + 0x60;
                 state->a = (uint8_t)(res & 0xff);
                 update_arithmetic_flags(state, res);
             }
             break;
        case 0x28:
            handle_unimplemented_opcode();
            break;
        case 0x29: {                                            // DAD H
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t res = hl + hl;
            state->h = (uint8_t)((res & 0xff00) >> 8);
            state->l = (uint8_t)(res & 0xff);
            state->flags.cy = ((res & 0xffff0000) != 0);
        }
            break;
        case 0x2a: {                                            // LHLD adr
            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->l = memory_read_direct(state, address);
            state->h = memory_read_direct(state, address + 1);
            break;
        }
        case 0x2b:                                              // DCX H
            state->l -= 1;
            if (state->l == 0xff) {
                state->h -= 1;
            }
            break;
        case 0x2c:                                              // INR L
            state->l += 1;
            update_flags(state, state->l);
            break;
        case 0x2d:                                              // DCR L
            state->l -= 1;
            update_flags(state, state->l);
            break;
        case 0x2e:                                              // MVI L, D8
            state->l = opcode[1];
            break;
        case 0x2f:                                              // CMA
            state->a = ~state->a;
            break;
        case 0x30:
            handle_unimplemented_opcode();
            break;
        case 0x31:                                              // LXI SP, D16
            state->sp = (opcode[2] << 8) | opcode[1];
            break;
        case 0x32: {                                            // STA adr
            uint16_t address = (opcode[2] << 8) | opcode[1];
            memory_write_direct(state, address, state->a);
        }
            break;
        case 0x33:                                              // INX SP
            state->sp += 1;
            break;
        case 0x34: {                                            // INR M
            uint8_t res = memory_read_hl(state) + 1;
            update_flags(state, res);
            memory_write_hl(state, res);
        }
            break;
        case 0x35: {                                            // DCR M
            uint8_t res = memory_read_hl(state) - 1;
            update_flags(state, res);
            memory_write_hl(state, res);
        }
            break;
        case 0x36:                                              // MVI M, D8
            memory_write_hl(state, opcode[1]);
            break;
        case 0x37:                                              // STC
            state->flags.cy = 1;
            break;
        case 0x38:
            handle_unimplemented_opcode();
            break;
        case 0x39: {                                            // DAD SP
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t res = hl + state->sp;
            state->h = (uint8_t)((res & 0xff00) >> 8);
            state->l = (uint8_t)(res & 0xff);
            state->flags.cy = ((res & 0xffff0000) > 0);
        }
            break;
        case 0x3a: {                                            // LDA adr
            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->a = memory_read_direct(state, address);
        }
            break;
        case 0x3b:                                              // DCX SP
            state->sp -= 1;
            break;
        case 0x3c:                                              // INR A
            state->a += 1;
            update_flags(state, state->a);
            break;
        case 0x3d:                                              // DCR A
            state->a -= 1;
            update_flags(state, state->a);
            break;
        case 0x3e:                                              // MVI A, D8
            state->a = opcode[1];
            break;
        case 0x3f:                                              // CMC
            state->flags.cy = ~state->flags.cy;
            break;
        case 0x40:                                              // MOV B, B
            state->b = state->b;
            break;
        case 0x41:                                              // MOV B, C
            state->b = state->c;
            break;
        case 0x42:                                              // MOV B, D
            state->b = state->d;
            break;
        case 0x43:                                              // MOV B, E
            state->b = state->e;
            break;
        case 0x44:                                              // MOV B, H
            state->b = state->h;
            break;
        case 0x45:                                              // MOV B, L
            state->b = state->l;
            break;
        case 0x46:                                              // MOV B, M
            state->b = memory_read_hl(state);
            break;
        case 0x47:                                              // MOV B, A
            state->b = state->a;
            break;
        case 0x48:                                              // MOV C, B
            state->c = state->b;
            break;
        case 0x49:                                              // MOV C, C
            state->c = state->c;
            break;
        case 0x4a:                                              // MOV C, D
            state->c = state->d;
            break;
        case 0x4b:                                              // MOV C, E
            state->c = state->e;
            break;
        case 0x4c:                                              // MOV C, H
            state->c = state->h;
            break;
        case 0x4d:                                              // MOV C, L
            state->c = state->l;
            break;
        case 0x4e:                                              // MOV C, M
            state->c = memory_read_hl(state);
            break;
        case 0x4f:                                              // MOV C, A
            state->c = state->a;
            break;
        case 0x50:                                              // MOV D, B
            state->d = state->b;
            break;
        case 0x51:                                              // MOV D, C
            state->d = state->c;
            break;
        case 0x52:                                              // MOV D, D
            state->d = state->d;
            break;
        case 0x53:                                              // MOV D, E
            state->d = state->e;
            break;
        case 0x54:                                              // MOV D, H
            state->d = state->h;
            break;
        case 0x55:                                              // MOV D, L
            state->d = state->l;
            break;
        case 0x56:                                              // MOV D, M
            state->d = memory_read_hl(state);
            break;
        case 0x57:                                              // MOV D, A
            state->d = state->a;
            break;
        case 0x58:                                              // MOV E, B
            state->e = state->b;
            break;
        case 0x59:                                              // MOV E, C
            state->e = state->c;
            break;
        case 0x5a:                                              // MOV E, D
            state->e = state->d;
            break;
        case 0x5b:                                              // MOV E, E
            state->e = state->e;
            break;
        case 0x5c:                                              // MOV E, H
            state->e = state->h;
            break;
        case 0x5d:                                              // MOV E, L
            state->e = state->l;
            break;
        case 0x5e:                                              // MOV E, M
            state->e = memory_read_hl(state);
            break;
        case 0x5f:                                              // MOV E, A
            state->e = state->a;
            break;
        case 0x60:                                              // MOV H, B
            state->h = state->b;
            break;
        case 0x61:                                              // MOV H, C
            state->h = state->c;
            break;
        case 0x62:                                              // MOV H, D
            state->h = state->d;
            break;
        case 0x63:                                              // MOV H, E
            state->h = state->e;
            break;
        case 0x64:                                              // MOV H, H
            state->h = state->h;
            break;
        case 0x65:                                              // MOV H, L
            state->h = state->l;
            break;
        case 0x66:                                              // MOV H, M
            state->h = memory_read_hl(state);
            break;
        case 0x67:                                              // MOV H, A
            state->h = state->a;
            break;
        case 0x68:                                              // MOV L, B
            state->l = state->b;
            break;
        case 0x69:                                              // MOV L, C
            state->l = state->c;
            break;
        case 0x6a:                                              // MOV L, D
            state->l = state->d;
            break;
        case 0x6b:                                              // MOV L, E
            state->l = state->e;
            break;
        case 0x6c:                                              // MOV L, H
            state->l = state->h;
            break;
        case 0x6d:                                              // MOV L, L
            state->l = state->l;
            break;
        case 0x6e:                                              // MOV L, M
            state->l = memory_read_hl(state);
            break;
        case 0x6f:                                              // MOV L, A
            state->l = state->a;
            break;
        case 0x70:                                              // MOV M, B
            memory_write_hl(state, state->b);
            break;
        case 0x71:                                              // MOV M, C
            memory_write_hl(state, state->c);
            break;
        case 0x72:                                              // MOV M, D
            memory_write_hl(state, state->d);
            break;
        case 0x73:                                              // MOV M, E
            memory_write_hl(state, state->e);
            break;
        case 0x74:                                              // MOV M, H
            memory_write_hl(state, state->h);
            break;
        case 0x75:                                              // MOV M, L
            memory_write_hl(state, state->l);
            break;
        case 0x76:                                              // HLT
            break;
        case 0x77:                                              // MOV M, A
            memory_write_hl(state, state->a);
            break;
        case 0x78:                                              // MOV A, B
            state->a = state->b;
            break;
        case 0x79:                                              // MOV A, C
            state->a = state->c;
            break;
        case 0x7a:                                              // MOV A, D
            state->a = state->d;
            break;
        case 0x7b:                                              // MOV A, E
            state->a = state->e;
            break;
        case 0x7c:                                              // MOV A, H
            state->a = state->h;
            break;
        case 0x7d:                                              // MOV A, L
            state->a = state->l;
            break;
        case 0x7e:                                              // MOV A, M
            state->a = memory_read_hl(state);
            break;
        case 0x7f:                                              // MOV A, A
            state->a = state->a;
            break;
        case 0x80: {                                            // ADD B
            uint16_t res = (uint16_t)state->a + (uint16_t)state->b;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x81: {                                            // ADD C
            uint16_t res = (uint16_t)state->a + (uint16_t)state->c;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x82: {                                            // ADD D
            uint16_t res = (uint16_t)state->a + (uint16_t)state->d;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x83: {                                            // ADD E
            uint16_t res = (uint16_t)state->a + (uint16_t)state->e;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x84: {                                            // ADD H
            uint16_t res = (uint16_t)state->a + (uint16_t)state->h;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x85: {                                            // ADD L
            uint16_t res = (uint16_t)state->a + (uint16_t)state->l;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x86: {                                            // ADD M
            uint16_t res = (uint16_t)state->a + (uint16_t)memory_read_hl(state);
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x87: {                                            // ADD A
            uint16_t res = (uint16_t)state->a + (uint16_t)state->a;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x88: {                                            // ADC B
            uint16_t res = (uint16_t)state->a + (uint16_t)state->b + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x89: {                                            // ADC C
            uint16_t res = (uint16_t)state->a + (uint16_t)state->c + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8a: {                                            // ADC D
            uint16_t res = (uint16_t)state->a + (uint16_t)state->d + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8b: {                                            // ADC E
            uint16_t res = (uint16_t)state->a + (uint16_t)state->e + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8c: {                                            // ADC H
            uint16_t res = (uint16_t)state->a + (uint16_t)state->h + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8d: {                                            // ADC L
            uint16_t res = (uint16_t)state->a + (uint16_t)state->l + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8e: {                                            // ADC M
            uint16_t res = (uint16_t)state->a + (uint16_t)memory_read_hl(state) + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x8f: {                                            // ADC A
            uint16_t res = (uint16_t)state->a + (uint16_t)state->a + state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x90: {                                            // SUB B
            uint16_t res = (uint16_t)state->a - (uint16_t)state->b;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x91: {                                            // SUB C
            uint16_t res = (uint16_t)state->a - (uint16_t)state->c;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x92: {                                            // SUB D
            uint16_t res = (uint16_t)state->a - (uint16_t)state->d;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x93: {                                            // SUB E
            uint16_t res = (uint16_t)state->a - (uint16_t)state->e;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x94: {                                            // SUB H
            uint16_t res = (uint16_t)state->a - (uint16_t)state->h;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x95: {                                            // SUB L
            uint16_t res = (uint16_t)state->a - (uint16_t)state->l;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x96: {                                            // SUB M
            uint16_t res = (uint16_t)state->a - (uint16_t)memory_read_hl(state);
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x97: {                                            // SUB A
            uint16_t res = (uint16_t)state->a - (uint16_t)state->a;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x98: {                                            // SBB B
            uint16_t res = (uint16_t)state->a - (uint16_t)state->b - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x99: {                                            // SBB C
            uint16_t res = (uint16_t)state->a - (uint16_t)state->c - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9a: {                                            // SBB D
            uint16_t res = (uint16_t)state->a - (uint16_t)state->d - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9b: {                                            // SBB E
            uint16_t res = (uint16_t)state->a - (uint16_t)state->e - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9c: {                                            // SBB H
            uint16_t res = (uint16_t)state->a - (uint16_t)state->h - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9d: {                                            // SBB L
            uint16_t res = (uint16_t)state->a - (uint16_t)state->l - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9e: {                                            // SBB M
            uint16_t res = (uint16_t)state->a - (uint16_t)memory_read_hl(state) - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0x9f: {                                            // SBB A
            uint16_t res = (uint16_t)state->a - (uint16_t)state->a - state->flags.cy;
            update_arithmetic_flags(state, res);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0xa0:                                              // ANA B
            state->a = state->a & state->b;
            update_logic_flags(state);
            break;
        case 0xa1:                                              // ANA C
            state->a = state->a & state->c;
            update_logic_flags(state);
            break;
        case 0xa2:                                              // ANA D
            state->a = state->a & state->d;
            update_logic_flags(state);
            break;
        case 0xa3:                                              // ANA E
            state->a = state->a & state->e;
            update_logic_flags(state);
            break;
        case 0xa4:                                              // ANA H
            state->a = state->a & state->h;
            update_logic_flags(state);
            break;
        case 0xa5:                                              // ANA L
            state->a = state->a & state->l;
            update_logic_flags(state);
            break;
        case 0xa6:                                              // ANA M
            state->a = state->a & memory_read_hl(state);
            update_logic_flags(state);
            break;
        case 0xa7:                                              // ANA A
            state->a = state->a & state->a;
            update_logic_flags(state);
            break;
        case 0xa8:                                              // XRA B
            state->a = state->a ^ state->b;
            update_logic_flags(state);
            break;
        case 0xa9:                                              // XRA C
            state->a = state->a ^ state->c;
            update_logic_flags(state);
            break;
        case 0xaa:                                              // XRA D
            state->a = state->a ^ state->d;
            update_logic_flags(state);
            break;
        case 0xab:                                              // XRA E
            state->a = state->a ^ state->e;
            update_logic_flags(state);
            break;
        case 0xac:                                              // XRA H
            state->a = state->a ^ state->h;
            update_logic_flags(state);
            break;
        case 0xad:                                              // XRA L
            state->a = state->a ^ state->l;
            update_logic_flags(state);
            break;
        case 0xae:                                              // XRA M
            state->a = state->a ^ memory_read_hl(state);
            update_logic_flags(state);
            break;
        case 0xaf:                                              // XRA A
            state->a = state->a ^ state->a;
            update_logic_flags(state);
            break;
        case 0xb0:                                              // ORA B
            state->a = state->a | state->b;
            update_logic_flags(state);
            break;
        case 0xb1:                                              // ORA C
            state->a = state->a | state->c;
            update_logic_flags(state);
            break;
        case 0xb2:                                              // ORA D
            state->a = state->a | state->d;
            update_logic_flags(state);
            break;
        case 0xb3:                                              // ORA E
            state->a = state->a | state->e;
            update_logic_flags(state);
            break;
        case 0xb4:                                              // ORA H
            state->a = state->a | state->h;
            update_logic_flags(state);
            break;
        case 0xb5:                                              // ORA L
            state->a = state->a | state->l;
            update_logic_flags(state);
            break;
        case 0xb6:                                              // ORA M
            state->a = state->a | memory_read_hl(state);
            update_logic_flags(state);
            break;
        case 0xb7:                                              // ORA A
            state->a = state->a | state->a;
            update_logic_flags(state);
            break;
        case 0xb8: {                                            // CMP B
            uint16_t res = (uint16_t)state->a - (uint16_t)state->b;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xb9: {                                            // CMP C
            uint16_t res = (uint16_t)state->a - (uint16_t)state->c;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xba: {                                            // CMP D
            uint16_t res = (uint16_t)state->a - (uint16_t)state->d;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xbb: {                                            // CMP E
            uint16_t res = (uint16_t)state->a - (uint16_t)state->e;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xbc: {                                            // CMP H
            uint16_t res = (uint16_t)state->a - (uint16_t)state->h;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xbd: {                                            // CMP L
            uint16_t res = (uint16_t)state->a - (uint16_t)state->l;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xbe: {                                            // CMP M
            uint16_t res = (uint16_t)state->a - (uint16_t)memory_read_hl(state);
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xbf: {                                            // CMP A
            uint16_t res = (uint16_t)state->a - (uint16_t)state->a;
            update_arithmetic_flags(state, res);
        }
            break;
        case 0xc0:                                              // RNZ
            if (state->flags.z == 0) {
                ret(state);
            }
            break;
        case 0xc1:                                              // POP B
            pop(state, &state->b, &state->c);
            break;
        case 0xc2:                                              // JNZ adr
            if (state->flags.z == 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xc3:                                              // JMP adr
            state->pc = (opcode[2] << 8) | opcode[1];
            break;
        case 0xc4:                                              // CNZ adr
            if (state->flags.z == 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xc5:                                              // PUSH B
            push(state, state->b, state->c);
            break;
        case 0xc6: {                                            // ADI D8
            uint16_t res = (uint16_t)state->a + (uint16_t)opcode[1];
            update_flags(state, res & 0xff);
            state->flags.cy = (res > 0xff);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0xc7:                                              // RST 0
            call(state, 0x0000);
            break;
        case 0xc8:                                              // RZ
            if (state->flags.z) {
                ret(state);
            }
            break;
        case 0xc9:                                              // RET
            ret(state);
            break;
        case 0xca:                                              // JZ adr
            if (state->flags.z) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xcb:
            handle_unimplemented_opcode();
            break;
        case 0xcc:                                              // CZ adr
            if (state->flags.z) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xcd:                                              // CALL adr
            if (5 ==  ((opcode[2] << 8) | opcode[1])) {
                if (state->c == 9) {
                    uint16_t offset = (state->d<<8) | (state->e);
                    char* str = (char*)(&state->memory[offset+3]);  //skip the prefix bytes
                    while (*str != '$')
                        printf("%c", *str++);
                    printf("\n");
                } else if (state->c == 2) {
                    //saw this in the inspected code, never saw it called
                    printf ("print char routine called\n");
                }
            } else if (0 ==  ((opcode[2] << 8) | opcode[1])) {
                exit(0);
            } else {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xce: {                                            // ACI D8
            uint16_t x = state->a + opcode[1] + state->flags.cy;
            update_flags(state, x & 0xff);
            state->flags.cy = (x > 0xff);
            state->a = x & 0xff;
        }
            break;
        case 0xcf:                                              // RST 1
            call(state, 0x0008);
            break;
        case 0xd0:                                              // RNC
            if (state->flags.cy == 0) {
                ret(state);
            }
            break;
        case 0xd1:                                              // POP D
            pop(state, &state->d, &state->e);
            break;
        case 0xd2:                                              // JNC adr
            if (state->flags.cy == 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xd3:                                              // OUT D8
            state->output(opcode[1], state->a);
            break;
        case 0xd4:                                              // CNC adr
            if (state->flags.cy == 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xd5:                                              // PUSH D
            push(state, state->d, state->e);
            break;
        case 0xd6: {                                            // SUI D8
            uint8_t res = state->a - opcode[1];
            update_flags(state, res & 0xff);
            state->flags.cy = (state->a < opcode[1]);
            state->a = res;
            break;
        }
        case 0xd7:                                              // RST 2
            call(state, 0x0010);
            break;
        case 0xd8:                                              // RC
            if (state->flags.cy != 0) {
                ret(state);
            }
            break;
        case 0xd9:
            handle_unimplemented_opcode();
            break;
        case 0xda:                                              // JS adr
            if (state->flags.cy != 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xdb:                                              // IN D8
            state->a = state->input(opcode[1]);
            break;
        case 0xdc:                                              // CC adr
            if (state->flags.cy != 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xdd:
            handle_unimplemented_opcode();
            break;
        case 0xde: {                                            // SBI D8
            uint16_t res = state->a - opcode[1] - state->flags.cy;
            update_flags(state, res & 0xff);
            state->flags.cy = (res > 0xff);
            state->a = (uint8_t)(res & 0xff);
        }
            break;
        case 0xdf:                                              // RST 3
            call(state, 0x0018);
            break;
        case 0xe0:                                              // RPO
            if (state->flags.p == 0) {
                ret(state);
            }
            break;
        case 0xe1:                                              // POP H
            pop(state, &state->h, &state->l);
            break;
        case 0xe2:                                              // JPO adr
            if (state->flags.p == 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xe3: {                                            // XTHL
            uint8_t l = state->l;
            uint8_t h = state->h;
            state->l = memory_read_direct(state, state->sp);
            state->h = memory_read_direct(state, state->sp + 1);
            memory_write_direct(state, state->sp, l);
            memory_write_direct(state, state->sp + 1, h);
        }
            break;
        case 0xe4:                                              // CPO adr
            if (state->flags.p == 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xe5:                                              // PUSH H
            push(state, state->h, state->l);
            break;
        case 0xe6:                                              // ANI D8
            state->a = state->a & opcode[1];
            update_logic_flags(state);
            break;
        case 0xe7:                                              // RST 4
            call(state, 0x0020);
            break;
        case 0xe8:                                              // RPE
            if (state->flags.p != 0) {
                ret(state);
            }
            break;
        case 0xe9:                                              // PCHL
            state->pc = (state->h << 8) | state->l;
            break;
        case 0xea:                                              // JPE adr
            if (state->flags.p != 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xeb: {                                            // XCHG
            uint8_t d = state->d;
            uint8_t e = state->e;
            state->d = state->h;
            state->e = state->l;
            state->h = d;
            state->l = e;
        }
            break;
        case 0xec:                                              // CPE adr
            if (state->flags.p != 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xed:
            handle_unimplemented_opcode();
            break;
        case 0xee: {                                            // XRI D8
            uint8_t res = state->a ^ opcode[1];
            update_flags(state, res);
            state->flags.cy = 0;
            state->a = res;
        }
            break;
        case 0xef:                                              // RST 5
            call(state, 0x0028);
            break;
        case 0xf0:                                              // RP
            if (state->flags.s == 0) {
                ret(state);
            }
            break;
        case 0xf1:                                              // POP PSW
            pop(state, &state->a, (uint8_t*)&state->flags);
            break;
        case 0xf2:                                              // JP adr
            if (state->flags.s == 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xf3:                                              // DI
            state->int_enable = 0;
            break;
        case 0xf4:                                              // CP
            if (state->flags.s == 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xf5:                                              // PUSH PSW
            push(state, state->a, *(uint8_t*)&state->flags);
            break;
        case 0xf6: {                                            // ORI byte
            uint8_t res = state->a | opcode[1];
            update_flags(state, res);
            state->flags.cy = 0;
            state->a = res;
        }
            break;
        case 0xf7:                                              // RST 6
            call(state, 0x0030);
            break;
        case 0xf8:                                              // RM
            if (state->flags.s != 0) {
                ret(state);
            }
            break;
        case 0xf9:                                              // SPHL
            state->sp = (state->h << 8) | state->l;
            break;
        case 0xfa:                                              // JM
            if (state->flags.s != 0) {
                state->pc = (opcode[2] << 8) | opcode[1];
            }
            break;
        case 0xfb:                                              // EI
            state->int_enable = 1;
            break;
        case 0xfc:                                              // CM
            if (state->flags.s != 0) {
                uint16_t address = (opcode[2] << 8) | opcode[1];
                call(state, address);
            }
            break;
        case 0xfd:
            handle_unimplemented_opcode();
            break;
        case 0xfe: {                                            //CPI  byte
            uint8_t res = state->a - opcode[1];
            update_flags(state, res);
            state->flags.cy = (state->a < opcode[1]);
            break;
        }
        case 0xff:                                              //RST 7
            call(state, 0x0038);
            break;
    }

    return cycles[*opcode];
}

void interrupt(cpu_state_t* state, uint16_t interrupt_num) {
    push(state, (uint8_t)((state->pc & 0xff00) >> 8), (uint8_t)(state->pc & 0xff));
    state->pc = (uint16_t)(8 * interrupt_num);
    state->int_enable = 0;
}

__attribute__((__noreturn__))
void handle_unimplemented_opcode(void) {
    printf ("Error: Unimplemented instruction\n");
    exit(1);
}

void memory_write_direct(cpu_state_t* state, uint16_t address, uint8_t value) {
    state->memory[address] = value;
}

uint8_t memory_read_direct(cpu_state_t* state, uint16_t address) {
    return state->memory[address];
}

void memory_write_hl(cpu_state_t* state, uint8_t value) {
    uint16_t address = (state->h << 8) | state->l;
    memory_write_direct(state, address, value);
}

uint8_t memory_read_hl(cpu_state_t* state) {
    uint16_t address = (state->h << 8) | state->l;
    return memory_read_direct(state, address);
}

int parity(int x, int size) {
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (int i = 0; i < size; i++) {
        if (x & 0x1) {
            p++;
        }
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}

void update_flags(cpu_state_t* state, uint8_t value) {
    state->flags.z = (value == 0);
    state->flags.s = (0x80 == (value & 0x80));
    state->flags.p = parity(value, 8);
}

void update_arithmetic_flags(cpu_state_t* state, uint16_t res) {
    state->flags.cy = (res > 0xff);
    update_flags(state, res & 0xff);
}

void update_logic_flags(cpu_state_t* state) {
    state->flags.cy = state->flags.ac = 0;
    update_flags(state, state->a);
}

void push(cpu_state_t* state, uint8_t high, uint8_t low) {
    memory_write_direct(state, state->sp - 1, high);
    memory_write_direct(state, state->sp - 2, low);
    state->sp -= 2;
}

void pop(cpu_state_t* state, uint8_t* high, uint8_t* low) {
    *low = memory_read_direct(state, state->sp);
    *high = memory_read_direct(state, state->sp + 1);
    state->sp += 2;
}

void call(cpu_state_t* state, uint16_t address) {
    uint16_t ret = state->pc;
    memory_write_direct(state, state->sp - 1, (ret >> 8) & 0xff);
    memory_write_direct(state, state->sp - 2, (ret & 0xff));
    state->sp -= 2;
    state->pc = address;
}

void ret(cpu_state_t* state) {
    state->pc = (state->memory[state->sp + 1] << 8) | state->memory[state->sp];
    state->sp += 2;
}

__attribute__((__noreturn__))
void cpu_diagnose(void) {
    FILE* f = fopen("../cpudiag.bin", "rb");
    if (f == NULL) {
        printf("Error: Couldn't open file\n");
        exit(EXIT_FAILURE);
    }

    cpu_state_t* state = calloc(sizeof(cpu_state_t), 1);
    memset(state, 0, sizeof(cpu_state_t));
    state->memory = malloc(16 * 0x1000);
    memset(state->memory, 0, 16 * 0x1000);

    fseek(f, 0L, SEEK_END);
    int size = (int)ftell(f);
    fseek(f, 0L, SEEK_SET);

    fread(state->memory + 0x100, size, 1, f);
    fclose(f);

    // First instruction is at 0x100
    state->memory[0]=0xc3; // JMP 0x100
    state->memory[1]=0x00;
    state->memory[2]=0x01;

    // Fix the stack pointer from 0x6ad to 0x7ad
    // this 0x06 byte 112 in the code, which is
    // byte 112 + 0x100 = 368 in memory
    state->memory[368] = 0x7;

    // Skip DAA test
    state->memory[0x59c] = 0xc3; // JMP 0x05C2
    state->memory[0x59d] = 0xc2;
    state->memory[0x59e] = 0x05;

    while (1) {
        emulate(state);
    }

    free(state->memory);
    free(state);
}
