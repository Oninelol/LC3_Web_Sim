#include "memory.h"

/* intialize memory to all zero */
Memory::Memory() {
    clear_memory();
}

uint16_t Memory::read(uint16_t address) const { /* read from memory */
    return memory[address];
}


void Memory::write(uint16_t address, uint16_t value) {  /* write memory */
    memory[address] = value;
}

void Memory::clear_memory() {   /* clear all memory */
    memory.fill(0);
}

void Memory::load_program(const uint16_t* data, size_t size, uint16_t start_addr) {
    for (size_t i = 0; i < size; i++) {
        if (start_addr + i < MEMORY_SIZE) {
            memory[start_addr + i] = data[i];
        }
    }
}