#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Memory{
    public:
    Memory();
    uint16_t read(uint16_t address) const;
    void write(uint16_t value,uint16_t address);    /* Reading and writing, standard hardware address */
    
    void clear_memory();   /* Reset Memory to all 0 */
    
    void load_program(const uint16_t* data,uint16_t size,uint16_t start_addr);   /* Load a program into memory at origin */

    uint16_t mem_debugger(uint16_t address) const{
        return memory[address];
    }   /* Access the data in specific memory address for debugging */

    private:
    static constexpr int MEMORY_SIZE = 65536;   /* LC3 memory locations range from x0000 to xFFFF, holding 2^16 different memory locations */
    std::array<uint16_t,MEMORY_SIZE> memory; 
    // Memory-mapped I/O (MMIO) addresses
    static constexpr uint16_t kKBSROffset = 0xFE00; // Keyboard Status
    static constexpr uint16_t kKBDROffset = 0xFE02; // Keyboard Data
    static constexpr uint16_t kDSROffset  = 0xFE04; // Display Status
    static constexpr uint16_t kDDROffset  = 0xFE06; // Display Data

};