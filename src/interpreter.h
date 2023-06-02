#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "display.h"

extern unsigned char key;
extern const size_t PROGRAM_START;
extern const size_t PROGRAM_END;
extern const unsigned char KEY_NULL;

void read_rom_into_memory(char path[]);

void execute_rom(Display display);

#endif
