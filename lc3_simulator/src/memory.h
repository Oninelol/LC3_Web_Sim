#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Memory{
    public:
    static constexpr int MEMORY_MAX = 0xFFFF;   /* LC3 memory locations range from x0000 to xFFFF */
    Memory();
    uint16_t read(uint16_t address) const;
    void write(uint16_t value,uint16_t address);    /* Reading and writing, standard hardware address */
    
    void reset();   /* Reset Memory to all 0 */
    
    bool load_program(uint16_t origin, const std::vector<uint16_t>& instructions);   /* Load a program into memory at origin */

    private:
    std::vector<uint16_t> data;

    // Memory-mapped I/O (MMIO) addresses
    static constexpr uint16_t kKBSROffset = 0xFE00; // Keyboard Status
    static constexpr uint16_t kKBDROffset = 0xFE02; // Keyboard Data
    static constexpr uint16_t kDSROffset  = 0xFE04; // Display Status
    static constexpr uint16_t kDDROffset  = 0xFE06; // Display Data

};