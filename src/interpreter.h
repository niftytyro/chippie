#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "display.h"

extern unsigned char key;
extern bool keys[16];
extern bool prev_keys[16];
extern const size_t PROGRAM_START;
extern const unsigned char KEY_NULL;

void read_rom_into_memory(char path[]);

void execute_rom(Display display);

#endif
