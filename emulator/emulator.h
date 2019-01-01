#ifndef EMU_EMULATOR_H
#define EMU_EMULATOR_H

#include <stdint.h>

typedef struct cpu_flags {
    uint8_t cy:1;
    uint8_t pad:1;
    uint8_t p:1;
    uint8_t pad2:1;
    uint8_t ac:1;
    uint8_t pad3:1;
    uint8_t z:1;
    uint8_t s:1;
} cpu_flags_t;

typedef struct cpu_state {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memory;
    cpu_flags_t flags;
    uint8_t int_enable;
    uint8_t (*input)(uint8_t);
    void (*output)(uint8_t, uint8_t);
} cpu_state_t;

unsigned int emulate(cpu_state_t* state);
void interrupt(cpu_state_t* state, uint16_t interrupt_num);

#endif //EMU_EMULATOR_H
