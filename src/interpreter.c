#include "interpreter.h"
#include "audio.h"
#include "display.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const unsigned char KEY_NULL = 0xFF;

unsigned char key = KEY_NULL;
bool wait_for_key = false;

// in this implementation, programs start at 0x50
// (till that point, memory is taken up by pre-loaded fonts)
// and end at 0xF00 (remaining space is taken by the display)
const size_t PROGRAM_START = 512;
const size_t PROGRAM_END = 4096 - 352;

uint16_t PC = PROGRAM_START;

unsigned char delay_timer = 0;
unsigned char sound_timer = 0;

unsigned char registers[16];
uint16_t I = 0;
uint16_t stack[16];
unsigned short stack_counter = 0;
unsigned char memory[4096] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

void read_rom_into_memory(char path[]) {
  FILE *fptr;

  fptr = fopen(path, "rb");

  fread(&memory[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, fptr);

  fclose(fptr);
}

enum INSTRUCTION_EXECUTION_RESULT {
  SUCCESS,
  STACK_OVERFLOW,
  STACK_UNDERFLOW,
  SUCCESS_UPDATED_PC
};

void handle_register_instruction(unsigned char P, unsigned char X,
                                 unsigned char Y, unsigned char N) {
  switch (N) {
  case 0x0:
    registers[X] = registers[Y];
    break;
  case 0x1:
    registers[X] = registers[X] | registers[Y];
    registers[15] = 0;
    break;
  case 0x2:
    registers[X] = registers[X] & registers[Y];
    registers[15] = 0;
    break;
  case 0x3:
    registers[X] = registers[X] ^ registers[Y];
    registers[15] = 0;
    break;
  case 0x4: {
    unsigned char flag = (registers[X] + registers[Y]) > 0xFF;
    registers[X] += registers[Y];
    registers[15] = flag;
    break;
  }
  case 0x5: {
    unsigned char flag = registers[X] >= registers[Y];
    registers[X] -= registers[Y];
    registers[15] = flag;
    break;
  }
  case 0x6: {
    unsigned char flag = registers[Y] & 1;
    registers[X] = registers[Y] >> 1;
    registers[15] = flag;
    break;
  }
  case 0x7: {
    unsigned char flag = registers[Y] >= registers[X];
    registers[X] = registers[Y] - registers[X];
    registers[15] = flag;
    break;
  }
  case 0xE: {
    unsigned char flag = registers[Y] >> 7;
    registers[X] = registers[Y] << 1;
    registers[15] = flag;
    break;
  }
  default:
    break;
  }
}

void update_display_byte(int address, unsigned char sprite) {
  unsigned char previous_pixels = memory[address];
  memory[address] = memory[address] ^ sprite;
  if ((memory[address] & previous_pixels) != memory[address]) {
    registers[15] = 1;
  }
}

void handle_draw(unsigned char VX, unsigned char VY, unsigned char N) {
  registers[15] = 0;

  for (int i = 0; i < N; i++) {

    // since VX is the X location on the screen, and we have bytes in memory, we
    // need to divide by 8 to find which byte to update. Moreover, X may not
    // necessarily be a multiple of 8, in that case, we need to update parts of
    // 2 consecutive bytes.
    int x = VX / 8;
    int x_offset = VX % 8;
    int y = VY * 8 + i * 8;

    unsigned char sprite = memory[I + i];

    int address = PROGRAM_END + y + x;

    if (x_offset == 0) {
      update_display_byte(address, sprite);
    } else {
      update_display_byte(address, sprite >> x_offset);
      update_display_byte(address + 1, sprite << (8 - x_offset));
    }
  }
}

void empty_screen() {
  for (int i = PROGRAM_END; i < sizeof(memory); i++) {
    memory[i] = 0;
  }
}

int execute_instruction(unsigned char instruction[2], Display display) {
  // Here, we take the higher and lower nibbles of the first and second byte of
  // instruction to help figure out the instruction and execute the
  // corresponding function
  unsigned char P = instruction[0] >> 4;
  unsigned char X = instruction[0] & 0x0F;
  unsigned char Y = instruction[1] >> 4;
  unsigned char N = instruction[1] & 0x0F;


  unsigned int address = ((instruction[0] << 8) | instruction[1]) & 0xFFF;

  switch (P) {
  case 0x0:
    switch (instruction[1]) {
    case 0xE0:
      empty_screen();
      return SUCCESS;
      break;
    case 0xEE:
      if (stack_counter <= 0) {
        return STACK_UNDERFLOW;
      }
      stack_counter--;
      PC = stack[stack_counter] + 2;
      return SUCCESS_UPDATED_PC;
      break;
    }
    break;
  case 0x1:
    PC = address;
    return SUCCESS_UPDATED_PC;

  case 0x2:
    if (stack_counter >= sizeof(stack)) {
      return STACK_OVERFLOW;
    }
    stack[stack_counter] = PC;
    stack_counter++;
    PC = address;
    return SUCCESS_UPDATED_PC;

  case 0x3:
    if (registers[X] == instruction[1]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x4:
    if (registers[X] != instruction[1]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x5:
    if (registers[X] == registers[Y]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x6:
    registers[X] = instruction[1];
    break;
  case 0x7:
    registers[X] += instruction[1];
    break;
  case 0x8:
    handle_register_instruction(P, X, Y, N);
    break;
  case 0x9:
    if (registers[X] != registers[Y]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0xA:
    I = 0x0 | X << 8 | Y << 4 | N;
    break;
  case 0xB:
    PC = address + registers[0];
    return SUCCESS_UPDATED_PC;
  case 0xC:
    srand(time(0));
    registers[X] = (rand() & 0xFF) & instruction[1];
    break;
  case 0xD:
    handle_draw(registers[X], registers[Y], N);
    break;
  case 0xE:
    if (instruction[1] == 0x9E) {
      if (key == registers[X]) {
        PC += 4;
        return SUCCESS_UPDATED_PC;
      }
    } else if (instruction[1] == 0xA1) {
      if (key != registers[X]) {
        PC += 4;
        return SUCCESS_UPDATED_PC;
      }
    }
    break;
  case 0xF:
    switch (instruction[1]) {
    case 0x07:
      registers[X] = delay_timer;
      break;
    case 0x15:
      delay_timer = registers[X];
      break;
    case 0x18:
      sound_timer = registers[X];
      break;
    case 0x29:
      I = registers[X] * 5;
      break;
    case 0x33: {
      unsigned char value = registers[X];
      for (int i = 0; i < 3; i++) {
        int digit = value % 10;
        memory[I + 2 - i] = digit;
        value = value / 10;
      }
      break;
    }
    case 0x55:
      for (int i = 0; i < X + 1; i++) {
        memory[I] = registers[i];
        I++;
      }
      break;
    case 0x65:
      for (int i = 0; i < X + 1; i++) {
        registers[i] = memory[I];
        I++;
      }
      break;
    case 0x0A:
      if (key != KEY_NULL) {
        registers[X] = key;
      } else {
        wait_for_key = true;
      }
      break;
    case 0x1E:
      I += registers[X];
      break;
    }
    break;
  default:
    break;
  }

  return SUCCESS;
}

void execute_rom(Display display) {
  // CHIP-8 instructions are 2 byte each
  // hence the i+=2 at the end of the loop
  unsigned char instruction[2];

  bool quit = false;

  while (PC < PROGRAM_END && !quit) {
    if (sound_timer >= 2) {
      play_sound();
    }

    quit = handle_sdl_event();

    if (wait_for_key && key != KEY_NULL) {
      PC -= 2;
      wait_for_key = false;
    }

    if (!wait_for_key) {
      instruction[0] = memory[PC];
      instruction[1] = memory[PC + 1];

      int result = execute_instruction(instruction, display);

      if (result != SUCCESS_UPDATED_PC) {
        PC += 2;
      }

      clear_screen(display);

      draw(display, memory);
    }

    if (delay_timer > 0) {
      delay_timer--;
    }
    if (sound_timer > 0) {
      sound_timer--;
    }

    key = KEY_NULL;

    usleep(1000 * 16);
  }
}
