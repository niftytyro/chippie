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

unsigned int PC = PROGRAM_START;

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

void handle_register_instruction(unsigned char highest_nibble,
                                 unsigned char higher_nibble,
                                 unsigned char lower_nibble,
                                 unsigned char lowest_nibble) {
  switch (lowest_nibble) {
  case 0x0:
    registers[higher_nibble] = registers[lower_nibble];
    break;
  case 0x1:
    registers[higher_nibble] =
        registers[higher_nibble] | registers[lower_nibble];
    break;
  case 0x2:
    registers[higher_nibble] =
        registers[higher_nibble] & registers[lower_nibble];
    break;
  case 0x3:
    registers[higher_nibble] =
        registers[higher_nibble] ^ registers[lower_nibble];
    break;
  case 0x4:
    registers[15] = (registers[higher_nibble] + registers[lower_nibble]) > 0xFF;
    registers[higher_nibble] += registers[lower_nibble];
    break;
  case 0x5:
    registers[15] = registers[higher_nibble] >= registers[lower_nibble];
    registers[higher_nibble] -= registers[lower_nibble];
    break;
  case 0x6:
    registers[15] = registers[lower_nibble] & 1;
    registers[higher_nibble] = registers[lower_nibble] >> 1;
    break;
  case 0x7:
    registers[15] = registers[lower_nibble] >= registers[higher_nibble];
    registers[higher_nibble] =
        registers[lower_nibble] - registers[higher_nibble];
    break;
  case 0xE:
    registers[15] = (registers[lower_nibble] & 128) / 128;
    registers[higher_nibble] = registers[lower_nibble] << 1;
    break;
  default:
    break;
  }
}
void handle_draw(unsigned char VX, unsigned char VY, unsigned char N) {
  registers[15] = 0;
  int mem_location = PROGRAM_END + (registers[VY] * 8) + registers[VX];

  for (int i = 0; i < N; i++) {
    unsigned char new_pixels = memory[mem_location + i] ^ memory[I + i];
    if ((memory[mem_location + i] & new_pixels) == memory[mem_location + i]) {
      registers[15] = 1;
    }
    memory[mem_location + i] = new_pixels;
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
  unsigned char highest_nibble = (instruction[0] & 0xF0) >> 4;
  unsigned char higher_nibble = instruction[0] & 0x0F;
  unsigned char lower_nibble = (instruction[1] & 0xF0) >> 4;
  unsigned char lowest_nibble = instruction[1] & 0x0F;

  unsigned char cls_instruction = 0x00E0;
  unsigned char return_instruction = 0x00EE;

  // Looking for and executing instructions which don't
  // have variable nibbles first
  if (memcmp(instruction, &cls_instruction, 2) == 0) {
    empty_screen();
    return SUCCESS;
  }

  if (memcmp(instruction, &return_instruction, 2) == 0) {
    if (stack_counter <= 0) {
      return STACK_UNDERFLOW;
    }
    stack_counter--;
    PC = stack[stack_counter];
    return SUCCESS_UPDATED_PC;
  }

  switch (highest_nibble) {
  case 0x1: {
    unsigned int address =
        0x0 | higher_nibble << 8 | lower_nibble << 4 | lowest_nibble;
    PC = address;
    return SUCCESS_UPDATED_PC;
  }
  case 0x2: {
    if (stack_counter >= sizeof(stack)) {
      return STACK_OVERFLOW;
    }
    stack[stack_counter] = PC;
    stack_counter++;
    unsigned int address =
        0x0 | higher_nibble << 8 | lower_nibble << 4 | lowest_nibble;
    PC = address;
    return SUCCESS_UPDATED_PC;
  }
  case 0x3:
    if (registers[higher_nibble] == instruction[1]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x4:
    if (registers[higher_nibble] != instruction[1]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x5:
    if (registers[higher_nibble] == registers[lower_nibble]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0x6:
    registers[higher_nibble] = instruction[1];
    break;
  case 0x7:
    registers[higher_nibble] += instruction[1];
    break;
  case 0x8:
    handle_register_instruction(highest_nibble, higher_nibble, lower_nibble,
                                lowest_nibble);
    break;
  case 0x9:
    if (registers[higher_nibble] != registers[lower_nibble]) {
      PC += 4;
      return SUCCESS_UPDATED_PC;
    }
    break;
  case 0xA:
    I = 0x0 | higher_nibble << 8 | lower_nibble << 4 | lowest_nibble;
    break;
  case 0xB: {
    unsigned int address =
        0x0 | higher_nibble << 8 | lower_nibble << 4 | lowest_nibble;
    PC = address + registers[0];
    return SUCCESS_UPDATED_PC;
  }
  case 0xC:
    srand(time(0));
    registers[higher_nibble] = (rand() % 0xFF) & instruction[1];
    break;
  case 0xD:
    handle_draw(registers[higher_nibble], registers[lower_nibble],
                registers[lowest_nibble]);
    break;
  case 0xE:
    if (instruction[1] == 0x9E) {
      if (key == registers[higher_nibble]) {
        PC += 4;
        return SUCCESS_UPDATED_PC;
      }
    } else if (instruction[1] == 0xA1) {
      if (key != registers[higher_nibble]) {
        PC += 4;
        return SUCCESS_UPDATED_PC;
      }
    }
    break;
  case 0xF:
    switch (instruction[1]) {
    case 0x15:
      delay_timer = registers[higher_nibble];
      break;
    case 0x07:
      registers[higher_nibble] = delay_timer;
      break;
    case 0x18:
      sound_timer = registers[higher_nibble];
      break;
    case 0x0A:
      if (key != KEY_NULL) {
        registers[higher_nibble] = key;
      } else {
        wait_for_key = true;
      }
      break;
    case 0x1E:
      I += registers[higher_nibble];
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

void run(Display display) {
  bool quit = false;

  while (!quit) {

    quit = handle_sdl_event();
    clear_screen(display);

    /* SDL_Rect rect = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 10, 10}; */
    /* SDL_SetRenderDrawColor(display.renderer, 255, 255, 255, 255); */
    /* SDL_RenderFillRect(display.renderer, &rect); */

    redraw_frame(display);
  }
}
